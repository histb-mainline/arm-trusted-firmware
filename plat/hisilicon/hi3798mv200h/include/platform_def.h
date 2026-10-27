/*
 * Copyright (c) 2026, David Yang <mmyangfl@gmail.com>.
 * Based on plat/hisilicon/poplar.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLATFORM_DEF_H
#define PLATFORM_DEF_H

#include <arch.h>
#include <common/interrupt_props.h>
#include <common/tbbr/tbbr_img_def.h>
#include <drivers/arm/gic_common.h>
#include <lib/utils_def.h>
#include <plat/common/common_def.h>

#include "hi3798mv200h.h"
#include "mv200h_layout.h"

#define PLATFORM_LINKER_FORMAT		"elf64-littleaarch64"
#define PLATFORM_LINKER_ARCH		aarch64

#define MV200H_CRASH_UART_BASE		PL011_UART0_BASE
#define MV200H_CRASH_UART_CLK_IN_HZ	PL011_UART0_CLK_IN_HZ
#define MV200H_CONSOLE_BAUDRATE		PL011_BAUDRATE

/* Generic platform constants */
#define PLATFORM_STACK_SIZE		UL(0x800)

#define PLATFORM_CACHE_LINE_SIZE	U(64)
#define PLATFORM_CLUSTER_COUNT		U(1)
#define PLATFORM_CORE_COUNT		U(4)
#define PLATFORM_MAX_CPUS_PER_CLUSTER	U(4)

/* Memory map related constants.  DRAM covers at least the 1 GiB window the
 * fastboot environment reports; BL31 only maps what it uses.
 */
#define DDR_BASE			UL(0x00000000)
#define DDR_SIZE			UL(0x40000000)

#define DEVICE_BASE			UL(0xF0000000)
#define DEVICE_SIZE			UL(0x0F000000)

/* BL31 memory layout: see mv200h_layout.h for the boot flow around it. */
#define BL31_BASE			MV200H_BL31_BASE
#define BL31_LIMIT			(MV200H_BL31_BASE + MV200H_BL31_SIZE)

/* Page table and MMU setup constants */
#define PLAT_VIRT_ADDR_SPACE_SIZE	(1ULL << 32)
#define PLAT_PHY_ADDR_SPACE_SIZE	(1ULL << 32)
/*
 * One table more than the mapping list consumes.  The extra page is not
 * headroom for regions: it is what moves .coherent_ram off 0x200e000, where
 * PSCI CPU_OFF does not complete on this board.  Measurements in
 * doc/plat/hi3798mv200h.rst.
 */
#define MAX_XLAT_TABLES			U(5)
#define MAX_MMAP_REGIONS		U(8)

#define CACHE_WRITEBACK_SHIFT		U(6)
#define CACHE_WRITEBACK_GRANULE		(1 << CACHE_WRITEBACK_SHIFT)

/* Power states */
#define PLAT_MAX_PWR_LVL		MPIDR_AFFLVL1
#define PLAT_MAX_OFF_STATE		U(2)
#define PLAT_MAX_RET_STATE		U(1)

/* Interrupt controller */
#define MV200H_GICD_BASE		GICD_BASE
#define MV200H_GICC_BASE		GICC_BASE

#define MV200H_G1S_IRQ_PROPS(grp) \
	INTR_PROP_DESC(HISI_IRQ_SEC_SGI_0, GIC_HIGHEST_SEC_PRIORITY, grp, \
			GIC_INTR_CFG_LEVEL), \
	INTR_PROP_DESC(HISI_IRQ_SEC_SGI_1, GIC_HIGHEST_SEC_PRIORITY, grp, \
			GIC_INTR_CFG_LEVEL), \
	INTR_PROP_DESC(HISI_IRQ_SEC_SGI_2, GIC_HIGHEST_SEC_PRIORITY, grp, \
			GIC_INTR_CFG_LEVEL), \
	INTR_PROP_DESC(HISI_IRQ_SEC_SGI_3, GIC_HIGHEST_SEC_PRIORITY, grp, \
			GIC_INTR_CFG_LEVEL), \
	INTR_PROP_DESC(HISI_IRQ_SEC_SGI_4, GIC_HIGHEST_SEC_PRIORITY, grp, \
			GIC_INTR_CFG_LEVEL), \
	INTR_PROP_DESC(HISI_IRQ_SEC_SGI_5, GIC_HIGHEST_SEC_PRIORITY, grp, \
			GIC_INTR_CFG_LEVEL), \
	INTR_PROP_DESC(HISI_IRQ_SEC_SGI_6, GIC_HIGHEST_SEC_PRIORITY, grp, \
			GIC_INTR_CFG_LEVEL), \
	INTR_PROP_DESC(HISI_IRQ_SEC_SGI_7, GIC_HIGHEST_SEC_PRIORITY, grp, \
			GIC_INTR_CFG_LEVEL), \
	INTR_PROP_DESC(HISI_IRQ_SEC_TIMER0, GIC_HIGHEST_SEC_PRIORITY, grp, \
			GIC_INTR_CFG_LEVEL), \
	INTR_PROP_DESC(HISI_IRQ_SEC_TIMER1, GIC_HIGHEST_SEC_PRIORITY, grp, \
			GIC_INTR_CFG_LEVEL), \
	INTR_PROP_DESC(HISI_IRQ_SEC_TIMER2, GIC_HIGHEST_SEC_PRIORITY, grp, \
			GIC_INTR_CFG_LEVEL), \
	INTR_PROP_DESC(HISI_IRQ_SEC_TIMER3, GIC_HIGHEST_SEC_PRIORITY, grp, \
			GIC_INTR_CFG_LEVEL)

#define MV200H_G0_IRQ_PROPS(grp)

#endif /* PLATFORM_DEF_H */
