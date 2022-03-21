/* SPDX-License-Identifier: MIT */
/*
 * Copyright © 2021-2022 Intel Corporation
 */

#ifndef _INTEL_GUC_CAPTURE_FWIF_H
#define _INTEL_GUC_CAPTURE_FWIF_H

#include <linux/types.h>
#include "intel_guc_fwif.h"

struct intel_guc;
struct file;

/**
 * struct guc_debug_capture_list_header / struct guc_debug_capture_list
 *
 * As part of ADS registration, these header structures (followed by
 * an array of 'struct guc_mmio_reg' entries) are used to register with
 * GuC microkernel the list of registers we want it to dump out prior
 * to a engine reset.
 */
struct guc_debug_capture_list_header {
	u32 info;
#define GUC_CAPTURELISTHDR_NUMDESCR GENMASK(15, 0)
} __packed;

struct guc_debug_capture_list {
	struct guc_debug_capture_list_header header;
	struct guc_mmio_reg regs[0];
} __packed;

/**
 * struct __guc_mmio_reg_descr / struct __guc_mmio_reg_descr_group
 *
 * intel_guc_capture module uses these structures to maintain static
 * tables (per unique platform) that consists of lists of registers
 * (offsets, names, flags,...) that are used at the ADS regisration
 * time as well as during runtime processing and reporting of error-
 * capture states generated by GuC just prior to engine reset events.
 */
struct __guc_mmio_reg_descr {
	i915_reg_t reg;
	u32 flags;
	u32 mask;
	const char *regname;
};

struct __guc_mmio_reg_descr_group {
	const struct __guc_mmio_reg_descr *list;
	u32 num_regs;
	u32 owner; /* see enum guc_capture_owner */
	u32 type; /* see enum guc_capture_type */
	u32 engine; /* as per MAX_ENGINE_CLASS */
};

/**
 * struct __guc_capture_ads_cache
 *
 * A structure to cache register lists that were populated and registered
 * with GuC at startup during ADS registration. This allows much quicker
 * GuC resets without re-parsing all the tables for the given gt.
 */
struct __guc_capture_ads_cache {
	bool is_valid;
	void *ptr;
	size_t size;
	int status;
};

/**
 * struct intel_guc_state_capture
 *
 * Internal context of the intel_guc_capture module.
 */
struct intel_guc_state_capture {
	/**
	 * @reglists: static table of register lists used for error-capture state.
	 */
	const struct __guc_mmio_reg_descr_group *reglists;

	/**
	 * @ads_cache: cached register lists that is ADS format ready
	 */
	struct __guc_capture_ads_cache ads_cache[GUC_CAPTURE_LIST_INDEX_MAX]
						[GUC_CAPTURE_LIST_TYPE_MAX]
						[GUC_MAX_ENGINE_CLASSES];
	void *ads_null_cache;
};

#endif /* _INTEL_GUC_CAPTURE_FWIF_H */
