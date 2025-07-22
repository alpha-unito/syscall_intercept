/*
 * Copyright 2025, University of Turin
 * Copyright 2016-2024, Intel Corporation
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
.globl intercept_wrapper
.type intercept_wrapper, @function

/* the C function in intercept.c */
.globl intercept_routine
.type intercept_routine, @function

/* the other C function in intercept.c, called right after cloning a thread */
.globl intercept_routine_post_clone
.type intercept_routine_post_clone, @function

.globl is_v_extension_supported

.text

intercept_wrapper:
    /*
     * Decreasing sp to allocate needed space to save caller-saved registers but
     * t5 and t6 which are already clobbered, original sp value around the ecall
     * and patch_desc pointer
     */
    addi sp, sp, -288

    /*
     * Saving temporary and function arguments registers, which are both
     * caller-saved onto the stack
     */
    sd ra, 0(sp)
    sd t0, 8(sp)
    sd t1, 16(sp)
    sd t2, 24(sp)
    sd t3, 32(sp)
    sd t4, 40(sp)

    /* fetching original value of sp */
    ld t6, 288(sp)
    sd t6, 48(sp)

    /* fetching pointer to patch_desc */
    ld t6, 296(sp)
    sd t6, 56(sp)

    sd a0, 64(sp)
    sd a1, 72(sp)
    sd a2, 80(sp)
    sd a3, 88(sp)
    sd a4, 96(sp)
    sd a5, 104(sp)
    sd a6, 112(sp)
    sd a7, 120(sp)
    fsd ft0, 128(sp)
    fsd ft1, 136(sp)
    fsd ft2, 144(sp)
    fsd ft3, 152(sp)
    fsd ft4, 160(sp)
    fsd ft5, 168(sp)
    fsd ft6, 176(sp)
    fsd ft7, 184(sp)
    fsd ft8, 192(sp)
    fsd ft9, 200(sp)
    fsd ft10, 208(sp)
    fsd ft11, 216(sp)
    fsd fa0, 224(sp)
    fsd fa1, 232(sp)
    fsd fa2, 240(sp)
    fsd fa3, 248(sp)
    fsd fa4, 256(sp)
    fsd fa5, 264(sp)
    fsd fa6, 272(sp)
    fsd fa7, 280(sp)

    /* argument passed to intercept_routine */
    mv a0, sp

    li t0, 1
    beq t5, t0, l0 # which function should be called?

    jal intercept_routine
    j l1
l0:
    jal intercept_routine_post_clone
l1:
    /*
     * At this point, the return value of the C
     * function (a struct wrapper_ret instance) is in a0, a1.
     *
     * This function doesn't use these values for anything, just
     * forwards them to the higher level wrapper function, generated
     * from the template.
     */
    mv t6, a1

    /* Restoring temporary and funcion arguments registers */
    ld ra, 0(sp)
    ld t0, 8(sp)
    ld t1, 16(sp)
    ld t2, 24(sp)
    ld t3, 32(sp)
    ld t4, 40(sp)

    /*
     * No point in restoring in context values at 48(sp) and 56(sp), while a0
     * must be restored only if system call has not been handled yet
     */
    li a1, 1
    beq t6, a1, handled_ecall
    ld a0, 64(sp)
handled_ecall:
    ld a1, 72(sp)
    ld a2, 80(sp)
    ld a3, 88(sp)
    ld a4, 96(sp)
    ld a5, 104(sp)
    ld a6, 112(sp)
    ld a7, 120(sp)
    fld ft0, 128(sp)
    fld ft1, 136(sp)
    fld ft2, 144(sp)
    fld ft3, 152(sp)
    fld ft4, 160(sp)
    fld ft5, 168(sp)
    fld ft6, 176(sp)
    fld ft7, 184(sp)
    fld ft8, 192(sp)
    fld ft9, 200(sp)
    fld ft10, 208(sp)
    fld ft11, 216(sp)
    fld fa0, 224(sp)
    fld fa1, 232(sp)
    fld fa2, 240(sp)
    fld fa3, 248(sp)
    fld fa4, 256(sp)
    fld fa5, 264(sp)
    fld fa6, 272(sp)
    fld fa7, 280(sp)

    /* Increasing sp to its original value */
    addi sp, sp, 288
    ret
