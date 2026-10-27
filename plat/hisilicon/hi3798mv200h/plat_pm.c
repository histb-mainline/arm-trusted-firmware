/*
 * Copyright (c) 2026, David Yang <mmyangfl@gmail.com>.
 * Based on plat/hisilicon/poplar (plat_pm.c).
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>

#include <platform_def.h>

#include <arch_helpers.h>
#include <common/bl_common.h>
#include <common/debug.h>
#include <context.h>
#include <lib/el3_runtime/context_mgmt.h>
#include <lib/mmio.h>
#include <drivers/arm/sp805.h>
#include <drivers/delay_timer.h>
#include <lib/psci/psci.h>
#include <plat/common/platform.h>

#include "hi3798mv200h.h"
#include "plat_private.h"

/*
 * The warm entrypoint PSCI handed to plat_setup_psci_ops().  It is kept here
 * because the vendor's sequence writes the cluster vector on every CPU_ON, not
 * only once at setup.
 */
static uintptr_t hisi_warm_entrypoint;

/*
 * One cluster-wide reset vector plus the AARCH64-at-reset switch, then
 * COREP0_SRST (bit 12+cpu) and CORE_SRST (bit 8+cpu): a released core comes out
 * at EL3, MMU and caches off, and lands on REG_PERI_CPU_RVBARADDR.
 * plat_setup_psci_ops() points that vector at the warm entrypoint, exactly as
 * Poplar does.  Nothing else is done to the core on its way in: CPUECTLR.SMPEN
 * is set by cortex_a53's reset handler, which el3_entrypoint_common runs on the
 * warm path too.
 */

static void hisi_cpu_standby(plat_local_state_t cpu_state)
{
	dsb();
	wfi();
}

static int hisi_pwr_domain_on(u_register_t mpidr)
{
	unsigned int cpu = plat_core_pos_by_mpidr(mpidr);
	unsigned int regval;

	/*
	 * The vendor's release (hisi_pm.c:72-116) writes AARCH_MODE and the
	 * cluster reset vector inside every single CPU_ON rather than once at
	 * setup, so do the same.
	*/

	mmio_write_32((uintptr_t)REG_PERI_CPU_AARCH_MODE,
		      REG_AARCH64_ALL_CORES);
	mmio_write_32((uintptr_t)REG_PERI_CPU_RVBARADDR, hisi_warm_entrypoint);

	/*
	 * R112: no CPU_LP frequency bracket here. The vendor's
	 * slave_cores_power_up() (hisi_pm.c:96-99, restore at :110-113) reads
	 * 0xf8a22048, forces 0x206/0x606 and writes the read value back --
	 * sound when their firmware does it once during early boot. We do it on
	 * every CPU_ON while Linux is running, so the value written back is
	 * whatever the live system left there, put back over a core that has
	 * just come out of reset. The board experiment that released core1
	 * without touching this register at all (AARCH_MODE + RVBAR + the two
	 * reset bits, and core1 then executed the stub) is what makes the
	 * bracket the suspect rather than the release itself.
	*/

	/*
	 * Assert the core reset first, for every core, then clear both of
	 * its reset bits below.  This fastboot brings the cluster up as an
	 * SMP board, so a core parked in one of its own AArch32 loops is
	 * not "in reset"; assert-then-clear is the transition stage1 uses
	 * to put the boot core into BL31, and it is idempotent when the
	 * core really was held.
	*/
	regval = mmio_read_32((uintptr_t)(REG_BASE_CRG + REG_CPU_RST));
	regval |= (U(1) << (cpu + 8));
	mmio_write_32((uintptr_t)(REG_BASE_CRG + REG_CPU_RST), regval);

	/* Clear the slave cpu arm_por_srst_req reset */
	regval = mmio_read_32((uintptr_t)(REG_BASE_CRG + REG_CPU_RST));
	regval &= ~(U(1) << (cpu + 12));
	mmio_write_32((uintptr_t)(REG_BASE_CRG + REG_CPU_RST), regval);

	/* Clear the slave cpu reset */
	regval = mmio_read_32((uintptr_t)(REG_BASE_CRG + REG_CPU_RST));
	regval &= ~(U(1) << (cpu + 8));
	mmio_write_32((uintptr_t)(REG_BASE_CRG + REG_CPU_RST), regval);

	/*
	 * The board-proven release puts a counted pause between the reset
	 * clears and the next access; without it a core may be scheduled
	 * before its reset deassert propagates.
	*/
	udelay(1);

	return PSCI_E_SUCCESS;
}

static void hisi_pwr_domain_off(const psci_power_state_t *target_state)
{
	assert(0);
}

static void hisi_pwr_domain_suspend(const psci_power_state_t *target_state)
{
	assert(0);
}

static void hisi_pwr_domain_on_finish(const psci_power_state_t *target_state)
{
	assert(target_state->pwr_domain_state[MPIDR_AFFLVL0] ==
					PLAT_MAX_OFF_STATE);

	/*
	 * R103: no GIC programming here either. This is the same write the
	 * boot-CPU path had in every round up to R98 --
	 * gicv2_pcpu_distif_init() re-groups and re-enables INTID 25..31 in the
	 * distributor's secure bank from EL3 -- and R99 showed that programming
	 * the distributor from EL3 is exactly what kept the non-secure kernel
	 * from ever seeing a delivered interrupt. On a secondary it would do
	 * that to the newly started core's own arch_timer PPI (INTID 30). The
	 * kernel does all of it itself: gic_starting_cpu() runs
	 * gic_cpu_init()/gic_cpu_dist_init() on every CPU_STARTING, which
	 * enables the interface and the per-CPU lines from the non-secure view
	 * that is the one actually wired.
	*/
}

static void hisi_pwr_domain_suspend_finish(
		const psci_power_state_t *target_state)
{
	assert(0);
}

static void __dead2 hisi_system_off(void)
{
	ERROR("mv200h System Off: operation not handled.\n");
	/* Stop watchdog0 before panic() */
	sp805_stop(HISI_WDG0_BASE);
	panic();
}

/*
 * Reset through watchdog0.  The clock must be running for the count to
 * decrement (board-proven: PERI_CRG94 bit0 is the wdg0_cken gate and a frozen
 * counter resumes the instant it is set), and stage1 plus the kernel's sp805
 * probe keep the gate open for the whole run -- but do not rely on that here:
 * CRG94 bit0 defaults to 1 at POR and nothing in the BL31 path gates it, while
 * the kernel's clk_disable_unused can only gate a clock no driver claimed, and
 * the DT for this chain keeps &wdg0 enabled.  LOAD is 100 ms at 24 MHz; the
 * kernel-side restart handler needed at least a non-zero load plus a wait
 * (LOAD=0 never fires), and 100 ms comfortably covers the flush + wfi below.
 */
static void __dead2 hisi_system_reset(void)
{
	/* The kernel's reboot path gates wdg0_cken (PERI_CRG94 bit0) before
	 * issuing SYSTEM_RESET, and a gated counter never fires (board-proven:
	 * a frozen counter resumes the instant the gate reopens).  Re-open the
	 * clock first, then load and arm -- the same two-part fix the kernel
	 * side needed (its wdt_restart alone was not enough).
	 */
	mmio_setbits_32(REG_BASE_CRG + REG_PERI_CRG94, CRG94_WDG0_CKEN);
	/*
	 * Not sp805_start(): it writes LOAD and CONTROL and only then re-locks
	 * the block, and on this part a lock write after LOAD freezes the
	 * running counter (board-measured three times: cv200's kernel keepalive
	 * path left WDTVALUE at 0 for 35 samples with CONTROL=0x3 and LOAD
	 * latched, and mv200h reproduces the freeze every round).  It also
	 * never unlocks, so it only works if the caller has already left the
	 * block unlocked.  Poplar -- the merged sibling -- does this sequence
	 * by hand too, and likewise never locks again at the end.
	 */
	mmio_write_32(HISI_WDG0_BASE + SP805_WDOG_LOCK_OFF, WDOG_UNLOCK_KEY);
	mmio_write_32(HISI_WDG0_BASE + SP805_WDOG_LOAD_OFF,
		      HISI_WDG0_RESET_LOAD);
	mmio_write_32(HISI_WDG0_BASE + SP805_WDOG_CTR_OFF,
		      SP805_CTR_INTEN | SP805_CTR_RESEN);

	wfi();
	ERROR("mv200h System Reset: operation not handled.\n");
	panic();
}

static int32_t hisi_validate_power_state(unsigned int power_state,
					 psci_power_state_t *req_state)
{
	VERBOSE("%s: power_state: 0x%x\n", __func__, power_state);

	int pstate = psci_get_pstate_type(power_state);

	assert(req_state);

	if (pstate == PSTATE_TYPE_STANDBY)
		req_state->pwr_domain_state[MPIDR_AFFLVL0] = PLAT_MAX_RET_STATE;
	else
		req_state->pwr_domain_state[MPIDR_AFFLVL0] = PLAT_MAX_OFF_STATE;

	/* We expect the 'state id' to be zero */
	if (psci_get_pstate_id(power_state))
		return PSCI_E_INVALID_PARAMS;

	return PSCI_E_SUCCESS;
}

static int hisi_validate_ns_entrypoint(uintptr_t entrypoint)
{
	/*
	 * Check if the non secure entrypoint lies within the non
	 * secure DRAM.
	*/
	if ((entrypoint >= DDR_BASE) && (entrypoint < (DDR_BASE + DDR_SIZE)))
		return PSCI_E_SUCCESS;

	return PSCI_E_INVALID_ADDRESS;
}

static void hisi_get_sys_suspend_power_state(psci_power_state_t *req_state)
{
	int i;

	for (i = MPIDR_AFFLVL0; i <= PLAT_MAX_PWR_LVL; i++)
		req_state->pwr_domain_state[i] = PLAT_MAX_OFF_STATE;
}

static const plat_psci_ops_t hisi_plat_psci_ops = {
	.cpu_standby			= hisi_cpu_standby,
	.pwr_domain_on			= hisi_pwr_domain_on,
	.pwr_domain_off			= hisi_pwr_domain_off,
	.pwr_domain_suspend		= hisi_pwr_domain_suspend,
	.pwr_domain_on_finish		= hisi_pwr_domain_on_finish,
	.pwr_domain_suspend_finish	= hisi_pwr_domain_suspend_finish,
	.system_off			= hisi_system_off,
	.system_reset			= hisi_system_reset,
	.validate_power_state		= hisi_validate_power_state,
	.validate_ns_entrypoint		= hisi_validate_ns_entrypoint,
	.get_sys_suspend_power_state	= hisi_get_sys_suspend_power_state,
};

int plat_setup_psci_ops(uintptr_t sec_entrypoint,
			const plat_psci_ops_t **psci_ops)
{
	*psci_ops = &hisi_plat_psci_ops;

	/*
	 * Point the cluster reset vector at the PSCI warm entrypoint. The cold
	 * boot core has already come through the vector stage1 wrote
	 * (bl31_entrypoint, full RESET_TO_BL31 init); from here on every
	 * released core enters the warm path directly, exactly as on Poplar.
	 * AARCH_MODE is written as the plain 0xf all four cores need to come
	 * out in AArch64.
	*/
	mmio_write_32((uintptr_t)REG_PERI_CPU_AARCH_MODE,
		      REG_AARCH64_ALL_CORES);
	hisi_warm_entrypoint = sec_entrypoint;
	mmio_write_32((uintptr_t)REG_PERI_CPU_RVBARADDR, sec_entrypoint);
	return 0;
}

