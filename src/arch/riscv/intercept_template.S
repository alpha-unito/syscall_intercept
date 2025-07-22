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
 * intercept_template.s -- see asm_wrapper.md
 */

.globl intercept_asm_wrapper_tmpl
.globl intercept_asm_wrapper_patch_desc_addr
.globl intercept_asm_wrapper_wrapper_level1_addr
.globl intercept_asm_wrapper_tmpl_end

.text

/*
 * preceding_ins_2
 * preceding_ins
 */
intercept_asm_wrapper_tmpl:
    addi sp, sp, -72
    addi t6, sp, 72 # sp+72 is the sp value before interception
    sd t6, 0(sp) # original sp on stack
    sd ra, 16(sp) # libc won't need to restore ra after just an ecall
    sd t0, 24(sp)
    sd t1, 32(sp)
    sd t2, 40(sp)
    sd t3, 48(sp)
    sd t4, 56(sp)
    sd t5, 64(sp)
    /*
     * t5 content will be evaluated in intercept_wrapper to determine which
     * interception routine must be called. 1 for post_clone, 0 otherwise
     */
    li t5, 0 # choose intercept_routine
    j intercept_asm_wrapper_patch_desc_addr
post_clone:
    addi sp, sp, -72
    addi t6, sp, 72 # sp+72 is the sp value before interception
    sd t6, 0(sp)
    sd ra, 16(sp)
    sd t0, 24(sp)
    sd t1, 32(sp)
    sd t2, 40(sp)
    sd t3, 48(sp)
    sd t4, 56(sp)
    sd t5, 64(sp)
    li t5, 1 # choose intercept_routine_post_clone
intercept_asm_wrapper_patch_desc_addr:
    /* load patch_desc address in t0 */

    /*
     * the 5 following instructions are an indication for the actual ones which
     * are written in binary by create_load_uint64t_into_t0() in patcher.c:528
     *
     * lui t0, 0x00000
     * addi t0, t0, 0x000
     * slli t0, t0, 32
     * lui t3, 0x00000
     * addi t3, t3, 0x000
     */
    .space 20, 0

    /* the 9 following instructions compose the 64-bit address in t0 */
    lui t1, 0x7ffff
    ori t2, zero, 0x7ff
    slli t2, t2, 1
    ori t2, t2, 1
    or t1, t1, t2
    slli t1, t1, 1
    ori t1, t1, 1
    and t3, t3, t1
    or t0, t0, t3

    sd t0, 8(sp) /* patch_desc pointer on stack */
intercept_asm_wrapper_wrapper_level1_addr:
    /* load intercept_wrapper address in t0 */

    /*
     * the 5 following instructions are an indication for the actual ones which
     * are written in binary by create_load_uint64t_into_t0() in patcher.c:529
     *
     * lui t0, 0x00000
     * ori t0, t0, 0x000
     * slli t0, t0, 32
     * lui t3, 0x00000
     * ori t3, t3, 0x000
     *
     */
    .space 20, 0

    /* the 9 following instructions compose the 64-bit address in t0 */
    lui t1, 0x7ffff
    ori t2, zero, 0x7ff
    slli t2, t2, 1
    ori t2, t2, 1
    or t1, t1, t2
    slli t1, t1, 1
    ori t1, t1, 1
    and t3, t3, t1
    or t0, t0, t3

    /* calling intercept_wrapper */
    jalr ra, t0, 0

    /*
     * restoring registers used before the intercept_wrapper function could
     * save them on the stack
     */
    ld ra, 16(sp)
    ld t0, 24(sp)
    ld t1, 32(sp)
    ld t2, 40(sp)
    ld t3, 48(sp)
    ld t4, 56(sp)
    ld t5, 64(sp)
    addi sp, sp, 72

    /*
     * The intercept_wrapper function did restore all registers to their
     * original state, except for a0, sp, t5 and t6. sp and t5 have just been
     * restored up here (lines 145-146).
     *
     * If t6 is zero, a0 contains a syscall number, and that syscall
     * is executed here.
     * If t6 is 1, a0 contains the return value of the hooked syscall.
     * If t6 is 2, a clone syscall is executed here.
     */
    beqz t6, execute_unhandled_ecall
    srli t6, t6, 1
    beqz t6, handled_ecall
    srli t6, t6, 1
    beqz t6, execute_clone
    ebreak # if t6 holds invalid value, i. e. different from 0, 1 or 2

execute_clone:
    ecall
    j post_clone

execute_unhandled_ecall:
    ecall
    j intercept_asm_wrapper_tmpl_end
handled_ecall:

intercept_asm_wrapper_tmpl_end:
    /*
     * following_ins
     *
     * This template must be appended here with a
     * jump back to the intercepted code, whic is written by
     * create_absolute_jump(*dst, patch->return_address) in patcher.c:537
     *
     */
