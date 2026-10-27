#
# Copyright (c) 2026, David Yang <mmyangfl@gmail.com>.
# Based on plat/hisilicon/poplar.
#
# SPDX-License-Identifier: BSD-3-Clause
#
# BL31-only port: no BL1, no BL2 and no FIP.  The unmodified vendor
# fastboot tftp-loads stage1 (ARM32), bl31.bin, the kernel Image and a DTB,
# and stage1 warm-resets the boot core into bl31_entrypoint through the
# cluster reset vector.  Build:
#
#   make PLAT=hi3798mv200h RESET_TO_BL31=1 CROSS_COMPILE=aarch64-linux-gnu- bl31
#

COLD_BOOT_SINGLE_CPU		:= 1
PROGRAMMABLE_RESET_ADDRESS	:= 1

# There is no BL2 to hand over a BL33 entry point, so the kernel Image address
# is a build-time constant.  The main Makefile turns PRELOADED_BL33_BASE into
# the -D of the same name (and skips it in the FIP), so only the default needs
# setting here; override it on the command line to boot an Image elsewhere.
PRELOADED_BL33_BASE			?= 0x08000000
CTX_INCLUDE_FPREGS		:= 1
ERRATA_A53_855873		:= 1
ERRATA_A53_835769		:= 1
ERRATA_A53_843419		:= 1
ENABLE_SVE_FOR_NS		:= 0
WORKAROUND_CVE_2017_5715	:= 0

PLAT_INCLUDES	:=	-Iplat/hisilicon/hi3798mv200h/include		\
			-Iplat/hisilicon/hi3798mv200h

PLAT_BL_COMMON_SOURCES	:=						\
		lib/xlat_tables/aarch64/xlat_tables.c			\
		lib/xlat_tables/xlat_tables_common.c			\
		drivers/delay_timer/generic_delay_timer.c		\
		drivers/arm/gic/common/gic_common.c			\
		drivers/arm/gic/v2/gicv2_helpers.c			\
		drivers/delay_timer/delay_timer.c			\
		drivers/arm/pl011/aarch64/pl011_console.S		\
		drivers/arm/gic/v2/gicv2_main.c				\
		plat/common/plat_gicv2.c				\
		plat/hisilicon/hi3798mv200h/aarch64/hisi_helpers.S	\
		plat/hisilicon/hi3798mv200h/aarch64/platform_common.c

BL31_SOURCES	+=							\
		drivers/arm/sp805/sp805.c				\
		lib/cpus/aarch64/aem_generic.S				\
		lib/cpus/aarch64/cortex_a53.S				\
		plat/common/plat_psci_common.c				\
		plat/hisilicon/hi3798mv200h/bl31_plat_setup.c		\
		plat/hisilicon/hi3798mv200h/plat_topology.c		\
		plat/hisilicon/hi3798mv200h/plat_pm.c			\
		plat/hisilicon/hi3798mv200h/hisi_gicv2.c