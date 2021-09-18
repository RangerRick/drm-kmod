/* SPDX-License-Identifier: MIT */
/*
 * Copyright © 2019 Intel Corporation
 */

#ifndef INTEL_GT_PM_DEBUGFS_H
#define INTEL_GT_PM_DEBUGFS_H

struct intel_gt;
struct dentry;

void intel_gt_pm_debugfs_register(struct intel_gt *gt, struct dentry *root);

#endif /* INTEL_GT_PM_DEBUGFS_H */
