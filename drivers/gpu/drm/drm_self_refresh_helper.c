// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2019 Google, Inc.
 *
 * Authors:
 * Sean Paul <seanpaul@chromium.org>
 */
#include <linux/bitops.h>
#include <linux/slab.h>
#include <linux/workqueue.h>

#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_connector.h>
#include <drm/drm_crtc.h>
#include <drm/drm_device.h>
#include <drm/drm_mode_config.h>
#include <drm/drm_modeset_lock.h>
#include <drm/drm_print.h>
#include <drm/drm_self_refresh_helper.h>

/**
 * DOC: overview
 *
 * This helper library provides an easy way for drivers to leverage the atomic
 * framework to implement panel self refresh (SR) support. Drivers are
 * responsible for initializing and cleaning up the SR helpers on load/unload
 * (see &drm_self_refresh_helper_init/&drm_self_refresh_helper_cleanup).
 * The connector is responsible for setting
 * &drm_connector_state.self_refresh_aware to true at runtime if it is SR-aware
 * (meaning it knows how to initiate self refresh on the panel).
 *
 * Once a crtc has enabled SR using &drm_self_refresh_helper_init, the
 * helpers will monitor activity and call back into the driver to enable/disable
 * SR as appropriate. The best way to think about this is that it's a DPMS
 * on/off request with &drm_crtc_state.self_refresh_active set in crtc state
 * that tells you to disable/enable SR on the panel instead of power-cycling it.
 *
 * During SR, drivers may choose to fully disable their crtc/encoder/bridge
 * hardware (in which case no driver changes are necessary), or they can inspect
 * &drm_crtc_state.self_refresh_active if they want to enter low power mode
 * without full disable (in case full disable/enable is too slow).
 *
 * SR will be deactivated if there are any atomic updates affecting the
 * pipe that is in SR mode. If a crtc is driving multiple connectors, all
 * connectors must be SR aware and all will enter/exit SR mode at the same time.
 *
 * If the crtc and connector are SR aware, but the panel connected does not
 * support it (or is otherwise unable to enter SR), the driver should fail
 * atomic_check when &drm_crtc_state.self_refresh_active is true.
 */

struct drm_self_refresh_data {
	struct drm_crtc *crtc;
	struct delayed_work entry_work;
	struct drm_atomic_state *save_state;
	unsigned int entry_delay_ms;
};

static void drm_self_refresh_helper_entry_work(struct work_struct *work)
{
	struct drm_self_refresh_data *sr_data = container_of(
				to_delayed_work(work),
				struct drm_self_refresh_data, entry_work);
	struct drm_crtc *crtc = sr_data->crtc;
	struct drm_device *dev = crtc->dev;
	struct drm_modeset_acquire_ctx ctx;
	struct drm_atomic_state *state;
	struct drm_connector *conn;
	struct drm_connector_state *conn_state;
	struct drm_crtc_state *crtc_state;
	int i, ret;

	drm_modeset_acquire_init(&ctx, 0);

	state = drm_atomic_state_alloc(dev);
	if (!state) {
		ret = -ENOMEM;
		goto out;
	}

retry:
	state->acquire_ctx = &ctx;

	crtc_state = drm_atomic_get_crtc_state(state, crtc);
	if (IS_ERR(crtc_state)) {
		ret = PTR_ERR(crtc_state);
		goto out;
	}

	if (!crtc_state->enable)
		goto out;

	ret = drm_atomic_add_affected_connectors(state, crtc);
	if (ret)
		goto out;

	for_each_new_connector_in_state(state, conn, conn_state, i) {
		if (!conn_state->self_refresh_aware)
			goto out;
	}

	crtc_state->active = false;
	crtc_state->self_refresh_active = true;

	ret = drm_atomic_commit(state);
	if (ret)
		goto out;

out:
	if (ret == -EDEADLK) {
		drm_atomic_state_clear(state);
		ret = drm_modeset_backoff(&ctx);
		if (!ret)
			goto retry;
	}

	drm_atomic_state_put(state);
	drm_modeset_drop_locks(&ctx);
	drm_modeset_acquire_fini(&ctx);
}

/**
 * drm_self_refresh_helper_alter_state - Alters the atomic state for SR exit
 * @state: the state currently being checked
 *
 * Called at the end of atomic check. This function checks the state for flags
 * incompatible with self refresh exit and changes them. This is a bit
 * disingenuous since userspace is expecting one thing and we're giving it
 * another. However in order to keep self refresh entirely hidden from
 * userspace, this is required.
 *
 * At the end, we queue up the self refresh entry work so we can enter PSR after
 * the desired delay.
 */
void drm_self_refresh_helper_alter_state(struct drm_atomic_state *state)
{
	struct drm_crtc *crtc;
	struct drm_crtc_state *crtc_state;
	int i;

	if (state->async_update || !state->allow_modeset) {
		for_each_old_crtc_in_state(state, crtc, crtc_state, i) {
			if (crtc_state->self_refresh_active) {
				state->async_update = false;
				state->allow_modeset = true;
				break;
			}
		}
	}

	for_each_new_crtc_in_state(state, crtc, crtc_state, i) {
		struct drm_self_refresh_data *sr_data;

		/* Don't trigger the entry timer when we're already in SR */
		if (crtc_state->self_refresh_active)
			continue;

		sr_data = crtc->self_refresh_data;
		if (!sr_data)
			continue;

		mod_delayed_work(system_wq, &sr_data->entry_work,
				 msecs_to_jiffies(sr_data->entry_delay_ms));
	}
}
EXPORT_SYMBOL(drm_self_refresh_helper_alter_state);

/**
 * drm_self_refresh_helper_init - Initializes self refresh helpers for a crtc
 * @crtc: the crtc which supports self refresh supported displays
 * @entry_delay_ms: amount of inactivity to wait before entering self refresh
 *
 * Returns zero if successful or -errno on failure
 */
int drm_self_refresh_helper_init(struct drm_crtc *crtc,
				 unsigned int entry_delay_ms)
{
	struct drm_self_refresh_data *sr_data = crtc->self_refresh_data;

	/* Helper is already initialized */
	if (WARN_ON(sr_data))
		return -EINVAL;

	sr_data = kzalloc(sizeof(*sr_data), GFP_KERNEL);
	if (!sr_data)
		return -ENOMEM;

	INIT_DELAYED_WORK(&sr_data->entry_work,
			  drm_self_refresh_helper_entry_work);
	sr_data->entry_delay_ms = entry_delay_ms;
	sr_data->crtc = crtc;

	crtc->self_refresh_data = sr_data;
	return 0;
}
EXPORT_SYMBOL(drm_self_refresh_helper_init);

/**
 * drm_self_refresh_helper_cleanup - Cleans up self refresh helpers for a crtc
 * @crtc: the crtc to cleanup
 */
void drm_self_refresh_helper_cleanup(struct drm_crtc *crtc)
{
	struct drm_self_refresh_data *sr_data = crtc->self_refresh_data;

	/* Helper is already uninitialized */
	if (sr_data)
		return;

	crtc->self_refresh_data = NULL;

	cancel_delayed_work_sync(&sr_data->entry_work);
	kfree(sr_data);
}
EXPORT_SYMBOL(drm_self_refresh_helper_cleanup);
