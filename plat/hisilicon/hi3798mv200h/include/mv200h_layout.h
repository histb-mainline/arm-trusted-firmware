/*
 * Copyright (c) 2026, David Yang <mmyangfl@gmail.com>.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MV200H_LAYOUT_H
#define MV200H_LAYOUT_H

/*
 * RAM layout for the tftp + `go` boot flow (vendor fastboot is the loader and
 * is not modified).  fastboot reports free DDR at base 0x01000000; the tramp
 * layout below reuses the addresses the trampoline rounds already proved.
 *
 *   0x02000000  stage1 (ARM32 `go` target, kept out of tree in
 *               mv200h-work/tramp; arms the boot-alive SP805 and writes
 *               RVBAR = MV200H_BL31_BASE)
 *   0x02001000  BL31 (this image, RESET_TO_BL31).  The address is a contract:
 *               it is the word stage1 writes into the cluster reset vector, and
 *               the SCS vector never reads back on this part, so the standing
 *               check is that bl31.elf's e_entry equals it -- which holds only
 *               while bl31_entrypoint stays the first emitted function.
 *   0x02016000  DTB (the stage1 blob carries it; BL31 hands its address to
 *               the kernel in x0)
 *   0x08000000  BL33 = arm64 kernel Image (2 MiB aligned, booting.rst); this
 *               is the default of the upstream PRELOADED_BL33_BASE build option
 *
 * The kernel DT must /memreserve/ the BL31 region: BL31 stays resident at EL3
 * and its .data/.bss/.stacks live in these pages for the whole kernel run.
 */

#define MV200H_BL31_BASE		U(0x02001000)
#define MV200H_BL31_SIZE		U(0x00014000)	/* poplar's slot */
#define MV200H_DTB_BASE			U(0x02016000)	/* past BL31_LIMIT */
#define MV200H_DTB_MAX_SIZE		U(0x00010000)	/* >= dtb */

#endif /* MV200H_LAYOUT_H */

