/* SPDX-License-Identifier: MIT */
/*
 * Copyright © 2021 Intel Corporation
 */

#ifndef _INTEL_GUC_SLPC_H_
#define _INTEL_GUC_SLPC_H_

#include "intel_guc_submission.h"
#include "intel_guc_slpc_types.h"

static inline bool intel_guc_slpc_is_supported(struct intel_guc *guc)
{
	return guc->slpc.supported;
}

static inline bool intel_guc_slpc_is_wanted(struct intel_guc *guc)
{
	return guc->slpc.selected;
}

static inline bool intel_guc_slpc_is_used(struct intel_guc *guc)
{
	return intel_guc_submission_is_used(guc) && intel_guc_slpc_is_wanted(guc);
}

void intel_guc_slpc_init_early(struct intel_guc_slpc *slpc);

int intel_guc_slpc_init(struct intel_guc_slpc *slpc);
int intel_guc_slpc_enable(struct intel_guc_slpc *slpc);
void intel_guc_slpc_fini(struct intel_guc_slpc *slpc);
int intel_guc_slpc_set_max_freq(struct intel_guc_slpc *slpc, u32 val);
int intel_guc_slpc_set_min_freq(struct intel_guc_slpc *slpc, u32 val);

#endif