/*
 * Copyright (c) 2026, David Yang <mmyangfl@gmail.com>.
 * Based on plat/hisilicon/poplar.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <platform_def.h>

#include <drivers/arm/gicv2.h>
#include <plat/common/platform.h>

#include "plat_private.h"

/******************************************************************************
 * On a GICv2 system, the Group 1 secure interrupts are treated as Group 0
 * interrupts.
 *****************************************************************************/
static const interrupt_prop_t hisi_interrupt_props[] = {
	MV200H_G1S_IRQ_PROPS(GICV2_INTR_GROUP0),
	MV200H_G0_IRQ_PROPS(GICV2_INTR_GROUP0)
};

static unsigned int target_mask_array[PLATFORM_CORE_COUNT];

static const gicv2_driver_data_t hisi_gic_data = {
	.gicd_base = MV200H_GICD_BASE,
	.gicc_base = MV200H_GICC_BASE,
	.interrupt_props = hisi_interrupt_props,
	.interrupt_props_num = ARRAY_SIZE(hisi_interrupt_props),
	.target_masks = target_mask_array,
	.target_masks_num = ARRAY_SIZE(target_mask_array),
};

/******************************************************************************
 * Helper to initialize the GICv2 only driver.
 *
 * This is the only GIC entry point EL3 needs: the distributor belongs to the
 * non-secure payload, and every round in which EL3 programmed it here ended
 * with no interrupt delivery.  See the R99 finding in
 * docs/mv200h-tfa-bl31-boardlog-2026-10-26.md.
 *****************************************************************************/
void hisi_gic_driver_init(void)
{
	gicv2_driver_init(&hisi_gic_data);
}
