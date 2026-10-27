/*
 * Copyright (c) 2026, David Yang <mmyangfl@gmail.com>.
 * Based on plat/hisilicon/poplar.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <asm_macros.S>
#include <arch.h>
#include <platform_def.h>

	.globl	plat_my_core_pos
	.globl	hisi_calc_core_pos
	.globl	plat_crash_console_init
	.globl	plat_crash_console_putc
	.globl	plat_crash_console_flush
	.globl	platform_mem_init
	.globl	plat_reset_handler

	/* ---------------------------------------------------------------------
	 * Reset vector dispatch, mv200h shape: stage1 (ARM32, out of tree)
	 * writes REG_PERI_CPU_RVBARADDR = bl31_entrypoint and warm-resets the
	 * boot core into this image, so the cold path needs no dispatch -- the
	 * cluster vector is rewritten to the PSCI warm entrypoint by
	 * plat_setup_psci_ops() once BL31 is up (same scheme as Poplar).  The
	 * A53 cluster has one shared reset vector, board-proven: a released
	 * core starts at 0xf8a80034's word whatever else is written.
	 *
	 * SCR_EL3 is deliberately NOT written here: TF-A's one-shot SCR write
	 * in el3_arch_init_common would lose SCR_EL3.RW (this part keeps RW
	 * RES0 while EL2 is disabled -- trampoline attempt 9).  Instead the
	 * HCE pre-write happens in plat_reset_handler below, which runs before
	 * el3_arch_init_common on both the cold and the warm path; with EL2
	 * enabled by HCE, the one-shot write lands RW (attempt 9's re-probe).
	 * HCR_EL2.RW, which the kernel needs for its EL1 handoff, is also set
	 * in plat_reset_handler -- nothing else: other HCR_EL2 bits trap EL1
	 * register access to EL2 and would kill the kernel for an unrelated
	 * reason.
	 * ---------------------------------------------------------------------
	*/
func plat_reset_handler
	mov	x0, #SCR_HCE_BIT
	msr	scr_el3, x0
	mov_imm	x1, HCR_RW_BIT
	msr	hcr_el2, x1
	isb
	ret
endfunc plat_reset_handler

	/* ---------------------------------------------------------------------
	 * unsigned int plat_my_core_pos(void)
	 * With this function: CorePos = (ClusterId * 4) + CoreId
	 * ---------------------------------------------------------------------
	*/
func plat_my_core_pos
	mrs	x0, mpidr_el1
	b	hisi_calc_core_pos
endfunc plat_my_core_pos

func hisi_calc_core_pos
	and	x1, x0, #MPIDR_CPU_MASK
	and	x0, x0, #MPIDR_CLUSTER_MASK
	add	x0, x1, x0, LSR #6
	ret
endfunc hisi_calc_core_pos

	/* ---------------------------------------------
	 * int plat_crash_console_init(void)
	 * Clobber list : x0 - x4
	 * ---------------------------------------------
	*/
func plat_crash_console_init
	mov_imm	x0, MV200H_CRASH_UART_BASE
	mov_imm	x1, MV200H_CRASH_UART_CLK_IN_HZ
	mov_imm	x2, MV200H_CONSOLE_BAUDRATE
	b	console_pl011_core_init
endfunc plat_crash_console_init

func plat_crash_console_putc
	mov_imm	x1, MV200H_CRASH_UART_BASE
	b	console_pl011_core_putc
endfunc plat_crash_console_putc

func plat_crash_console_flush
	mov_imm	x0, MV200H_CRASH_UART_BASE
	b	console_pl011_core_flush
endfunc plat_crash_console_flush

	/* ---------------------------------------------------------------------
	 * No platform memory init needed: DRAM was brought up and scrubbed by
	 * the vendor fastboot long before stage1.  The stale dirty L2 lines
	 * that survive this SoC's warm reset (the "cisw dance" the trampoline
	 * performs in stage2) are stage1's job here: it must clean+invalidate
	 * L1+L2 in AArch32 before the wfi, while its caches are off, so that
	 * neither BL31's own data nor the tftp'd DTB can be shadowed by lines
	 * from a previous run.  BL31 itself only reads memory it wrote first.
	 * ---------------------------------------------------------------------
	*/
func platform_mem_init
	ret
endfunc platform_mem_init

