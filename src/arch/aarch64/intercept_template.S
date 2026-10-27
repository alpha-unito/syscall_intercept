/*
 * Copyright 2016-2017, Intel Corporation
 * intercept_template.S COPYRIGHT FUJITSU LIMITED 2019
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * intercept_template.s -- see asm_wrapper.md
 */

.global intercept_asm_wrapper_tmpl;
.hidden intercept_asm_wrapper_tmpl;
.global intercept_asm_wrapper_patch_desc_addr;
.hidden intercept_asm_wrapper_patch_desc_addr;
.global intercept_asm_wrapper_wrapper_level1_addr;
.hidden intercept_asm_wrapper_wrapper_level1_addr;
.global intercept_asm_wrapper_tmpl_end;
.hidden intercept_asm_wrapper_tmpl_end;

.text

/*
 * Locals on the stack:
 * [sp + 0x00] the original value of x29, in the code around the svc
 * [sp + 0x08] the original value of x30, in the code around the svc
 * [sp + 0x10] the original value of x10, in the code around the svc
 * [sp + 0x18] the original value of x11, in the code around the svc
 * [sp + 0x20] the original value of sp, in the code around the svc
 * [sp + 0x28] the pointer to the struct patch_desc instance
 *
 * The x10 register controls which C function to call in intercept.c:
 *
 * if x10 == 0 then call intercept_routine
 * if x10 == 1 then intercept_routine_post_clone
 *
 * This value in x10 is passed to the function intercep_wrapper.
 */

.macro intercept_asm_wrapper_save_regs
	stp  x29, x30, [sp, #-0x30]!  /* store x29, x30 */
	mov  x29, sp                  /* update frame pointer */
	stp  x10, x11, [sp, #0x10]    /* store x10, x11 */
	add  x10, sp, #0x30
	str  x10, [sp, #0x20]         /* store original sp */
.endm

.macro intercept_asm_wrapper_restore_regs
	ldp x10, x11, [sp, #0x10]     /* restore x10, x11 */
	ldp x29, x30, [sp], #0x30     /* restore x29, x30, sp */
.endm

intercept_asm_wrapper_tmpl:
	intercept_asm_wrapper_save_regs
	mov  x10, #0                        /* choose intercept_routine */
	b 0f
intercept_asm_wrapper_patch_desc_addr:
	.quad   0
intercept_asm_wrapper_wrapper_level1_addr:
	.quad   0
0:
	adr  x30, intercept_asm_wrapper_patch_desc_addr
	ldr  x11, [x30]
	str  x11, [sp, 0x28]                 /* store struct patch_desc address */
	adr  x30, intercept_asm_wrapper_wrapper_level1_addr
	ldr  x11, [x30]
	blr  x11                             /* call intercept_wrapper */

	/*
	 * The intercept_wrapper function did restore all registers to their
	 * original state, except for x10, x11, x29, x30, sp, pc.
	 *
	 * If x10 is zero, x8 contains a syscall number, and that syscall
	 *  is executed here.
	 * If x10 is 1, x0 contains the return value of the hooked syscall.
	 * If x10 is 2, a clone syscall is executed here.
	 */
	cmp  x10, #0x0
	b.eq 2f
	cmp  x10, #0x1
	b.eq 3f
	cmp  x10, #0x2
	b.eq 1f

	wfi  /* x10 value is invalid? */
1:
	/* execute the clone syscall in its original context */
	intercept_asm_wrapper_restore_regs
	svc  #0x0

	/*
	 * Now goto 0, and call the C function named
	 * intercept_routine_post_clone both in the parent thread, and the
	 * child thread.
	 */
	intercept_asm_wrapper_save_regs
	mov  x10, #0x1  /* choose intercept_routine_post_clone */
	b    0b
2:
	intercept_asm_wrapper_restore_regs
	svc  #0x0
	b    intercept_asm_wrapper_tmpl_end
3:
	intercept_asm_wrapper_restore_regs
intercept_asm_wrapper_tmpl_end:
	/*
	 * This template must be appended here with a
	 * jump back to the intercepted code.
	 */
