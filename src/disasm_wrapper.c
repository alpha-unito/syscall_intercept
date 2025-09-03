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
 * disasm_wrapper.c -- connecting the interceptor code
 * to the disassembler code from the capstone project.
 *
 * See:
 * http://www.capstone-engine.org/lang_c.html
 */

#include "intercept.h"
#include "intercept_util.h"
#include "disasm_wrapper.h"

#include "capstone_wrapper.h"

#if defined(__x86_64__) || defined(_M_X64)
	#define CS_ARCH CS_ARCH_X86
	#define CS_MODE CS_MODE_64
#elif defined(__riscv)
	#define CS_ARCH CS_ARCH_RISCV
	#define CS_MODE CS_MODE_RISCV64 | CS_MODE_RISCVC
#elif defined(__aarch64__) || defined(_M_ARM64)
	#define CS_ARCH CS_ARCH_ARM64
	#define CS_MODE CS_MODE_LITTLE_ENDIAN
#else
	#error "Unsupported ISA"
#endif

struct intercept_disasm_context {
	csh handle;
	cs_insn *insn;
	const unsigned char *begin;
	const unsigned char *end;
};

/*
 * nop_vsnprintf - A dummy function, serving as a callback called by
 * the capstone implementation. The syscall_intercept library never makes
 * any use of string representation of instructions, but there seems to no
 * trivial way to use disassemble using capstone without it spending time
 * on printing syscalls. This seems to be the most that can be done in
 * this regard i.e. providing capstone with nop implementation of vsnprintf.
 */
static int
nop_vsnprintf()
{
	return 0;
}

/*
 * intercept_disasm_init -- should be called before disassembling a region of
 * code. The context created contains the context capstone needs ( or generally
 * the underlying disassembling library, if something other than capstone might
 * be used ).
 *
 * One must pass this context pointer to intercept_disasm_destroy following
 * a disassembling loop.
 */
struct intercept_disasm_context *
intercept_disasm_init(const unsigned char *begin, const unsigned char *end)
{
	struct intercept_disasm_context *context;

	context = xmmap_anon(sizeof(*context));
	context->begin = begin;
	context->end = end;

	/*
	 * Initialize the disassembler.
	 * The handle here must be passed to capstone each time it is used.
	 */
	if (cs_open(CS_ARCH, CS_MODE, &context->handle) != CS_ERR_OK)
		xabort("cs_open");

	/*
	 * Kindly ask capstone to return some details about the instruction.
	 * Without this, it only prints the instruction, and we would need
	 * to parse the resulting string.
	 */
	if (cs_option(context->handle, CS_OPT_DETAIL, CS_OPT_ON) != 0)
		xabort("cs_option - CS_OPT_DETAIL");

	/*
	 * Overriding the printing routine used by capstone,
	 * see comments above about nop_vsnprintf.
	 */
	cs_opt_mem x = {
		.malloc = malloc,
		.free = free,
		.calloc = calloc,
		.realloc = realloc,
		.vsnprintf = nop_vsnprintf};
	if (cs_option(context->handle, CS_OPT_MEM, (size_t)&x) != 0)
		xabort("cs_option - CS_OPT_MEM");

	if ((context->insn = cs_malloc(context->handle)) == NULL)
		xabort("cs_malloc");

	return context;
}

/*
 * intercept_disasm_destroy -- see comments for above routine
 */
void
intercept_disasm_destroy(struct intercept_disasm_context *context)
{
	cs_free(context->insn, 1);
	cs_close(&context->handle);
	xmunmap(context, sizeof(*context));
}
