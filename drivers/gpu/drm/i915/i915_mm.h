/* SPDX-License-Identifier: MIT */
/*
 * Copyright © 2021 Intel Corporation
 */

#ifndef __I915_MM_H__
#define __I915_MM_H__

#include <linux/types.h>

struct vm_area_struct;
struct io_mapping;
struct scatterlist;

#if IS_ENABLED(CONFIG_X86)
int remap_io_mapping(struct vm_area_struct *vma,
		     unsigned long addr, unsigned long pfn, unsigned long size,
		     struct io_mapping *iomap);
#else
static inline
int remap_io_mapping(struct vm_area_struct *vma,
		     unsigned long addr, unsigned long pfn, unsigned long size,
		     struct io_mapping *iomap)
{
	pr_err("Architecture has no %s() and shouldn't be calling this function\n", __func__);
	WARN_ON_ONCE(1);
	return 0;
}
#endif

int remap_io_sg(struct vm_area_struct *vma,
		unsigned long addr, unsigned long size,
		struct scatterlist *sgl, resource_size_t iobase);

#endif /* __I915_MM_H__ */
