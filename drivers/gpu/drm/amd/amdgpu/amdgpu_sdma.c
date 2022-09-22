/*
 * Copyright 2018 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <linux/firmware.h>
#include "amdgpu.h"
#include "amdgpu_sdma.h"
#include "amdgpu_ras.h"

#define AMDGPU_CSA_SDMA_SIZE 64
/* SDMA CSA reside in the 3rd page of CSA */
#define AMDGPU_CSA_SDMA_OFFSET (4096 * 2)

/*
 * GPU SDMA IP block helpers function.
 */

struct amdgpu_sdma_instance *amdgpu_sdma_get_instance_from_ring(struct amdgpu_ring *ring)
{
	struct amdgpu_device *adev = ring->adev;
	int i;

	for (i = 0; i < adev->sdma.num_instances; i++)
		if (ring == &adev->sdma.instance[i].ring ||
		    ring == &adev->sdma.instance[i].page)
			return &adev->sdma.instance[i];

	return NULL;
}

int amdgpu_sdma_get_index_from_ring(struct amdgpu_ring *ring, uint32_t *index)
{
	struct amdgpu_device *adev = ring->adev;
	int i;

	for (i = 0; i < adev->sdma.num_instances; i++) {
		if (ring == &adev->sdma.instance[i].ring ||
			ring == &adev->sdma.instance[i].page) {
			*index = i;
			return 0;
		}
	}

	return -EINVAL;
}

uint64_t amdgpu_sdma_get_csa_mc_addr(struct amdgpu_ring *ring,
				     unsigned vmid)
{
	struct amdgpu_device *adev = ring->adev;
	uint64_t csa_mc_addr;
	uint32_t index = 0;
	int r;

	/* don't enable OS preemption on SDMA under SRIOV */
	if (amdgpu_sriov_vf(adev) || vmid == 0 || !amdgpu_mcbp)
		return 0;

	if (ring->is_mes_queue) {
		uint32_t offset = 0;

		offset = offsetof(struct amdgpu_mes_ctx_meta_data,
				  sdma[ring->idx].sdma_meta_data);
		csa_mc_addr = amdgpu_mes_ctx_get_offs_gpu_addr(ring, offset);
	} else {
		r = amdgpu_sdma_get_index_from_ring(ring, &index);

		if (r || index > 31)
			csa_mc_addr = 0;
		else
			csa_mc_addr = amdgpu_csa_vaddr(adev) +
				AMDGPU_CSA_SDMA_OFFSET +
				index * AMDGPU_CSA_SDMA_SIZE;
	}

	return csa_mc_addr;
}

int amdgpu_sdma_ras_late_init(struct amdgpu_device *adev,
			      struct ras_common_if *ras_block)
{
	int r, i;

	r = amdgpu_ras_block_late_init(adev, ras_block);
	if (r)
		return r;

	if (amdgpu_ras_is_supported(adev, ras_block->block)) {
		for (i = 0; i < adev->sdma.num_instances; i++) {
			r = amdgpu_irq_get(adev, &adev->sdma.ecc_irq,
				AMDGPU_SDMA_IRQ_INSTANCE0 + i);
			if (r)
				goto late_fini;
		}
	}

	return 0;

late_fini:
	amdgpu_ras_block_late_fini(adev, ras_block);
	return r;
}

int amdgpu_sdma_process_ras_data_cb(struct amdgpu_device *adev,
		void *err_data,
		struct amdgpu_iv_entry *entry)
{
	kgd2kfd_set_sram_ecc_flag(adev->kfd.dev);

	if (amdgpu_sriov_vf(adev))
		return AMDGPU_RAS_SUCCESS;

	amdgpu_ras_reset_gpu(adev);

	return AMDGPU_RAS_SUCCESS;
}

int amdgpu_sdma_process_ecc_irq(struct amdgpu_device *adev,
				      struct amdgpu_irq_src *source,
				      struct amdgpu_iv_entry *entry)
{
	struct ras_common_if *ras_if = adev->sdma.ras_if;
	struct ras_dispatch_if ih_data = {
		.entry = entry,
	};

	if (!ras_if)
		return 0;

	ih_data.head = *ras_if;

	amdgpu_ras_interrupt_dispatch(adev, &ih_data);
	return 0;
}

static int amdgpu_sdma_init_inst_ctx(struct amdgpu_sdma_instance *sdma_inst)
{
	int err = 0;
	const struct sdma_firmware_header_v1_0 *hdr;

	err = amdgpu_ucode_validate(sdma_inst->fw);
	if (err)
		return err;

	hdr = (const struct sdma_firmware_header_v1_0 *)sdma_inst->fw->data;
	sdma_inst->fw_version = le32_to_cpu(hdr->header.ucode_version);
	sdma_inst->feature_version = le32_to_cpu(hdr->ucode_feature_version);

	if (sdma_inst->feature_version >= 20)
		sdma_inst->burst_nop = true;

	return 0;
}

void amdgpu_sdma_destroy_inst_ctx(struct amdgpu_device *adev,
				  bool duplicate)
{
	int i;

	for (i = 0; i < adev->sdma.num_instances; i++) {
		release_firmware(adev->sdma.instance[i].fw);
		if (duplicate)
			break;
	}

	memset((void *)adev->sdma.instance, 0,
	       sizeof(struct amdgpu_sdma_instance) * AMDGPU_MAX_SDMA_INSTANCES);
}

int amdgpu_sdma_init_microcode(struct amdgpu_device *adev,
			       char *fw_name, u32 instance,
			       bool duplicate)
{
	struct amdgpu_firmware_info *info = NULL;
	const struct common_firmware_header *header = NULL;
	int err = 0, i;

	if (duplicate && instance)
		return -EINVAL;

	err = request_firmware(&adev->sdma.instance[instance].fw, fw_name, adev->dev);
	if (err)
		goto out;
	err = amdgpu_sdma_init_inst_ctx(&adev->sdma.instance[instance]);
	if (err)
		goto out;

	if (duplicate) {
		for (i = 1; i < adev->sdma.num_instances; i++)
			memcpy((void *)&adev->sdma.instance[i],
			       (void *)&adev->sdma.instance[0],
			       sizeof(struct amdgpu_sdma_instance));
	}

	if (amdgpu_sriov_vf(adev))
		return 0;

	DRM_DEBUG("psp_load == '%s'\n",
		  adev->firmware.load_type == AMDGPU_FW_LOAD_PSP ? "true" : "false");

	if (adev->firmware.load_type == AMDGPU_FW_LOAD_PSP) {
		for (i = 0; i < adev->sdma.num_instances; i++) {
			if (!duplicate && (instance != i))
				continue;
			else {
				info = &adev->firmware.ucode[AMDGPU_UCODE_ID_SDMA0 + i];
				info->ucode_id = AMDGPU_UCODE_ID_SDMA0 + i;
				info->fw = adev->sdma.instance[i].fw;
				header = (const struct common_firmware_header *)info->fw->data;
				adev->firmware.fw_size +=
					ALIGN(le32_to_cpu(header->ucode_size_bytes), PAGE_SIZE);
			}
		}
	}

out:
	if (err) {
		DRM_ERROR("SDMA: Failed to init firmware \"%s\"\n", fw_name);
		amdgpu_sdma_destroy_inst_ctx(adev, duplicate);
	}
	return err;
}
