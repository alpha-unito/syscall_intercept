/*
 * Copyright 2016-2017, Intel Corporation
 * intercept_wrapper.S COPYRIGHT FUJITSU LIMITED 2019
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
 * intercept_wrapper.s -- see asm_wrapper.md
 */

/* the function in this file */
.global intercept_wrapper
.hidden intercept_wrapper
.type intercept_wrapper, @function

/* the C function in intercept.c */
.global intercept_routine
.hidden intercept_routine
.type intercept_routine, @function

/* the other C function in intercept.c, called right after cloning a thread */
.global intercept_routine_post_clone
.hidden intercept_routine_post_clone
.type intercept_routine_post_clone, @function

.text

/*
 * Local stack layout:
 *  [sp + 0x00] -- fp(x29)
 *  [sp + 0x08] -- lr(x30)
 *  [sp + 0x10] -- struct context
 *
 * Arguments recieved on stack:
 *  [sp + 0x120] -- original value of fp(x29)
 *  [sp + 0x128] -- original value of lr(x30)
 *  [sp + 0x130] -- original value of x10
 *  [sp + 0x138] -- original value of x11
 *  [sp + 0x140] -- original value of sp
 *  [sp + 0x148] -- pointer to a struct patch_desc instance
 *
 * A pointer to these saved register is passed to intercept_routine, so the
 * layout of `struct context` must match this part of the stack layout.
 *
 * Other arguments:
 * x10  -- which C function to call
 */
intercept_wrapper:
	stp  x29, x30, [sp, #-(16 + (8 + 8 * 32) + 8)]! /* allocate stack for stack frame and locals */

	stp  x0, x1, [sp, #(16 + (8 + 8 * 0))]
	stp  x2, x3, [sp, #(16 + (8 + 8 * 2))]
	stp  x4, x5, [sp, #(16 + (8 + 8 * 4))]
	stp  x6, x7, [sp, #(16 + (8 + 8 * 6))]
	stp  x8, x9, [sp, #(16 + (8 + 8 * 8))]

	ldp  x0, x1, [sp, #(16 + (8 + 8 * 32) + 8 + 0x00)]  /* fetch original value of x29,x30 */
	ldp  x2, x3, [sp, #(16 + (8 + 8 * 32) + 8 + 0x10)]  /* fetch original value of x10,x11 */
	ldp  x4, x5, [sp, #(16 + (8 + 8 * 32) + 8 + 0x20)]  /* fetch original value of sp, pointer to patch_desc */

	stp   x2,  x3, [sp, #(16 + (8 + 8 * 10))]
	stp  x12, x13, [sp, #(16 + (8 + 8 * 12))]
	stp  x14, x15, [sp, #(16 + (8 + 8 * 14))]
	stp  x16, x17, [sp, #(16 + (8 + 8 * 16))]
	stp  x18, x19, [sp, #(16 + (8 + 8 * 18))]
	stp  x20, x21, [sp, #(16 + (8 + 8 * 20))]
	stp  x22, x23, [sp, #(16 + (8 + 8 * 22))]
	stp  x24, x25, [sp, #(16 + (8 + 8 * 24))]
	stp  x26, x27, [sp, #(16 + (8 + 8 * 26))]
	stp  x28,  x0, [sp, #(16 + (8 + 8 * 28))]
	stp   x1,  x4, [sp, #(16 + (8 + 8 * 30))]
	str   x5, [sp, #16]

	/* argument passed to intercept_routine */
	add  x0, sp, #16
	cmp  x10, #0x0  /* which function should be called? */
	b.ne 0f
	bl   intercept_routine
	b    1f
0:	bl   intercept_routine_post_clone
1:
	/*
	 * At this point, the return value of the C
	 * function (a struct wrapper_ret instance) is in x0, x1.
	 *
	 * This function doesn't use these values for anything, just
	 * forwards them to the higher level wrapper function, generated
	 * from the template.
	 */
	mov  x10, x1

	/*
	 * At this point, the return values of this asm function
	 * are in x0, x10.
	 *
	 * Restore the other registers, and return.
	 */
	ldr  x1,       [sp, #(16 + (8 + 8 * 1))]
	ldp  x2, x3,   [sp, #(16 + (8 + 8 * 2))]
	ldp  x4, x5,   [sp, #(16 + (8 + 8 * 4))]
	ldp  x6, x7,   [sp, #(16 + (8 + 8 * 6))]
	ldp  x8, x9,   [sp, #(16 + (8 + 8 * 8))]
	ldr  x11,      [sp, #(16 + (8 + 8 * 11))]
	ldp  x12, x13, [sp, #(16 + (8 + 8 * 12))]
	ldp  x14, x15, [sp, #(16 + (8 + 8 * 14))]
	ldp  x16, x17, [sp, #(16 + (8 + 8 * 16))]
	ldp  x18, x19, [sp, #(16 + (8 + 8 * 18))]
	ldp  x20, x21, [sp, #(16 + (8 + 8 * 20))]
	ldp  x22, x23, [sp, #(16 + (8 + 8 * 22))]
	ldp  x24, x25, [sp, #(16 + (8 + 8 * 24))]
	ldp  x26, x27, [sp, #(16 + (8 + 8 * 26))]
	ldr  x28,      [sp, #(16 + (8 + 8 * 28))]

	ldp  x29, x30, [sp], #(16 + (8 + 8 * 32) + 8)
	ret
