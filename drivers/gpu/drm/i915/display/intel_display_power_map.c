// SPDX-License-Identifier: MIT
/*
 * Copyright © 2022 Intel Corporation
 */

#include "i915_drv.h"
#include "i915_reg.h"

#include "vlv_sideband_reg.h"

#include "intel_display_power_map.h"
#include "intel_display_power_well.h"

#define POWER_DOMAIN_MASK (GENMASK_ULL(POWER_DOMAIN_NUM - 1, 0))

static const struct i915_power_well_desc i9xx_always_on_power_well[] = {
	{
		.name = "always-on",
		.domains = POWER_DOMAIN_MASK,
		.ops = &i9xx_always_on_power_well_ops,
		.always_on = true,
		.id = DISP_PW_ID_NONE,
	},
};

#define I830_PIPES_POWER_DOMAINS (		\
	BIT_ULL(POWER_DOMAIN_PIPE_A) |		\
	BIT_ULL(POWER_DOMAIN_PIPE_B) |		\
	BIT_ULL(POWER_DOMAIN_PIPE_PANEL_FITTER_A) |	\
	BIT_ULL(POWER_DOMAIN_PIPE_PANEL_FITTER_B) |	\
	BIT_ULL(POWER_DOMAIN_TRANSCODER_A) |	\
	BIT_ULL(POWER_DOMAIN_TRANSCODER_B) |	\
	BIT_ULL(POWER_DOMAIN_INIT))

static const struct i915_power_well_desc i830_power_wells[] = {
	{
		.name = "always-on",
		.domains = POWER_DOMAIN_MASK,
		.ops = &i9xx_always_on_power_well_ops,
		.always_on = true,
		.id = DISP_PW_ID_NONE,
	}, {
		.name = "pipes",
		.domains = I830_PIPES_POWER_DOMAINS,
		.ops = &i830_pipes_power_well_ops,
		.id = DISP_PW_ID_NONE,
	},
};

#define HSW_DISPLAY_POWER_DOMAINS (			\
	BIT_ULL(POWER_DOMAIN_PIPE_B) |			\
	BIT_ULL(POWER_DOMAIN_PIPE_C) |			\
	BIT_ULL(POWER_DOMAIN_PIPE_PANEL_FITTER_A) |		\
	BIT_ULL(POWER_DOMAIN_PIPE_PANEL_FITTER_B) |		\
	BIT_ULL(POWER_DOMAIN_PIPE_PANEL_FITTER_C) |		\
	BIT_ULL(POWER_DOMAIN_TRANSCODER_A) |		\
	BIT_ULL(POWER_DOMAIN_TRANSCODER_B) |		\
	BIT_ULL(POWER_DOMAIN_TRANSCODER_C) |		\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_B) |		\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_C) |		\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_D) |		\
	BIT_ULL(POWER_DOMAIN_PORT_CRT) | /* DDI E */	\
	BIT_ULL(POWER_DOMAIN_VGA) |				\
	BIT_ULL(POWER_DOMAIN_AUDIO_MMIO) |		\
	BIT_ULL(POWER_DOMAIN_AUDIO_PLAYBACK) |			\
	BIT_ULL(POWER_DOMAIN_INIT))

static const struct i915_power_well_desc hsw_power_wells[] = {
	{
		.name = "always-on",
		.domains = POWER_DOMAIN_MASK,
		.ops = &i9xx_always_on_power_well_ops,
		.always_on = true,
		.id = DISP_PW_ID_NONE,
	}, {
		.name = "display",
		.domains = HSW_DISPLAY_POWER_DOMAINS,
		.ops = &hsw_power_well_ops,
		.has_vga = true,
		.id = HSW_DISP_PW_GLOBAL,
		{
			.hsw.idx = HSW_PW_CTL_IDX_GLOBAL,
		},
	},
};

#define BDW_DISPLAY_POWER_DOMAINS (			\
	BIT_ULL(POWER_DOMAIN_PIPE_B) |			\
	BIT_ULL(POWER_DOMAIN_PIPE_C) |			\
	BIT_ULL(POWER_DOMAIN_PIPE_PANEL_FITTER_B) |		\
	BIT_ULL(POWER_DOMAIN_PIPE_PANEL_FITTER_C) |		\
	BIT_ULL(POWER_DOMAIN_TRANSCODER_A) |		\
	BIT_ULL(POWER_DOMAIN_TRANSCODER_B) |		\
	BIT_ULL(POWER_DOMAIN_TRANSCODER_C) |		\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_B) |		\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_C) |		\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_D) |		\
	BIT_ULL(POWER_DOMAIN_PORT_CRT) | /* DDI E */	\
	BIT_ULL(POWER_DOMAIN_VGA) |				\
	BIT_ULL(POWER_DOMAIN_AUDIO_MMIO) |		\
	BIT_ULL(POWER_DOMAIN_AUDIO_PLAYBACK) |			\
	BIT_ULL(POWER_DOMAIN_INIT))

static const struct i915_power_well_desc bdw_power_wells[] = {
	{
		.name = "always-on",
		.domains = POWER_DOMAIN_MASK,
		.ops = &i9xx_always_on_power_well_ops,
		.always_on = true,
		.id = DISP_PW_ID_NONE,
	}, {
		.name = "display",
		.domains = BDW_DISPLAY_POWER_DOMAINS,
		.ops = &hsw_power_well_ops,
		.has_vga = true,
		.irq_pipe_mask = BIT(PIPE_B) | BIT(PIPE_C),
		.id = HSW_DISP_PW_GLOBAL,
		{
			.hsw.idx = HSW_PW_CTL_IDX_GLOBAL,
		},
	},
};

#define VLV_DISPLAY_POWER_DOMAINS (		\
	BIT_ULL(POWER_DOMAIN_DISPLAY_CORE) |	\
	BIT_ULL(POWER_DOMAIN_PIPE_A) |		\
	BIT_ULL(POWER_DOMAIN_PIPE_B) |		\
	BIT_ULL(POWER_DOMAIN_PIPE_PANEL_FITTER_A) |	\
	BIT_ULL(POWER_DOMAIN_PIPE_PANEL_FITTER_B) |	\
	BIT_ULL(POWER_DOMAIN_TRANSCODER_A) |	\
	BIT_ULL(POWER_DOMAIN_TRANSCODER_B) |	\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_B) |	\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_C) |	\
	BIT_ULL(POWER_DOMAIN_PORT_DSI) |		\
	BIT_ULL(POWER_DOMAIN_PORT_CRT) |		\
	BIT_ULL(POWER_DOMAIN_VGA) |			\
	BIT_ULL(POWER_DOMAIN_AUDIO_MMIO) |		\
	BIT_ULL(POWER_DOMAIN_AUDIO_PLAYBACK) |		\
	BIT_ULL(POWER_DOMAIN_AUX_B) |		\
	BIT_ULL(POWER_DOMAIN_AUX_C) |		\
	BIT_ULL(POWER_DOMAIN_GMBUS) |		\
	BIT_ULL(POWER_DOMAIN_INIT))

#define VLV_DPIO_CMN_BC_POWER_DOMAINS (		\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_B) |	\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_C) |	\
	BIT_ULL(POWER_DOMAIN_PORT_CRT) |		\
	BIT_ULL(POWER_DOMAIN_AUX_B) |		\
	BIT_ULL(POWER_DOMAIN_AUX_C) |		\
	BIT_ULL(POWER_DOMAIN_INIT))

#define VLV_DPIO_TX_B_LANES_01_POWER_DOMAINS (	\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_B) |	\
	BIT_ULL(POWER_DOMAIN_AUX_B) |		\
	BIT_ULL(POWER_DOMAIN_INIT))

#define VLV_DPIO_TX_B_LANES_23_POWER_DOMAINS (	\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_B) |	\
	BIT_ULL(POWER_DOMAIN_AUX_B) |		\
	BIT_ULL(POWER_DOMAIN_INIT))

#define VLV_DPIO_TX_C_LANES_01_POWER_DOMAINS (	\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_C) |	\
	BIT_ULL(POWER_DOMAIN_AUX_C) |		\
	BIT_ULL(POWER_DOMAIN_INIT))

#define VLV_DPIO_TX_C_LANES_23_POWER_DOMAINS (	\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_C) |	\
	BIT_ULL(POWER_DOMAIN_AUX_C) |		\
	BIT_ULL(POWER_DOMAIN_INIT))

static const struct i915_power_well_desc vlv_power_wells[] = {
	{
		.name = "always-on",
		.domains = POWER_DOMAIN_MASK,
		.ops = &i9xx_always_on_power_well_ops,
		.always_on = true,
		.id = DISP_PW_ID_NONE,
	}, {
		.name = "display",
		.domains = VLV_DISPLAY_POWER_DOMAINS,
		.ops = &vlv_display_power_well_ops,
		.id = VLV_DISP_PW_DISP2D,
		{
			.vlv.idx = PUNIT_PWGT_IDX_DISP2D,
		},
	}, {
		.name = "dpio-tx-b-01",
		.domains = VLV_DPIO_TX_B_LANES_01_POWER_DOMAINS |
			   VLV_DPIO_TX_B_LANES_23_POWER_DOMAINS |
			   VLV_DPIO_TX_C_LANES_01_POWER_DOMAINS |
			   VLV_DPIO_TX_C_LANES_23_POWER_DOMAINS,
		.ops = &vlv_dpio_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.vlv.idx = PUNIT_PWGT_IDX_DPIO_TX_B_LANES_01,
		},
	}, {
		.name = "dpio-tx-b-23",
		.domains = VLV_DPIO_TX_B_LANES_01_POWER_DOMAINS |
			   VLV_DPIO_TX_B_LANES_23_POWER_DOMAINS |
			   VLV_DPIO_TX_C_LANES_01_POWER_DOMAINS |
			   VLV_DPIO_TX_C_LANES_23_POWER_DOMAINS,
		.ops = &vlv_dpio_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.vlv.idx = PUNIT_PWGT_IDX_DPIO_TX_B_LANES_23,
		},
	}, {
		.name = "dpio-tx-c-01",
		.domains = VLV_DPIO_TX_B_LANES_01_POWER_DOMAINS |
			   VLV_DPIO_TX_B_LANES_23_POWER_DOMAINS |
			   VLV_DPIO_TX_C_LANES_01_POWER_DOMAINS |
			   VLV_DPIO_TX_C_LANES_23_POWER_DOMAINS,
		.ops = &vlv_dpio_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.vlv.idx = PUNIT_PWGT_IDX_DPIO_TX_C_LANES_01,
		},
	}, {
		.name = "dpio-tx-c-23",
		.domains = VLV_DPIO_TX_B_LANES_01_POWER_DOMAINS |
			   VLV_DPIO_TX_B_LANES_23_POWER_DOMAINS |
			   VLV_DPIO_TX_C_LANES_01_POWER_DOMAINS |
			   VLV_DPIO_TX_C_LANES_23_POWER_DOMAINS,
		.ops = &vlv_dpio_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.vlv.idx = PUNIT_PWGT_IDX_DPIO_TX_C_LANES_23,
		},
	}, {
		.name = "dpio-common",
		.domains = VLV_DPIO_CMN_BC_POWER_DOMAINS,
		.ops = &vlv_dpio_cmn_power_well_ops,
		.id = VLV_DISP_PW_DPIO_CMN_BC,
		{
			.vlv.idx = PUNIT_PWGT_IDX_DPIO_CMN_BC,
		},
	},
};

#define CHV_DISPLAY_POWER_DOMAINS (		\
	BIT_ULL(POWER_DOMAIN_DISPLAY_CORE) |	\
	BIT_ULL(POWER_DOMAIN_PIPE_A) |		\
	BIT_ULL(POWER_DOMAIN_PIPE_B) |		\
	BIT_ULL(POWER_DOMAIN_PIPE_C) |		\
	BIT_ULL(POWER_DOMAIN_PIPE_PANEL_FITTER_A) |	\
	BIT_ULL(POWER_DOMAIN_PIPE_PANEL_FITTER_B) |	\
	BIT_ULL(POWER_DOMAIN_PIPE_PANEL_FITTER_C) |	\
	BIT_ULL(POWER_DOMAIN_TRANSCODER_A) |	\
	BIT_ULL(POWER_DOMAIN_TRANSCODER_B) |	\
	BIT_ULL(POWER_DOMAIN_TRANSCODER_C) |	\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_B) |	\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_C) |	\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_D) |	\
	BIT_ULL(POWER_DOMAIN_PORT_DSI) |		\
	BIT_ULL(POWER_DOMAIN_VGA) |			\
	BIT_ULL(POWER_DOMAIN_AUDIO_MMIO) |		\
	BIT_ULL(POWER_DOMAIN_AUDIO_PLAYBACK) |		\
	BIT_ULL(POWER_DOMAIN_AUX_B) |		\
	BIT_ULL(POWER_DOMAIN_AUX_C) |		\
	BIT_ULL(POWER_DOMAIN_AUX_D) |		\
	BIT_ULL(POWER_DOMAIN_GMBUS) |		\
	BIT_ULL(POWER_DOMAIN_INIT))

#define CHV_DPIO_CMN_BC_POWER_DOMAINS (		\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_B) |	\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_C) |	\
	BIT_ULL(POWER_DOMAIN_AUX_B) |		\
	BIT_ULL(POWER_DOMAIN_AUX_C) |		\
	BIT_ULL(POWER_DOMAIN_INIT))

#define CHV_DPIO_CMN_D_POWER_DOMAINS (		\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_D) |	\
	BIT_ULL(POWER_DOMAIN_AUX_D) |		\
	BIT_ULL(POWER_DOMAIN_INIT))

static const struct i915_power_well_desc chv_power_wells[] = {
	{
		.name = "always-on",
		.domains = POWER_DOMAIN_MASK,
		.ops = &i9xx_always_on_power_well_ops,
		.always_on = true,
		.id = DISP_PW_ID_NONE,
	}, {
		.name = "display",
		/*
		 * Pipe A power well is the new disp2d well. Pipe B and C
		 * power wells don't actually exist. Pipe A power well is
		 * required for any pipe to work.
		 */
		.domains = CHV_DISPLAY_POWER_DOMAINS,
		.ops = &chv_pipe_power_well_ops,
		.id = DISP_PW_ID_NONE,
	}, {
		.name = "dpio-common-bc",
		.domains = CHV_DPIO_CMN_BC_POWER_DOMAINS,
		.ops = &chv_dpio_cmn_power_well_ops,
		.id = VLV_DISP_PW_DPIO_CMN_BC,
		{
			.vlv.idx = PUNIT_PWGT_IDX_DPIO_CMN_BC,
		},
	}, {
		.name = "dpio-common-d",
		.domains = CHV_DPIO_CMN_D_POWER_DOMAINS,
		.ops = &chv_dpio_cmn_power_well_ops,
		.id = CHV_DISP_PW_DPIO_CMN_D,
		{
			.vlv.idx = PUNIT_PWGT_IDX_DPIO_CMN_D,
		},
	},
};

#define SKL_DISPLAY_POWERWELL_2_POWER_DOMAINS (		\
	BIT_ULL(POWER_DOMAIN_PIPE_B) |			\
	BIT_ULL(POWER_DOMAIN_PIPE_C) |			\
	BIT_ULL(POWER_DOMAIN_PIPE_PANEL_FITTER_B) |		\
	BIT_ULL(POWER_DOMAIN_PIPE_PANEL_FITTER_C) |		\
	BIT_ULL(POWER_DOMAIN_TRANSCODER_A) |		\
	BIT_ULL(POWER_DOMAIN_TRANSCODER_B) |		\
	BIT_ULL(POWER_DOMAIN_TRANSCODER_C) |		\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_B) |		\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_C) |		\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_D) |		\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_E) |		\
	BIT_ULL(POWER_DOMAIN_VGA) |				\
	BIT_ULL(POWER_DOMAIN_AUDIO_MMIO) |		\
	BIT_ULL(POWER_DOMAIN_AUDIO_PLAYBACK) |			\
	BIT_ULL(POWER_DOMAIN_AUX_B) |                       \
	BIT_ULL(POWER_DOMAIN_AUX_C) |			\
	BIT_ULL(POWER_DOMAIN_AUX_D) |			\
	BIT_ULL(POWER_DOMAIN_INIT))

#define SKL_DISPLAY_DC_OFF_POWER_DOMAINS (		\
	SKL_DISPLAY_POWERWELL_2_POWER_DOMAINS |		\
	BIT_ULL(POWER_DOMAIN_AUX_A) |			\
	BIT_ULL(POWER_DOMAIN_MODESET) |			\
	BIT_ULL(POWER_DOMAIN_GT_IRQ) |			\
	BIT_ULL(POWER_DOMAIN_INIT))

#define SKL_DISPLAY_DDI_IO_A_E_POWER_DOMAINS (		\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_IO_A) |		\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_IO_E) |		\
	BIT_ULL(POWER_DOMAIN_INIT))

#define SKL_DISPLAY_DDI_IO_B_POWER_DOMAINS (		\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_IO_B) |		\
	BIT_ULL(POWER_DOMAIN_INIT))

#define SKL_DISPLAY_DDI_IO_C_POWER_DOMAINS (		\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_IO_C) |		\
	BIT_ULL(POWER_DOMAIN_INIT))

#define SKL_DISPLAY_DDI_IO_D_POWER_DOMAINS (		\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_IO_D) |		\
	BIT_ULL(POWER_DOMAIN_INIT))

static const struct i915_power_well_desc skl_power_wells[] = {
	{
		.name = "always-on",
		.domains = POWER_DOMAIN_MASK,
		.ops = &i9xx_always_on_power_well_ops,
		.always_on = true,
		.id = DISP_PW_ID_NONE,
	}, {
		.name = "PW_1",
		/* Handled by the DMC firmware */
		.domains = 0,
		.ops = &hsw_power_well_ops,
		.always_on = true,
		.has_fuses = true,
		.id = SKL_DISP_PW_1,
		{
			.hsw.idx = SKL_PW_CTL_IDX_PW_1,
		},
	}, {
		.name = "MISC_IO",
		/* Handled by the DMC firmware */
		.domains = 0,
		.ops = &hsw_power_well_ops,
		.always_on = true,
		.id = SKL_DISP_PW_MISC_IO,
		{
			.hsw.idx = SKL_PW_CTL_IDX_MISC_IO,
		},
	}, {
		.name = "DC_off",
		.domains = SKL_DISPLAY_DC_OFF_POWER_DOMAINS,
		.ops = &gen9_dc_off_power_well_ops,
		.id = SKL_DISP_DC_OFF,
	}, {
		.name = "PW_2",
		.domains = SKL_DISPLAY_POWERWELL_2_POWER_DOMAINS,
		.ops = &hsw_power_well_ops,
		.has_vga = true,
		.irq_pipe_mask = BIT(PIPE_B) | BIT(PIPE_C),
		.has_fuses = true,
		.id = SKL_DISP_PW_2,
		{
			.hsw.idx = SKL_PW_CTL_IDX_PW_2,
		},
	}, {
		.name = "DDI_IO_A_E",
		.domains = SKL_DISPLAY_DDI_IO_A_E_POWER_DOMAINS,
		.ops = &hsw_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = SKL_PW_CTL_IDX_DDI_A_E,
		},
	}, {
		.name = "DDI_IO_B",
		.domains = SKL_DISPLAY_DDI_IO_B_POWER_DOMAINS,
		.ops = &hsw_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = SKL_PW_CTL_IDX_DDI_B,
		},
	}, {
		.name = "DDI_IO_C",
		.domains = SKL_DISPLAY_DDI_IO_C_POWER_DOMAINS,
		.ops = &hsw_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = SKL_PW_CTL_IDX_DDI_C,
		},
	}, {
		.name = "DDI_IO_D",
		.domains = SKL_DISPLAY_DDI_IO_D_POWER_DOMAINS,
		.ops = &hsw_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = SKL_PW_CTL_IDX_DDI_D,
		},
	},
};

#define BXT_DISPLAY_POWERWELL_2_POWER_DOMAINS (		\
	BIT_ULL(POWER_DOMAIN_PIPE_B) |			\
	BIT_ULL(POWER_DOMAIN_PIPE_C) |			\
	BIT_ULL(POWER_DOMAIN_PIPE_PANEL_FITTER_B) |		\
	BIT_ULL(POWER_DOMAIN_PIPE_PANEL_FITTER_C) |		\
	BIT_ULL(POWER_DOMAIN_TRANSCODER_A) |		\
	BIT_ULL(POWER_DOMAIN_TRANSCODER_B) |		\
	BIT_ULL(POWER_DOMAIN_TRANSCODER_C) |		\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_B) |		\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_C) |		\
	BIT_ULL(POWER_DOMAIN_VGA) |				\
	BIT_ULL(POWER_DOMAIN_AUDIO_MMIO) |		\
	BIT_ULL(POWER_DOMAIN_AUDIO_PLAYBACK) |			\
	BIT_ULL(POWER_DOMAIN_AUX_B) |			\
	BIT_ULL(POWER_DOMAIN_AUX_C) |			\
	BIT_ULL(POWER_DOMAIN_INIT))

#define BXT_DISPLAY_DC_OFF_POWER_DOMAINS (		\
	BXT_DISPLAY_POWERWELL_2_POWER_DOMAINS |		\
	BIT_ULL(POWER_DOMAIN_AUX_A) |			\
	BIT_ULL(POWER_DOMAIN_GMBUS) |			\
	BIT_ULL(POWER_DOMAIN_MODESET) |			\
	BIT_ULL(POWER_DOMAIN_GT_IRQ) |			\
	BIT_ULL(POWER_DOMAIN_INIT))

#define BXT_DPIO_CMN_A_POWER_DOMAINS (			\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_A) |		\
	BIT_ULL(POWER_DOMAIN_AUX_A) |			\
	BIT_ULL(POWER_DOMAIN_INIT))

#define BXT_DPIO_CMN_BC_POWER_DOMAINS (			\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_B) |		\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_C) |		\
	BIT_ULL(POWER_DOMAIN_AUX_B) |			\
	BIT_ULL(POWER_DOMAIN_AUX_C) |			\
	BIT_ULL(POWER_DOMAIN_INIT))

static const struct i915_power_well_desc bxt_power_wells[] = {
	{
		.name = "always-on",
		.domains = POWER_DOMAIN_MASK,
		.ops = &i9xx_always_on_power_well_ops,
		.always_on = true,
		.id = DISP_PW_ID_NONE,
	}, {
		.name = "PW_1",
		/* Handled by the DMC firmware */
		.domains = 0,
		.ops = &hsw_power_well_ops,
		.always_on = true,
		.has_fuses = true,
		.id = SKL_DISP_PW_1,
		{
			.hsw.idx = SKL_PW_CTL_IDX_PW_1,
		},
	}, {
		.name = "DC_off",
		.domains = BXT_DISPLAY_DC_OFF_POWER_DOMAINS,
		.ops = &gen9_dc_off_power_well_ops,
		.id = SKL_DISP_DC_OFF,
	}, {
		.name = "PW_2",
		.domains = BXT_DISPLAY_POWERWELL_2_POWER_DOMAINS,
		.ops = &hsw_power_well_ops,
		.has_vga = true,
		.irq_pipe_mask = BIT(PIPE_B) | BIT(PIPE_C),
		.has_fuses = true,
		.id = SKL_DISP_PW_2,
		{
			.hsw.idx = SKL_PW_CTL_IDX_PW_2,
		},
	}, {
		.name = "dpio-common-a",
		.domains = BXT_DPIO_CMN_A_POWER_DOMAINS,
		.ops = &bxt_dpio_cmn_power_well_ops,
		.id = BXT_DISP_PW_DPIO_CMN_A,
		{
			.bxt.phy = DPIO_PHY1,
		},
	}, {
		.name = "dpio-common-bc",
		.domains = BXT_DPIO_CMN_BC_POWER_DOMAINS,
		.ops = &bxt_dpio_cmn_power_well_ops,
		.id = VLV_DISP_PW_DPIO_CMN_BC,
		{
			.bxt.phy = DPIO_PHY0,
		},
	},
};

#define GLK_DISPLAY_POWERWELL_2_POWER_DOMAINS (		\
	BIT_ULL(POWER_DOMAIN_PIPE_B) |			\
	BIT_ULL(POWER_DOMAIN_PIPE_C) |			\
	BIT_ULL(POWER_DOMAIN_PIPE_PANEL_FITTER_B) |		\
	BIT_ULL(POWER_DOMAIN_PIPE_PANEL_FITTER_C) |		\
	BIT_ULL(POWER_DOMAIN_TRANSCODER_A) |		\
	BIT_ULL(POWER_DOMAIN_TRANSCODER_B) |		\
	BIT_ULL(POWER_DOMAIN_TRANSCODER_C) |		\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_B) |		\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_C) |		\
	BIT_ULL(POWER_DOMAIN_VGA) |				\
	BIT_ULL(POWER_DOMAIN_AUDIO_MMIO) |		\
	BIT_ULL(POWER_DOMAIN_AUDIO_PLAYBACK) |			\
	BIT_ULL(POWER_DOMAIN_AUX_B) |                       \
	BIT_ULL(POWER_DOMAIN_AUX_C) |			\
	BIT_ULL(POWER_DOMAIN_INIT))

#define GLK_DISPLAY_DC_OFF_POWER_DOMAINS (		\
	GLK_DISPLAY_POWERWELL_2_POWER_DOMAINS |		\
	BIT_ULL(POWER_DOMAIN_AUX_A) |			\
	BIT_ULL(POWER_DOMAIN_GMBUS) |			\
	BIT_ULL(POWER_DOMAIN_MODESET) |			\
	BIT_ULL(POWER_DOMAIN_GT_IRQ) |			\
	BIT_ULL(POWER_DOMAIN_INIT))

#define GLK_DISPLAY_DDI_IO_A_POWER_DOMAINS	BIT_ULL(POWER_DOMAIN_PORT_DDI_IO_A)
#define GLK_DISPLAY_DDI_IO_B_POWER_DOMAINS	BIT_ULL(POWER_DOMAIN_PORT_DDI_IO_B)
#define GLK_DISPLAY_DDI_IO_C_POWER_DOMAINS	BIT_ULL(POWER_DOMAIN_PORT_DDI_IO_C)

#define GLK_DPIO_CMN_A_POWER_DOMAINS (			\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_A) |		\
	BIT_ULL(POWER_DOMAIN_AUX_A) |			\
	BIT_ULL(POWER_DOMAIN_INIT))

#define GLK_DPIO_CMN_B_POWER_DOMAINS (			\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_B) |		\
	BIT_ULL(POWER_DOMAIN_AUX_B) |			\
	BIT_ULL(POWER_DOMAIN_INIT))

#define GLK_DPIO_CMN_C_POWER_DOMAINS (			\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_C) |		\
	BIT_ULL(POWER_DOMAIN_AUX_C) |			\
	BIT_ULL(POWER_DOMAIN_INIT))

#define GLK_DISPLAY_AUX_A_POWER_DOMAINS (		\
	BIT_ULL(POWER_DOMAIN_AUX_A) |		\
	BIT_ULL(POWER_DOMAIN_AUX_IO_A) |		\
	BIT_ULL(POWER_DOMAIN_INIT))

#define GLK_DISPLAY_AUX_B_POWER_DOMAINS (		\
	BIT_ULL(POWER_DOMAIN_AUX_B) |		\
	BIT_ULL(POWER_DOMAIN_INIT))

#define GLK_DISPLAY_AUX_C_POWER_DOMAINS (		\
	BIT_ULL(POWER_DOMAIN_AUX_C) |		\
	BIT_ULL(POWER_DOMAIN_INIT))

static const struct i915_power_well_desc glk_power_wells[] = {
	{
		.name = "always-on",
		.domains = POWER_DOMAIN_MASK,
		.ops = &i9xx_always_on_power_well_ops,
		.always_on = true,
		.id = DISP_PW_ID_NONE,
	}, {
		.name = "PW_1",
		/* Handled by the DMC firmware */
		.domains = 0,
		.ops = &hsw_power_well_ops,
		.always_on = true,
		.has_fuses = true,
		.id = SKL_DISP_PW_1,
		{
			.hsw.idx = SKL_PW_CTL_IDX_PW_1,
		},
	}, {
		.name = "DC_off",
		.domains = GLK_DISPLAY_DC_OFF_POWER_DOMAINS,
		.ops = &gen9_dc_off_power_well_ops,
		.id = SKL_DISP_DC_OFF,
	}, {
		.name = "PW_2",
		.domains = GLK_DISPLAY_POWERWELL_2_POWER_DOMAINS,
		.ops = &hsw_power_well_ops,
		.has_vga = true,
		.irq_pipe_mask = BIT(PIPE_B) | BIT(PIPE_C),
		.has_fuses = true,
		.id = SKL_DISP_PW_2,
		{
			.hsw.idx = SKL_PW_CTL_IDX_PW_2,
		},
	}, {
		.name = "dpio-common-a",
		.domains = GLK_DPIO_CMN_A_POWER_DOMAINS,
		.ops = &bxt_dpio_cmn_power_well_ops,
		.id = BXT_DISP_PW_DPIO_CMN_A,
		{
			.bxt.phy = DPIO_PHY1,
		},
	}, {
		.name = "dpio-common-b",
		.domains = GLK_DPIO_CMN_B_POWER_DOMAINS,
		.ops = &bxt_dpio_cmn_power_well_ops,
		.id = VLV_DISP_PW_DPIO_CMN_BC,
		{
			.bxt.phy = DPIO_PHY0,
		},
	}, {
		.name = "dpio-common-c",
		.domains = GLK_DPIO_CMN_C_POWER_DOMAINS,
		.ops = &bxt_dpio_cmn_power_well_ops,
		.id = GLK_DISP_PW_DPIO_CMN_C,
		{
			.bxt.phy = DPIO_PHY2,
		},
	}, {
		.name = "AUX_A",
		.domains = GLK_DISPLAY_AUX_A_POWER_DOMAINS,
		.ops = &hsw_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = GLK_PW_CTL_IDX_AUX_A,
		},
	}, {
		.name = "AUX_B",
		.domains = GLK_DISPLAY_AUX_B_POWER_DOMAINS,
		.ops = &hsw_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = GLK_PW_CTL_IDX_AUX_B,
		},
	}, {
		.name = "AUX_C",
		.domains = GLK_DISPLAY_AUX_C_POWER_DOMAINS,
		.ops = &hsw_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = GLK_PW_CTL_IDX_AUX_C,
		},
	}, {
		.name = "DDI_IO_A",
		.domains = GLK_DISPLAY_DDI_IO_A_POWER_DOMAINS,
		.ops = &hsw_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = GLK_PW_CTL_IDX_DDI_A,
		},
	}, {
		.name = "DDI_IO_B",
		.domains = GLK_DISPLAY_DDI_IO_B_POWER_DOMAINS,
		.ops = &hsw_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = SKL_PW_CTL_IDX_DDI_B,
		},
	}, {
		.name = "DDI_IO_C",
		.domains = GLK_DISPLAY_DDI_IO_C_POWER_DOMAINS,
		.ops = &hsw_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = SKL_PW_CTL_IDX_DDI_C,
		},
	},
};

/*
 * ICL PW_0/PG_0 domains (HW/DMC control):
 * - PCI
 * - clocks except port PLL
 * - central power except FBC
 * - shared functions except pipe interrupts, pipe MBUS, DBUF registers
 * ICL PW_1/PG_1 domains (HW/DMC control):
 * - DBUF function
 * - PIPE_A and its planes, except VGA
 * - transcoder EDP + PSR
 * - transcoder DSI
 * - DDI_A
 * - FBC
 */
#define ICL_PW_4_POWER_DOMAINS (			\
	BIT_ULL(POWER_DOMAIN_PIPE_C) |			\
	BIT_ULL(POWER_DOMAIN_PIPE_PANEL_FITTER_C) |	\
	BIT_ULL(POWER_DOMAIN_INIT))
	/* VDSC/joining */

#define ICL_PW_3_POWER_DOMAINS (			\
	ICL_PW_4_POWER_DOMAINS |			\
	BIT_ULL(POWER_DOMAIN_PIPE_B) |			\
	BIT_ULL(POWER_DOMAIN_PIPE_PANEL_FITTER_B) |	\
	BIT_ULL(POWER_DOMAIN_TRANSCODER_A) |		\
	BIT_ULL(POWER_DOMAIN_TRANSCODER_B) |		\
	BIT_ULL(POWER_DOMAIN_TRANSCODER_C) |		\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_B) |	\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_C) |	\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_D) |	\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_E) |	\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_F) |	\
	BIT_ULL(POWER_DOMAIN_VGA) |			\
	BIT_ULL(POWER_DOMAIN_AUDIO_MMIO) |		\
	BIT_ULL(POWER_DOMAIN_AUDIO_PLAYBACK) |		\
	BIT_ULL(POWER_DOMAIN_AUX_B) |			\
	BIT_ULL(POWER_DOMAIN_AUX_C) |			\
	BIT_ULL(POWER_DOMAIN_AUX_D) |			\
	BIT_ULL(POWER_DOMAIN_AUX_E) |			\
	BIT_ULL(POWER_DOMAIN_AUX_F) |			\
	BIT_ULL(POWER_DOMAIN_AUX_TBT_C) |		\
	BIT_ULL(POWER_DOMAIN_AUX_TBT_D) |		\
	BIT_ULL(POWER_DOMAIN_AUX_TBT_E) |		\
	BIT_ULL(POWER_DOMAIN_AUX_TBT_F) |		\
	BIT_ULL(POWER_DOMAIN_INIT))
	/*
	 * - transcoder WD
	 * - KVMR (HW control)
	 */

#define ICL_PW_2_POWER_DOMAINS (			\
	ICL_PW_3_POWER_DOMAINS |			\
	BIT_ULL(POWER_DOMAIN_TRANSCODER_VDSC_PW2) |		\
	BIT_ULL(POWER_DOMAIN_INIT))
	/*
	 * - KVMR (HW control)
	 */

#define ICL_DISPLAY_DC_OFF_POWER_DOMAINS (		\
	ICL_PW_2_POWER_DOMAINS |			\
	BIT_ULL(POWER_DOMAIN_AUX_A) |			\
	BIT_ULL(POWER_DOMAIN_MODESET) |			\
	BIT_ULL(POWER_DOMAIN_DC_OFF) |			\
	BIT_ULL(POWER_DOMAIN_INIT))

#define ICL_DDI_IO_A_POWER_DOMAINS		BIT_ULL(POWER_DOMAIN_PORT_DDI_IO_A)
#define ICL_DDI_IO_B_POWER_DOMAINS		BIT_ULL(POWER_DOMAIN_PORT_DDI_IO_B)
#define ICL_DDI_IO_C_POWER_DOMAINS		BIT_ULL(POWER_DOMAIN_PORT_DDI_IO_C)
#define ICL_DDI_IO_D_POWER_DOMAINS		BIT_ULL(POWER_DOMAIN_PORT_DDI_IO_D)
#define ICL_DDI_IO_E_POWER_DOMAINS		BIT_ULL(POWER_DOMAIN_PORT_DDI_IO_E)
#define ICL_DDI_IO_F_POWER_DOMAINS		BIT_ULL(POWER_DOMAIN_PORT_DDI_IO_F)

#define ICL_AUX_A_IO_POWER_DOMAINS (			\
	BIT_ULL(POWER_DOMAIN_AUX_A) |			\
	BIT_ULL(POWER_DOMAIN_AUX_IO_A))

#define ICL_AUX_B_IO_POWER_DOMAINS		BIT_ULL(POWER_DOMAIN_AUX_B)
#define ICL_AUX_C_TC1_IO_POWER_DOMAINS		BIT_ULL(POWER_DOMAIN_AUX_C)
#define ICL_AUX_D_TC2_IO_POWER_DOMAINS		BIT_ULL(POWER_DOMAIN_AUX_D)
#define ICL_AUX_E_TC3_IO_POWER_DOMAINS		BIT_ULL(POWER_DOMAIN_AUX_E)
#define ICL_AUX_F_TC4_IO_POWER_DOMAINS		BIT_ULL(POWER_DOMAIN_AUX_F)
#define ICL_AUX_C_TBT1_IO_POWER_DOMAINS		BIT_ULL(POWER_DOMAIN_AUX_TBT_C)
#define ICL_AUX_D_TBT2_IO_POWER_DOMAINS		BIT_ULL(POWER_DOMAIN_AUX_TBT_D)
#define ICL_AUX_E_TBT3_IO_POWER_DOMAINS		BIT_ULL(POWER_DOMAIN_AUX_TBT_E)
#define ICL_AUX_F_TBT4_IO_POWER_DOMAINS		BIT_ULL(POWER_DOMAIN_AUX_TBT_F)

static const struct i915_power_well_desc icl_power_wells[] = {
	{
		.name = "always-on",
		.domains = POWER_DOMAIN_MASK,
		.ops = &i9xx_always_on_power_well_ops,
		.always_on = true,
		.id = DISP_PW_ID_NONE,
	}, {
		.name = "PW_1",
		/* Handled by the DMC firmware */
		.domains = 0,
		.ops = &hsw_power_well_ops,
		.always_on = true,
		.has_fuses = true,
		.id = SKL_DISP_PW_1,
		{
			.hsw.idx = ICL_PW_CTL_IDX_PW_1,
		},
	}, {
		.name = "DC_off",
		.domains = ICL_DISPLAY_DC_OFF_POWER_DOMAINS,
		.ops = &gen9_dc_off_power_well_ops,
		.id = SKL_DISP_DC_OFF,
	}, {
		.name = "PW_2",
		.domains = ICL_PW_2_POWER_DOMAINS,
		.ops = &hsw_power_well_ops,
		.has_fuses = true,
		.id = SKL_DISP_PW_2,
		{
			.hsw.idx = ICL_PW_CTL_IDX_PW_2,
		},
	}, {
		.name = "PW_3",
		.domains = ICL_PW_3_POWER_DOMAINS,
		.ops = &hsw_power_well_ops,
		.has_vga = true,
		.irq_pipe_mask = BIT(PIPE_B),
		.has_fuses = true,
		.id = ICL_DISP_PW_3,
		{
			.hsw.idx = ICL_PW_CTL_IDX_PW_3,
		},
	}, {
		.name = "DDI_IO_A",
		.domains = ICL_DDI_IO_A_POWER_DOMAINS,
		.ops = &icl_ddi_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = ICL_PW_CTL_IDX_DDI_A,
		},
	}, {
		.name = "DDI_IO_B",
		.domains = ICL_DDI_IO_B_POWER_DOMAINS,
		.ops = &icl_ddi_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = ICL_PW_CTL_IDX_DDI_B,
		},
	}, {
		.name = "DDI_IO_C",
		.domains = ICL_DDI_IO_C_POWER_DOMAINS,
		.ops = &icl_ddi_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = ICL_PW_CTL_IDX_DDI_C,
		},
	}, {
		.name = "DDI_IO_D",
		.domains = ICL_DDI_IO_D_POWER_DOMAINS,
		.ops = &icl_ddi_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = ICL_PW_CTL_IDX_DDI_D,
		},
	}, {
		.name = "DDI_IO_E",
		.domains = ICL_DDI_IO_E_POWER_DOMAINS,
		.ops = &icl_ddi_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = ICL_PW_CTL_IDX_DDI_E,
		},
	}, {
		.name = "DDI_IO_F",
		.domains = ICL_DDI_IO_F_POWER_DOMAINS,
		.ops = &icl_ddi_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = ICL_PW_CTL_IDX_DDI_F,
		},
	}, {
		.name = "AUX_A",
		.domains = ICL_AUX_A_IO_POWER_DOMAINS,
		.ops = &icl_aux_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = ICL_PW_CTL_IDX_AUX_A,
		},
	}, {
		.name = "AUX_B",
		.domains = ICL_AUX_B_IO_POWER_DOMAINS,
		.ops = &icl_aux_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = ICL_PW_CTL_IDX_AUX_B,
		},
	}, {
		.name = "AUX_C",
		.domains = ICL_AUX_C_TC1_IO_POWER_DOMAINS,
		.ops = &icl_aux_power_well_ops,
		.is_tc_tbt = false,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = ICL_PW_CTL_IDX_AUX_C,
		},
	}, {
		.name = "AUX_D",
		.domains = ICL_AUX_D_TC2_IO_POWER_DOMAINS,
		.ops = &icl_aux_power_well_ops,
		.is_tc_tbt = false,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = ICL_PW_CTL_IDX_AUX_D,
		},
	}, {
		.name = "AUX_E",
		.domains = ICL_AUX_E_TC3_IO_POWER_DOMAINS,
		.ops = &icl_aux_power_well_ops,
		.is_tc_tbt = false,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = ICL_PW_CTL_IDX_AUX_E,
		},
	}, {
		.name = "AUX_F",
		.domains = ICL_AUX_F_TC4_IO_POWER_DOMAINS,
		.ops = &icl_aux_power_well_ops,
		.is_tc_tbt = false,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = ICL_PW_CTL_IDX_AUX_F,
		},
	}, {
		.name = "AUX_TBT1",
		.domains = ICL_AUX_C_TBT1_IO_POWER_DOMAINS,
		.ops = &icl_aux_power_well_ops,
		.is_tc_tbt = true,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = ICL_PW_CTL_IDX_AUX_TBT1,
		},
	}, {
		.name = "AUX_TBT2",
		.domains = ICL_AUX_D_TBT2_IO_POWER_DOMAINS,
		.ops = &icl_aux_power_well_ops,
		.is_tc_tbt = true,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = ICL_PW_CTL_IDX_AUX_TBT2,
		},
	}, {
		.name = "AUX_TBT3",
		.domains = ICL_AUX_E_TBT3_IO_POWER_DOMAINS,
		.ops = &icl_aux_power_well_ops,
		.is_tc_tbt = true,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = ICL_PW_CTL_IDX_AUX_TBT3,
		},
	}, {
		.name = "AUX_TBT4",
		.domains = ICL_AUX_F_TBT4_IO_POWER_DOMAINS,
		.ops = &icl_aux_power_well_ops,
		.is_tc_tbt = true,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = ICL_PW_CTL_IDX_AUX_TBT4,
		},
	}, {
		.name = "PW_4",
		.domains = ICL_PW_4_POWER_DOMAINS,
		.ops = &hsw_power_well_ops,
		.irq_pipe_mask = BIT(PIPE_C),
		.has_fuses = true,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = ICL_PW_CTL_IDX_PW_4,
		},
	},
};

#define TGL_PW_5_POWER_DOMAINS (			\
	BIT_ULL(POWER_DOMAIN_PIPE_D) |			\
	BIT_ULL(POWER_DOMAIN_PIPE_PANEL_FITTER_D) |     \
	BIT_ULL(POWER_DOMAIN_TRANSCODER_D) |		\
	BIT_ULL(POWER_DOMAIN_INIT))

#define TGL_PW_4_POWER_DOMAINS (			\
	TGL_PW_5_POWER_DOMAINS |			\
	BIT_ULL(POWER_DOMAIN_PIPE_C) |			\
	BIT_ULL(POWER_DOMAIN_PIPE_PANEL_FITTER_C) |	\
	BIT_ULL(POWER_DOMAIN_TRANSCODER_C) |		\
	BIT_ULL(POWER_DOMAIN_INIT))

#define TGL_PW_3_POWER_DOMAINS (			\
	TGL_PW_4_POWER_DOMAINS |			\
	BIT_ULL(POWER_DOMAIN_PIPE_B) |			\
	BIT_ULL(POWER_DOMAIN_PIPE_PANEL_FITTER_B) |	\
	BIT_ULL(POWER_DOMAIN_TRANSCODER_B) |		\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_TC1) |	\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_TC2) |	\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_TC3) |	\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_TC4) |	\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_TC5) |	\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_TC6) |	\
	BIT_ULL(POWER_DOMAIN_VGA) |			\
	BIT_ULL(POWER_DOMAIN_AUDIO_MMIO) |		\
	BIT_ULL(POWER_DOMAIN_AUDIO_PLAYBACK) |		\
	BIT_ULL(POWER_DOMAIN_AUX_USBC1) |		\
	BIT_ULL(POWER_DOMAIN_AUX_USBC2) |		\
	BIT_ULL(POWER_DOMAIN_AUX_USBC3) |		\
	BIT_ULL(POWER_DOMAIN_AUX_USBC4) |		\
	BIT_ULL(POWER_DOMAIN_AUX_USBC5) |		\
	BIT_ULL(POWER_DOMAIN_AUX_USBC6) |		\
	BIT_ULL(POWER_DOMAIN_AUX_TBT1) |		\
	BIT_ULL(POWER_DOMAIN_AUX_TBT2) |		\
	BIT_ULL(POWER_DOMAIN_AUX_TBT3) |		\
	BIT_ULL(POWER_DOMAIN_AUX_TBT4) |		\
	BIT_ULL(POWER_DOMAIN_AUX_TBT5) |		\
	BIT_ULL(POWER_DOMAIN_AUX_TBT6) |		\
	BIT_ULL(POWER_DOMAIN_INIT))

#define TGL_PW_2_POWER_DOMAINS (			\
	TGL_PW_3_POWER_DOMAINS |			\
	BIT_ULL(POWER_DOMAIN_TRANSCODER_VDSC_PW2) |	\
	BIT_ULL(POWER_DOMAIN_INIT))

#define TGL_DISPLAY_DC_OFF_POWER_DOMAINS (		\
	TGL_PW_3_POWER_DOMAINS |			\
	BIT_ULL(POWER_DOMAIN_AUX_A) |			\
	BIT_ULL(POWER_DOMAIN_AUX_B) |			\
	BIT_ULL(POWER_DOMAIN_AUX_C) |			\
	BIT_ULL(POWER_DOMAIN_MODESET) |			\
	BIT_ULL(POWER_DOMAIN_INIT))

#define TGL_DDI_IO_TC1_POWER_DOMAINS	BIT_ULL(POWER_DOMAIN_PORT_DDI_IO_TC1)
#define TGL_DDI_IO_TC2_POWER_DOMAINS	BIT_ULL(POWER_DOMAIN_PORT_DDI_IO_TC2)
#define TGL_DDI_IO_TC3_POWER_DOMAINS	BIT_ULL(POWER_DOMAIN_PORT_DDI_IO_TC3)
#define TGL_DDI_IO_TC4_POWER_DOMAINS	BIT_ULL(POWER_DOMAIN_PORT_DDI_IO_TC4)
#define TGL_DDI_IO_TC5_POWER_DOMAINS	BIT_ULL(POWER_DOMAIN_PORT_DDI_IO_TC5)
#define TGL_DDI_IO_TC6_POWER_DOMAINS	BIT_ULL(POWER_DOMAIN_PORT_DDI_IO_TC6)

#define TGL_AUX_A_IO_POWER_DOMAINS (		\
	BIT_ULL(POWER_DOMAIN_AUX_A) |		\
	BIT_ULL(POWER_DOMAIN_AUX_IO_A))
#define TGL_AUX_B_IO_POWER_DOMAINS	BIT_ULL(POWER_DOMAIN_AUX_B)
#define TGL_AUX_C_IO_POWER_DOMAINS	BIT_ULL(POWER_DOMAIN_AUX_C)

#define TGL_AUX_IO_USBC1_POWER_DOMAINS	BIT_ULL(POWER_DOMAIN_AUX_USBC1)
#define TGL_AUX_IO_USBC2_POWER_DOMAINS	BIT_ULL(POWER_DOMAIN_AUX_USBC2)
#define TGL_AUX_IO_USBC3_POWER_DOMAINS	BIT_ULL(POWER_DOMAIN_AUX_USBC3)
#define TGL_AUX_IO_USBC4_POWER_DOMAINS	BIT_ULL(POWER_DOMAIN_AUX_USBC4)
#define TGL_AUX_IO_USBC5_POWER_DOMAINS	BIT_ULL(POWER_DOMAIN_AUX_USBC5)
#define TGL_AUX_IO_USBC6_POWER_DOMAINS	BIT_ULL(POWER_DOMAIN_AUX_USBC6)

#define TGL_AUX_IO_TBT1_POWER_DOMAINS	BIT_ULL(POWER_DOMAIN_AUX_TBT1)
#define TGL_AUX_IO_TBT2_POWER_DOMAINS	BIT_ULL(POWER_DOMAIN_AUX_TBT2)
#define TGL_AUX_IO_TBT3_POWER_DOMAINS	BIT_ULL(POWER_DOMAIN_AUX_TBT3)
#define TGL_AUX_IO_TBT4_POWER_DOMAINS	BIT_ULL(POWER_DOMAIN_AUX_TBT4)
#define TGL_AUX_IO_TBT5_POWER_DOMAINS	BIT_ULL(POWER_DOMAIN_AUX_TBT5)
#define TGL_AUX_IO_TBT6_POWER_DOMAINS	BIT_ULL(POWER_DOMAIN_AUX_TBT6)

#define TGL_TC_COLD_OFF_POWER_DOMAINS (		\
	BIT_ULL(POWER_DOMAIN_AUX_USBC1)	|	\
	BIT_ULL(POWER_DOMAIN_AUX_USBC2)	|	\
	BIT_ULL(POWER_DOMAIN_AUX_USBC3)	|	\
	BIT_ULL(POWER_DOMAIN_AUX_USBC4)	|	\
	BIT_ULL(POWER_DOMAIN_AUX_USBC5)	|	\
	BIT_ULL(POWER_DOMAIN_AUX_USBC6)	|	\
	BIT_ULL(POWER_DOMAIN_AUX_TBT1) |	\
	BIT_ULL(POWER_DOMAIN_AUX_TBT2) |	\
	BIT_ULL(POWER_DOMAIN_AUX_TBT3) |	\
	BIT_ULL(POWER_DOMAIN_AUX_TBT4) |	\
	BIT_ULL(POWER_DOMAIN_AUX_TBT5) |	\
	BIT_ULL(POWER_DOMAIN_AUX_TBT6) |	\
	BIT_ULL(POWER_DOMAIN_TC_COLD_OFF))

static const struct i915_power_well_desc tgl_power_wells[] = {
	{
		.name = "always-on",
		.domains = POWER_DOMAIN_MASK,
		.ops = &i9xx_always_on_power_well_ops,
		.always_on = true,
		.id = DISP_PW_ID_NONE,
	}, {
		.name = "PW_1",
		/* Handled by the DMC firmware */
		.domains = 0,
		.ops = &hsw_power_well_ops,
		.always_on = true,
		.has_fuses = true,
		.id = SKL_DISP_PW_1,
		{
			.hsw.idx = ICL_PW_CTL_IDX_PW_1,
		},
	}, {
		.name = "DC_off",
		.domains = TGL_DISPLAY_DC_OFF_POWER_DOMAINS,
		.ops = &gen9_dc_off_power_well_ops,
		.id = SKL_DISP_DC_OFF,
	}, {
		.name = "PW_2",
		.domains = TGL_PW_2_POWER_DOMAINS,
		.ops = &hsw_power_well_ops,
		.has_fuses = true,
		.id = SKL_DISP_PW_2,
		{
			.hsw.idx = ICL_PW_CTL_IDX_PW_2,
		},
	}, {
		.name = "PW_3",
		.domains = TGL_PW_3_POWER_DOMAINS,
		.ops = &hsw_power_well_ops,
		.has_vga = true,
		.irq_pipe_mask = BIT(PIPE_B),
		.has_fuses = true,
		.id = ICL_DISP_PW_3,
		{
			.hsw.idx = ICL_PW_CTL_IDX_PW_3,
		},
	}, {
		.name = "DDI_IO_A",
		.domains = ICL_DDI_IO_A_POWER_DOMAINS,
		.ops = &icl_ddi_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = ICL_PW_CTL_IDX_DDI_A,
		}
	}, {
		.name = "DDI_IO_B",
		.domains = ICL_DDI_IO_B_POWER_DOMAINS,
		.ops = &icl_ddi_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = ICL_PW_CTL_IDX_DDI_B,
		}
	}, {
		.name = "DDI_IO_C",
		.domains = ICL_DDI_IO_C_POWER_DOMAINS,
		.ops = &icl_ddi_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = ICL_PW_CTL_IDX_DDI_C,
		}
	}, {
		.name = "DDI_IO_TC1",
		.domains = TGL_DDI_IO_TC1_POWER_DOMAINS,
		.ops = &icl_ddi_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = TGL_PW_CTL_IDX_DDI_TC1,
		},
	}, {
		.name = "DDI_IO_TC2",
		.domains = TGL_DDI_IO_TC2_POWER_DOMAINS,
		.ops = &icl_ddi_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = TGL_PW_CTL_IDX_DDI_TC2,
		},
	}, {
		.name = "DDI_IO_TC3",
		.domains = TGL_DDI_IO_TC3_POWER_DOMAINS,
		.ops = &icl_ddi_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = TGL_PW_CTL_IDX_DDI_TC3,
		},
	}, {
		.name = "DDI_IO_TC4",
		.domains = TGL_DDI_IO_TC4_POWER_DOMAINS,
		.ops = &icl_ddi_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = TGL_PW_CTL_IDX_DDI_TC4,
		},
	}, {
		.name = "DDI_IO_TC5",
		.domains = TGL_DDI_IO_TC5_POWER_DOMAINS,
		.ops = &icl_ddi_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = TGL_PW_CTL_IDX_DDI_TC5,
		},
	}, {
		.name = "DDI_IO_TC6",
		.domains = TGL_DDI_IO_TC6_POWER_DOMAINS,
		.ops = &icl_ddi_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = TGL_PW_CTL_IDX_DDI_TC6,
		},
	}, {
		.name = "TC_cold_off",
		.domains = TGL_TC_COLD_OFF_POWER_DOMAINS,
		.ops = &tgl_tc_cold_off_ops,
		.id = TGL_DISP_PW_TC_COLD_OFF,
	}, {
		.name = "AUX_A",
		.domains = TGL_AUX_A_IO_POWER_DOMAINS,
		.ops = &icl_aux_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = ICL_PW_CTL_IDX_AUX_A,
		},
	}, {
		.name = "AUX_B",
		.domains = TGL_AUX_B_IO_POWER_DOMAINS,
		.ops = &icl_aux_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = ICL_PW_CTL_IDX_AUX_B,
		},
	}, {
		.name = "AUX_C",
		.domains = TGL_AUX_C_IO_POWER_DOMAINS,
		.ops = &icl_aux_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = ICL_PW_CTL_IDX_AUX_C,
		},
	}, {
		.name = "AUX_USBC1",
		.domains = TGL_AUX_IO_USBC1_POWER_DOMAINS,
		.ops = &icl_aux_power_well_ops,
		.is_tc_tbt = false,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = TGL_PW_CTL_IDX_AUX_TC1,
		},
	}, {
		.name = "AUX_USBC2",
		.domains = TGL_AUX_IO_USBC2_POWER_DOMAINS,
		.ops = &icl_aux_power_well_ops,
		.is_tc_tbt = false,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = TGL_PW_CTL_IDX_AUX_TC2,
		},
	}, {
		.name = "AUX_USBC3",
		.domains = TGL_AUX_IO_USBC3_POWER_DOMAINS,
		.ops = &icl_aux_power_well_ops,
		.is_tc_tbt = false,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = TGL_PW_CTL_IDX_AUX_TC3,
		},
	}, {
		.name = "AUX_USBC4",
		.domains = TGL_AUX_IO_USBC4_POWER_DOMAINS,
		.ops = &icl_aux_power_well_ops,
		.is_tc_tbt = false,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = TGL_PW_CTL_IDX_AUX_TC4,
		},
	}, {
		.name = "AUX_USBC5",
		.domains = TGL_AUX_IO_USBC5_POWER_DOMAINS,
		.ops = &icl_aux_power_well_ops,
		.is_tc_tbt = false,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = TGL_PW_CTL_IDX_AUX_TC5,
		},
	}, {
		.name = "AUX_USBC6",
		.domains = TGL_AUX_IO_USBC6_POWER_DOMAINS,
		.ops = &icl_aux_power_well_ops,
		.is_tc_tbt = false,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = TGL_PW_CTL_IDX_AUX_TC6,
		},
	}, {
		.name = "AUX_TBT1",
		.domains = TGL_AUX_IO_TBT1_POWER_DOMAINS,
		.ops = &icl_aux_power_well_ops,
		.is_tc_tbt = true,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = TGL_PW_CTL_IDX_AUX_TBT1,
		},
	}, {
		.name = "AUX_TBT2",
		.domains = TGL_AUX_IO_TBT2_POWER_DOMAINS,
		.ops = &icl_aux_power_well_ops,
		.is_tc_tbt = true,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = TGL_PW_CTL_IDX_AUX_TBT2,
		},
	}, {
		.name = "AUX_TBT3",
		.domains = TGL_AUX_IO_TBT3_POWER_DOMAINS,
		.ops = &icl_aux_power_well_ops,
		.is_tc_tbt = true,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = TGL_PW_CTL_IDX_AUX_TBT3,
		},
	}, {
		.name = "AUX_TBT4",
		.domains = TGL_AUX_IO_TBT4_POWER_DOMAINS,
		.ops = &icl_aux_power_well_ops,
		.is_tc_tbt = true,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = TGL_PW_CTL_IDX_AUX_TBT4,
		},
	}, {
		.name = "AUX_TBT5",
		.domains = TGL_AUX_IO_TBT5_POWER_DOMAINS,
		.ops = &icl_aux_power_well_ops,
		.is_tc_tbt = true,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = TGL_PW_CTL_IDX_AUX_TBT5,
		},
	}, {
		.name = "AUX_TBT6",
		.domains = TGL_AUX_IO_TBT6_POWER_DOMAINS,
		.ops = &icl_aux_power_well_ops,
		.is_tc_tbt = true,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = TGL_PW_CTL_IDX_AUX_TBT6,
		},
	}, {
		.name = "PW_4",
		.domains = TGL_PW_4_POWER_DOMAINS,
		.ops = &hsw_power_well_ops,
		.has_fuses = true,
		.irq_pipe_mask = BIT(PIPE_C),
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = ICL_PW_CTL_IDX_PW_4,
		}
	}, {
		.name = "PW_5",
		.domains = TGL_PW_5_POWER_DOMAINS,
		.ops = &hsw_power_well_ops,
		.has_fuses = true,
		.irq_pipe_mask = BIT(PIPE_D),
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = TGL_PW_CTL_IDX_PW_5,
		},
	},
};

#define RKL_PW_4_POWER_DOMAINS (			\
	BIT_ULL(POWER_DOMAIN_PIPE_C) |			\
	BIT_ULL(POWER_DOMAIN_PIPE_PANEL_FITTER_C) |	\
	BIT_ULL(POWER_DOMAIN_TRANSCODER_C) |		\
	BIT_ULL(POWER_DOMAIN_INIT))

#define RKL_PW_3_POWER_DOMAINS (			\
	RKL_PW_4_POWER_DOMAINS |			\
	BIT_ULL(POWER_DOMAIN_PIPE_B) |			\
	BIT_ULL(POWER_DOMAIN_PIPE_PANEL_FITTER_B) |	\
	BIT_ULL(POWER_DOMAIN_TRANSCODER_B) |		\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_TC1) |	\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_TC2) |	\
	BIT_ULL(POWER_DOMAIN_VGA) |			\
	BIT_ULL(POWER_DOMAIN_AUDIO_MMIO) |		\
	BIT_ULL(POWER_DOMAIN_AUDIO_PLAYBACK) |			\
	BIT_ULL(POWER_DOMAIN_AUX_USBC1) |		\
	BIT_ULL(POWER_DOMAIN_AUX_USBC2) |		\
	BIT_ULL(POWER_DOMAIN_INIT))

/*
 * There is no PW_2/PG_2 on RKL.
 *
 * RKL PW_1/PG_1 domains (under HW/DMC control):
 * - DBUF function (note: registers are in PW0)
 * - PIPE_A and its planes and VDSC/joining, except VGA
 * - transcoder A
 * - DDI_A and DDI_B
 * - FBC
 *
 * RKL PW_0/PG_0 domains (under HW/DMC control):
 * - PCI
 * - clocks except port PLL
 * - shared functions:
 *     * interrupts except pipe interrupts
 *     * MBus except PIPE_MBUS_DBOX_CTL
 *     * DBUF registers
 * - central power except FBC
 * - top-level GTC (DDI-level GTC is in the well associated with the DDI)
 */

#define RKL_DISPLAY_DC_OFF_POWER_DOMAINS (		\
	RKL_PW_3_POWER_DOMAINS |			\
	BIT_ULL(POWER_DOMAIN_AUX_A) |			\
	BIT_ULL(POWER_DOMAIN_AUX_B) |			\
	BIT_ULL(POWER_DOMAIN_MODESET) |			\
	BIT_ULL(POWER_DOMAIN_INIT))

static const struct i915_power_well_desc rkl_power_wells[] = {
	{
		.name = "always-on",
		.domains = POWER_DOMAIN_MASK,
		.ops = &i9xx_always_on_power_well_ops,
		.always_on = true,
		.id = DISP_PW_ID_NONE,
	}, {
		.name = "PW_1",
		/* Handled by the DMC firmware */
		.domains = 0,
		.ops = &hsw_power_well_ops,
		.always_on = true,
		.has_fuses = true,
		.id = SKL_DISP_PW_1,
		{
			.hsw.idx = ICL_PW_CTL_IDX_PW_1,
		},
	}, {
		.name = "DC_off",
		.domains = RKL_DISPLAY_DC_OFF_POWER_DOMAINS,
		.ops = &gen9_dc_off_power_well_ops,
		.id = SKL_DISP_DC_OFF,
	}, {
		.name = "PW_3",
		.domains = RKL_PW_3_POWER_DOMAINS,
		.ops = &hsw_power_well_ops,
		.irq_pipe_mask = BIT(PIPE_B),
		.has_vga = true,
		.has_fuses = true,
		.id = ICL_DISP_PW_3,
		{
			.hsw.idx = ICL_PW_CTL_IDX_PW_3,
		},
	}, {
		.name = "PW_4",
		.domains = RKL_PW_4_POWER_DOMAINS,
		.ops = &hsw_power_well_ops,
		.has_fuses = true,
		.irq_pipe_mask = BIT(PIPE_C),
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = ICL_PW_CTL_IDX_PW_4,
		}
	}, {
		.name = "DDI_IO_A",
		.domains = ICL_DDI_IO_A_POWER_DOMAINS,
		.ops = &icl_ddi_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = ICL_PW_CTL_IDX_DDI_A,
		}
	}, {
		.name = "DDI_IO_B",
		.domains = ICL_DDI_IO_B_POWER_DOMAINS,
		.ops = &icl_ddi_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = ICL_PW_CTL_IDX_DDI_B,
		}
	}, {
		.name = "DDI_IO_TC1",
		.domains = TGL_DDI_IO_TC1_POWER_DOMAINS,
		.ops = &icl_ddi_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = TGL_PW_CTL_IDX_DDI_TC1,
		},
	}, {
		.name = "DDI_IO_TC2",
		.domains = TGL_DDI_IO_TC2_POWER_DOMAINS,
		.ops = &icl_ddi_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = TGL_PW_CTL_IDX_DDI_TC2,
		},
	}, {
		.name = "AUX_A",
		.domains = ICL_AUX_A_IO_POWER_DOMAINS,
		.ops = &icl_aux_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = ICL_PW_CTL_IDX_AUX_A,
		},
	}, {
		.name = "AUX_B",
		.domains = ICL_AUX_B_IO_POWER_DOMAINS,
		.ops = &icl_aux_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = ICL_PW_CTL_IDX_AUX_B,
		},
	}, {
		.name = "AUX_USBC1",
		.domains = TGL_AUX_IO_USBC1_POWER_DOMAINS,
		.ops = &icl_aux_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = TGL_PW_CTL_IDX_AUX_TC1,
		},
	}, {
		.name = "AUX_USBC2",
		.domains = TGL_AUX_IO_USBC2_POWER_DOMAINS,
		.ops = &icl_aux_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = TGL_PW_CTL_IDX_AUX_TC2,
		},
	},
};

/*
 * DG1 onwards Audio MMIO/VERBS lies in PG0 power well.
 */
#define DG1_PW_3_POWER_DOMAINS (			\
	TGL_PW_4_POWER_DOMAINS |			\
	BIT_ULL(POWER_DOMAIN_PIPE_B) |			\
	BIT_ULL(POWER_DOMAIN_PIPE_PANEL_FITTER_B) |	\
	BIT_ULL(POWER_DOMAIN_TRANSCODER_B) |		\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_TC1) |	\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_TC2) |	\
	BIT_ULL(POWER_DOMAIN_VGA) |			\
	BIT_ULL(POWER_DOMAIN_AUDIO_PLAYBACK) |		\
	BIT_ULL(POWER_DOMAIN_AUX_USBC1) |		\
	BIT_ULL(POWER_DOMAIN_AUX_USBC2) |		\
	BIT_ULL(POWER_DOMAIN_INIT))

#define DG1_DISPLAY_DC_OFF_POWER_DOMAINS (		\
	DG1_PW_3_POWER_DOMAINS |			\
	BIT_ULL(POWER_DOMAIN_AUDIO_MMIO) |		\
	BIT_ULL(POWER_DOMAIN_AUX_A) |			\
	BIT_ULL(POWER_DOMAIN_AUX_B) |			\
	BIT_ULL(POWER_DOMAIN_MODESET) |			\
	BIT_ULL(POWER_DOMAIN_INIT))

#define DG1_PW_2_POWER_DOMAINS (			\
	DG1_PW_3_POWER_DOMAINS |			\
	BIT_ULL(POWER_DOMAIN_TRANSCODER_VDSC_PW2) |	\
	BIT_ULL(POWER_DOMAIN_INIT))

static const struct i915_power_well_desc dg1_power_wells[] = {
	{
		.name = "always-on",
		.domains = POWER_DOMAIN_MASK,
		.ops = &i9xx_always_on_power_well_ops,
		.always_on = true,
		.id = DISP_PW_ID_NONE,
	}, {
		.name = "PW_1",
		/* Handled by the DMC firmware */
		.domains = 0,
		.ops = &hsw_power_well_ops,
		.always_on = true,
		.has_fuses = true,
		.id = SKL_DISP_PW_1,
		{
			.hsw.idx = ICL_PW_CTL_IDX_PW_1,
		},
	}, {
		.name = "DC_off",
		.domains = DG1_DISPLAY_DC_OFF_POWER_DOMAINS,
		.ops = &gen9_dc_off_power_well_ops,
		.id = SKL_DISP_DC_OFF,
	}, {
		.name = "PW_2",
		.domains = DG1_PW_2_POWER_DOMAINS,
		.ops = &hsw_power_well_ops,
		.has_fuses = true,
		.id = SKL_DISP_PW_2,
		{
			.hsw.idx = ICL_PW_CTL_IDX_PW_2,
		},
	}, {
		.name = "PW_3",
		.domains = DG1_PW_3_POWER_DOMAINS,
		.ops = &hsw_power_well_ops,
		.irq_pipe_mask = BIT(PIPE_B),
		.has_vga = true,
		.has_fuses = true,
		.id = ICL_DISP_PW_3,
		{
			.hsw.idx = ICL_PW_CTL_IDX_PW_3,
		},
	}, {
		.name = "DDI_IO_A",
		.domains = ICL_DDI_IO_A_POWER_DOMAINS,
		.ops = &icl_ddi_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = ICL_PW_CTL_IDX_DDI_A,
		}
	}, {
		.name = "DDI_IO_B",
		.domains = ICL_DDI_IO_B_POWER_DOMAINS,
		.ops = &icl_ddi_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = ICL_PW_CTL_IDX_DDI_B,
		}
	}, {
		.name = "DDI_IO_TC1",
		.domains = TGL_DDI_IO_TC1_POWER_DOMAINS,
		.ops = &icl_ddi_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = TGL_PW_CTL_IDX_DDI_TC1,
		},
	}, {
		.name = "DDI_IO_TC2",
		.domains = TGL_DDI_IO_TC2_POWER_DOMAINS,
		.ops = &icl_ddi_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = TGL_PW_CTL_IDX_DDI_TC2,
		},
	}, {
		.name = "AUX_A",
		.domains = TGL_AUX_A_IO_POWER_DOMAINS,
		.ops = &icl_aux_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = ICL_PW_CTL_IDX_AUX_A,
		},
	}, {
		.name = "AUX_B",
		.domains = TGL_AUX_B_IO_POWER_DOMAINS,
		.ops = &icl_aux_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = ICL_PW_CTL_IDX_AUX_B,
		},
	}, {
		.name = "AUX_USBC1",
		.domains = TGL_AUX_IO_USBC1_POWER_DOMAINS,
		.ops = &icl_aux_power_well_ops,
		.is_tc_tbt = false,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = TGL_PW_CTL_IDX_AUX_TC1,
		},
	}, {
		.name = "AUX_USBC2",
		.domains = TGL_AUX_IO_USBC2_POWER_DOMAINS,
		.ops = &icl_aux_power_well_ops,
		.is_tc_tbt = false,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = TGL_PW_CTL_IDX_AUX_TC2,
		},
	}, {
		.name = "PW_4",
		.domains = TGL_PW_4_POWER_DOMAINS,
		.ops = &hsw_power_well_ops,
		.has_fuses = true,
		.irq_pipe_mask = BIT(PIPE_C),
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = ICL_PW_CTL_IDX_PW_4,
		}
	}, {
		.name = "PW_5",
		.domains = TGL_PW_5_POWER_DOMAINS,
		.ops = &hsw_power_well_ops,
		.has_fuses = true,
		.irq_pipe_mask = BIT(PIPE_D),
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = TGL_PW_CTL_IDX_PW_5,
		},
	},
};

/*
 * XE_LPD Power Domains
 *
 * Previous platforms required that PG(n-1) be enabled before PG(n).  That
 * dependency chain turns into a dependency tree on XE_LPD:
 *
 *       PG0
 *        |
 *     --PG1--
 *    /       \
 *  PGA     --PG2--
 *         /   |   \
 *       PGB  PGC  PGD
 *
 * Power wells must be enabled from top to bottom and disabled from bottom
 * to top.  This allows pipes to be power gated independently.
 */

#define XELPD_PW_D_POWER_DOMAINS (			\
	BIT_ULL(POWER_DOMAIN_PIPE_D) |			\
	BIT_ULL(POWER_DOMAIN_PIPE_PANEL_FITTER_D) |	\
	BIT_ULL(POWER_DOMAIN_TRANSCODER_D) |		\
	BIT_ULL(POWER_DOMAIN_INIT))

#define XELPD_PW_C_POWER_DOMAINS (			\
	BIT_ULL(POWER_DOMAIN_PIPE_C) |			\
	BIT_ULL(POWER_DOMAIN_PIPE_PANEL_FITTER_C) |	\
	BIT_ULL(POWER_DOMAIN_TRANSCODER_C) |		\
	BIT_ULL(POWER_DOMAIN_INIT))

#define XELPD_PW_B_POWER_DOMAINS (			\
	BIT_ULL(POWER_DOMAIN_PIPE_B) |			\
	BIT_ULL(POWER_DOMAIN_PIPE_PANEL_FITTER_B) |	\
	BIT_ULL(POWER_DOMAIN_TRANSCODER_B) |		\
	BIT_ULL(POWER_DOMAIN_INIT))

#define XELPD_PW_A_POWER_DOMAINS (			\
	BIT_ULL(POWER_DOMAIN_PIPE_A) |			\
	BIT_ULL(POWER_DOMAIN_PIPE_PANEL_FITTER_A) |	\
	BIT_ULL(POWER_DOMAIN_INIT))

#define XELPD_PW_2_POWER_DOMAINS (			\
	XELPD_PW_B_POWER_DOMAINS |			\
	XELPD_PW_C_POWER_DOMAINS |			\
	XELPD_PW_D_POWER_DOMAINS |			\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_C) |	\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_D_XELPD) |	\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_E_XELPD) |	\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_TC1) |	\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_TC2) |	\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_TC3) |	\
	BIT_ULL(POWER_DOMAIN_PORT_DDI_LANES_TC4) |	\
	BIT_ULL(POWER_DOMAIN_VGA) |			\
	BIT_ULL(POWER_DOMAIN_AUDIO_PLAYBACK) |		\
	BIT_ULL(POWER_DOMAIN_AUX_C) |			\
	BIT_ULL(POWER_DOMAIN_AUX_D_XELPD) |		\
	BIT_ULL(POWER_DOMAIN_AUX_E_XELPD) |		\
	BIT_ULL(POWER_DOMAIN_AUX_USBC1) |			\
	BIT_ULL(POWER_DOMAIN_AUX_USBC2) |			\
	BIT_ULL(POWER_DOMAIN_AUX_USBC3) |			\
	BIT_ULL(POWER_DOMAIN_AUX_USBC4) |			\
	BIT_ULL(POWER_DOMAIN_AUX_TBT1) |			\
	BIT_ULL(POWER_DOMAIN_AUX_TBT2) |			\
	BIT_ULL(POWER_DOMAIN_AUX_TBT3) |			\
	BIT_ULL(POWER_DOMAIN_AUX_TBT4) |			\
	BIT_ULL(POWER_DOMAIN_INIT))

/*
 * XELPD PW_1/PG_1 domains (under HW/DMC control):
 *  - DBUF function (registers are in PW0)
 *  - Transcoder A
 *  - DDI_A and DDI_B
 *
 * XELPD PW_0/PW_1 domains (under HW/DMC control):
 *  - PCI
 *  - Clocks except port PLL
 *  - Shared functions:
 *     * interrupts except pipe interrupts
 *     * MBus except PIPE_MBUS_DBOX_CTL
 *     * DBUF registers
 *  - Central power except FBC
 *  - Top-level GTC (DDI-level GTC is in the well associated with the DDI)
 */

#define XELPD_DISPLAY_DC_OFF_POWER_DOMAINS (		\
	XELPD_PW_2_POWER_DOMAINS |			\
	BIT_ULL(POWER_DOMAIN_PORT_DSI) |		\
	BIT_ULL(POWER_DOMAIN_AUDIO_MMIO) |		\
	BIT_ULL(POWER_DOMAIN_AUX_A) |			\
	BIT_ULL(POWER_DOMAIN_AUX_B) |			\
	BIT_ULL(POWER_DOMAIN_MODESET) |			\
	BIT_ULL(POWER_DOMAIN_INIT))

#define XELPD_AUX_IO_D_XELPD_POWER_DOMAINS	BIT_ULL(POWER_DOMAIN_AUX_D_XELPD)
#define XELPD_AUX_IO_E_XELPD_POWER_DOMAINS	BIT_ULL(POWER_DOMAIN_AUX_E_XELPD)
#define XELPD_AUX_IO_USBC1_POWER_DOMAINS	BIT_ULL(POWER_DOMAIN_AUX_USBC1)
#define XELPD_AUX_IO_USBC2_POWER_DOMAINS	BIT_ULL(POWER_DOMAIN_AUX_USBC2)
#define XELPD_AUX_IO_USBC3_POWER_DOMAINS	BIT_ULL(POWER_DOMAIN_AUX_USBC3)
#define XELPD_AUX_IO_USBC4_POWER_DOMAINS	BIT_ULL(POWER_DOMAIN_AUX_USBC4)

#define XELPD_AUX_IO_TBT1_POWER_DOMAINS		BIT_ULL(POWER_DOMAIN_AUX_TBT1)
#define XELPD_AUX_IO_TBT2_POWER_DOMAINS		BIT_ULL(POWER_DOMAIN_AUX_TBT2)
#define XELPD_AUX_IO_TBT3_POWER_DOMAINS		BIT_ULL(POWER_DOMAIN_AUX_TBT3)
#define XELPD_AUX_IO_TBT4_POWER_DOMAINS		BIT_ULL(POWER_DOMAIN_AUX_TBT4)

#define XELPD_DDI_IO_D_XELPD_POWER_DOMAINS	BIT_ULL(POWER_DOMAIN_PORT_DDI_IO_D_XELPD)
#define XELPD_DDI_IO_E_XELPD_POWER_DOMAINS	BIT_ULL(POWER_DOMAIN_PORT_DDI_IO_E_XELPD)
#define XELPD_DDI_IO_TC1_POWER_DOMAINS		BIT_ULL(POWER_DOMAIN_PORT_DDI_IO_TC1)
#define XELPD_DDI_IO_TC2_POWER_DOMAINS		BIT_ULL(POWER_DOMAIN_PORT_DDI_IO_TC2)
#define XELPD_DDI_IO_TC3_POWER_DOMAINS		BIT_ULL(POWER_DOMAIN_PORT_DDI_IO_TC3)
#define XELPD_DDI_IO_TC4_POWER_DOMAINS		BIT_ULL(POWER_DOMAIN_PORT_DDI_IO_TC4)

static const struct i915_power_well_desc xelpd_power_wells[] = {
	{
		.name = "always-on",
		.domains = POWER_DOMAIN_MASK,
		.ops = &i9xx_always_on_power_well_ops,
		.always_on = true,
		.id = DISP_PW_ID_NONE,
	}, {
		.name = "PW_1",
		/* Handled by the DMC firmware */
		.domains = 0,
		.ops = &hsw_power_well_ops,
		.always_on = true,
		.has_fuses = true,
		.id = SKL_DISP_PW_1,
		{
			.hsw.idx = ICL_PW_CTL_IDX_PW_1,
		},
	}, {
		.name = "DC_off",
		.domains = XELPD_DISPLAY_DC_OFF_POWER_DOMAINS,
		.ops = &gen9_dc_off_power_well_ops,
		.id = SKL_DISP_DC_OFF,
	}, {
		.name = "PW_2",
		.domains = XELPD_PW_2_POWER_DOMAINS,
		.ops = &hsw_power_well_ops,
		.has_vga = true,
		.has_fuses = true,
		.id = SKL_DISP_PW_2,
		{
			.hsw.idx = ICL_PW_CTL_IDX_PW_2,
		},
	}, {
		.name = "PW_A",
		.domains = XELPD_PW_A_POWER_DOMAINS,
		.ops = &hsw_power_well_ops,
		.irq_pipe_mask = BIT(PIPE_A),
		.has_fuses = true,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = XELPD_PW_CTL_IDX_PW_A,
		},
	}, {
		.name = "PW_B",
		.domains = XELPD_PW_B_POWER_DOMAINS,
		.ops = &hsw_power_well_ops,
		.irq_pipe_mask = BIT(PIPE_B),
		.has_fuses = true,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = XELPD_PW_CTL_IDX_PW_B,
		},
	}, {
		.name = "PW_C",
		.domains = XELPD_PW_C_POWER_DOMAINS,
		.ops = &hsw_power_well_ops,
		.irq_pipe_mask = BIT(PIPE_C),
		.has_fuses = true,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = XELPD_PW_CTL_IDX_PW_C,
		},
	}, {
		.name = "PW_D",
		.domains = XELPD_PW_D_POWER_DOMAINS,
		.ops = &hsw_power_well_ops,
		.irq_pipe_mask = BIT(PIPE_D),
		.has_fuses = true,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = XELPD_PW_CTL_IDX_PW_D,
		},
	}, {
		.name = "DDI_IO_A",
		.domains = ICL_DDI_IO_A_POWER_DOMAINS,
		.ops = &icl_ddi_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = ICL_PW_CTL_IDX_DDI_A,
		}
	}, {
		.name = "DDI_IO_B",
		.domains = ICL_DDI_IO_B_POWER_DOMAINS,
		.ops = &icl_ddi_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = ICL_PW_CTL_IDX_DDI_B,
		}
	}, {
		.name = "DDI_IO_C",
		.domains = ICL_DDI_IO_C_POWER_DOMAINS,
		.ops = &icl_ddi_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = ICL_PW_CTL_IDX_DDI_C,
		}
	}, {
		.name = "DDI_IO_D_XELPD",
		.domains = XELPD_DDI_IO_D_XELPD_POWER_DOMAINS,
		.ops = &icl_ddi_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = XELPD_PW_CTL_IDX_DDI_D,
		}
	}, {
		.name = "DDI_IO_E_XELPD",
		.domains = XELPD_DDI_IO_E_XELPD_POWER_DOMAINS,
		.ops = &icl_ddi_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = XELPD_PW_CTL_IDX_DDI_E,
		}
	}, {
		.name = "DDI_IO_TC1",
		.domains = XELPD_DDI_IO_TC1_POWER_DOMAINS,
		.ops = &icl_ddi_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = TGL_PW_CTL_IDX_DDI_TC1,
		}
	}, {
		.name = "DDI_IO_TC2",
		.domains = XELPD_DDI_IO_TC2_POWER_DOMAINS,
		.ops = &icl_ddi_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = TGL_PW_CTL_IDX_DDI_TC2,
		}
	}, {
		.name = "DDI_IO_TC3",
		.domains = XELPD_DDI_IO_TC3_POWER_DOMAINS,
		.ops = &icl_ddi_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = TGL_PW_CTL_IDX_DDI_TC3,
		}
	}, {
		.name = "DDI_IO_TC4",
		.domains = XELPD_DDI_IO_TC4_POWER_DOMAINS,
		.ops = &icl_ddi_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = TGL_PW_CTL_IDX_DDI_TC4,
		}
	}, {
		.name = "AUX_A",
		.domains = ICL_AUX_A_IO_POWER_DOMAINS,
		.ops = &icl_aux_power_well_ops,
		.fixed_enable_delay = true,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = ICL_PW_CTL_IDX_AUX_A,
		},
	}, {
		.name = "AUX_B",
		.domains = ICL_AUX_B_IO_POWER_DOMAINS,
		.ops = &icl_aux_power_well_ops,
		.fixed_enable_delay = true,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = ICL_PW_CTL_IDX_AUX_B,
		},
	}, {
		.name = "AUX_C",
		.domains = TGL_AUX_C_IO_POWER_DOMAINS,
		.ops = &icl_aux_power_well_ops,
		.fixed_enable_delay = true,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = ICL_PW_CTL_IDX_AUX_C,
		},
	}, {
		.name = "AUX_D_XELPD",
		.domains = XELPD_AUX_IO_D_XELPD_POWER_DOMAINS,
		.ops = &icl_aux_power_well_ops,
		.fixed_enable_delay = true,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = XELPD_PW_CTL_IDX_AUX_D,
		},
	}, {
		.name = "AUX_E_XELPD",
		.domains = XELPD_AUX_IO_E_XELPD_POWER_DOMAINS,
		.ops = &icl_aux_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = XELPD_PW_CTL_IDX_AUX_E,
		},
	}, {
		.name = "AUX_USBC1",
		.domains = XELPD_AUX_IO_USBC1_POWER_DOMAINS,
		.ops = &icl_aux_power_well_ops,
		.fixed_enable_delay = true,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = TGL_PW_CTL_IDX_AUX_TC1,
		},
	}, {
		.name = "AUX_USBC2",
		.domains = XELPD_AUX_IO_USBC2_POWER_DOMAINS,
		.ops = &icl_aux_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = TGL_PW_CTL_IDX_AUX_TC2,
		},
	}, {
		.name = "AUX_USBC3",
		.domains = XELPD_AUX_IO_USBC3_POWER_DOMAINS,
		.ops = &icl_aux_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = TGL_PW_CTL_IDX_AUX_TC3,
		},
	}, {
		.name = "AUX_USBC4",
		.domains = XELPD_AUX_IO_USBC4_POWER_DOMAINS,
		.ops = &icl_aux_power_well_ops,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = TGL_PW_CTL_IDX_AUX_TC4,
		},
	}, {
		.name = "AUX_TBT1",
		.domains = XELPD_AUX_IO_TBT1_POWER_DOMAINS,
		.ops = &icl_aux_power_well_ops,
		.is_tc_tbt = true,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = TGL_PW_CTL_IDX_AUX_TBT1,
		},
	}, {
		.name = "AUX_TBT2",
		.domains = XELPD_AUX_IO_TBT2_POWER_DOMAINS,
		.ops = &icl_aux_power_well_ops,
		.is_tc_tbt = true,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = TGL_PW_CTL_IDX_AUX_TBT2,
		},
	}, {
		.name = "AUX_TBT3",
		.domains = XELPD_AUX_IO_TBT3_POWER_DOMAINS,
		.ops = &icl_aux_power_well_ops,
		.is_tc_tbt = true,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = TGL_PW_CTL_IDX_AUX_TBT3,
		},
	}, {
		.name = "AUX_TBT4",
		.domains = XELPD_AUX_IO_TBT4_POWER_DOMAINS,
		.ops = &icl_aux_power_well_ops,
		.is_tc_tbt = true,
		.id = DISP_PW_ID_NONE,
		{
			.hsw.idx = TGL_PW_CTL_IDX_AUX_TBT4,
		},
	},
};

static int
__set_power_wells(struct i915_power_domains *power_domains,
		  const struct i915_power_well_desc *power_well_descs,
		  int power_well_descs_sz, u64 skip_mask)
{
	struct drm_i915_private *i915 = container_of(power_domains,
						     struct drm_i915_private,
						     power_domains);
	u64 power_well_ids = 0;
	int power_well_count = 0;
	int i, plt_idx = 0;

	for (i = 0; i < power_well_descs_sz; i++)
		if (!(BIT_ULL(power_well_descs[i].id) & skip_mask))
			power_well_count++;

	power_domains->power_well_count = power_well_count;
	power_domains->power_wells =
				kcalloc(power_well_count,
					sizeof(*power_domains->power_wells),
					GFP_KERNEL);
	if (!power_domains->power_wells)
		return -ENOMEM;

	for (i = 0; i < power_well_descs_sz; i++) {
		enum i915_power_well_id id = power_well_descs[i].id;

		if (BIT_ULL(id) & skip_mask)
			continue;

		power_domains->power_wells[plt_idx++].desc =
			&power_well_descs[i];

		if (id == DISP_PW_ID_NONE)
			continue;

		drm_WARN_ON(&i915->drm, id >= sizeof(power_well_ids) * 8);
		drm_WARN_ON(&i915->drm, power_well_ids & BIT_ULL(id));
		power_well_ids |= BIT_ULL(id);
	}

	return 0;
}

#define set_power_wells_mask(power_domains, __power_well_descs, skip_mask) \
	__set_power_wells(power_domains, __power_well_descs, \
			  ARRAY_SIZE(__power_well_descs), skip_mask)

#define set_power_wells(power_domains, __power_well_descs) \
	set_power_wells_mask(power_domains, __power_well_descs, 0)

/**
 * intel_display_power_map_init - initialize power domain -> power well mappings
 * @power_domains: power domain state
 *
 * Creates all the power wells for the current platform, initializes the
 * dynamic state for them and initializes the mapping of each power well to
 * all the power domains the power well belongs to.
 */
int intel_display_power_map_init(struct i915_power_domains *power_domains)
{
	struct drm_i915_private *i915 = container_of(power_domains,
						     struct drm_i915_private,
						     power_domains);
	/*
	 * The enabling order will be from lower to higher indexed wells,
	 * the disabling order is reversed.
	 */
	if (!HAS_DISPLAY(i915)) {
		power_domains->power_well_count = 0;
		return 0;
	}

	if (DISPLAY_VER(i915) >= 13)
		return set_power_wells(power_domains, xelpd_power_wells);
	else if (IS_DG1(i915))
		return set_power_wells(power_domains, dg1_power_wells);
	else if (IS_ALDERLAKE_S(i915))
		return set_power_wells_mask(power_domains, tgl_power_wells,
					   BIT_ULL(TGL_DISP_PW_TC_COLD_OFF));
	else if (IS_ROCKETLAKE(i915))
		return set_power_wells(power_domains, rkl_power_wells);
	else if (DISPLAY_VER(i915) == 12)
		return set_power_wells(power_domains, tgl_power_wells);
	else if (DISPLAY_VER(i915) == 11)
		return set_power_wells(power_domains, icl_power_wells);
	else if (IS_GEMINILAKE(i915))
		return set_power_wells(power_domains, glk_power_wells);
	else if (IS_BROXTON(i915))
		return set_power_wells(power_domains, bxt_power_wells);
	else if (DISPLAY_VER(i915) == 9)
		return set_power_wells(power_domains, skl_power_wells);
	else if (IS_CHERRYVIEW(i915))
		return set_power_wells(power_domains, chv_power_wells);
	else if (IS_BROADWELL(i915))
		return set_power_wells(power_domains, bdw_power_wells);
	else if (IS_HASWELL(i915))
		return set_power_wells(power_domains, hsw_power_wells);
	else if (IS_VALLEYVIEW(i915))
		return set_power_wells(power_domains, vlv_power_wells);
	else if (IS_I830(i915))
		return set_power_wells(power_domains, i830_power_wells);
	else
		return set_power_wells(power_domains, i9xx_always_on_power_well);
}

/**
 * intel_display_power_map_cleanup - clean up power domain -> power well mappings
 * @power_domains: power domain state
 *
 * Cleans up all the state that was initialized by intel_display_power_map_init().
 */
void intel_display_power_map_cleanup(struct i915_power_domains *power_domains)
{
	kfree(power_domains->power_wells);
}
