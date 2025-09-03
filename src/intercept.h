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
 * intercept.h - a few declarations used in libsyscall_intercept
 */

#ifndef INTERCEPT_INTERCEPT_H
#define INTERCEPT_INTERCEPT_H

#include <stdbool.h>
#include <elf.h>
#include <unistd.h>
#include <dlfcn.h>
#include <link.h>

#include "disasm_wrapper.h"

extern bool debug_dumps_on;
void debug_dump(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

#define INTERCEPTOR_EXIT_CODE 111

__attribute__((noreturn)) void xabort_errno(int error_code, const char *msg);

__attribute__((noreturn)) void xabort(const char *msg);

void xabort_on_syserror(long syscall_result, const char *msg);

struct syscall_desc {
	int nr;
	long args[6];
};

struct range {
	unsigned char *address;
	size_t size;
};

/*
 * The patch_list array stores some information on
 * whereabouts of patches made to glibc.
 * The syscall_addr pointer points to where a syscall
 *  instruction originally resided in glibc.
 * The asm_wrapper pointer points to the function
 *  called from glibc.
 * The glibc_call_patch pointer points to the exact
 *  location, where the new call instruction should
 *  be written.
 */
struct patch_desc {
	/* the original syscall instruction */
	unsigned char *syscall_addr;

	const char *containing_lib_path;

	/* the offset of the original syscall instruction */
	unsigned long syscall_offset;

	/* the new asm wrapper created */
	unsigned char *asm_wrapper;

	/* the first byte overwritten in the code */
	unsigned char *dst_jmp_patch;

	/* the address to jump back to */
	unsigned char *return_address;

	/*
	 * Describe up to three instructions surrounding the original
	 * syscall instructions. Sometimes just overwritting the two
	 * direct neighbors of the syscall is not enough, ( e.g. if
	 * both the directly preceding, and the directly following are
	 * single byte instruction, that only gives 4 bytes of space ).
	 */
	struct intercept_disasm_result preceding_ins_2;
	struct intercept_disasm_result preceding_ins;
	struct intercept_disasm_result following_ins;
	bool uses_prev_ins_2;
	bool uses_prev_ins;
	bool uses_next_ins;

	bool uses_nop_trampoline;

	/*
	 * Flag marking if the patch has just 6 relocatable bytes. If true
	 * an auipc+c.jalr sequence is needed to perform a jump to the trampoline
	 */
	bool needs_compressed_ins;

	/*
	 * Flag marking if the patch is 10 bytes long. If true, two 32 bits
	 * instructions and one compressed 16 bit instruction has been selected
	 * for patching, but since 8 bytes are enough a 2 byte padding is needed.
	 * It will be provided by a c.nop instruction. If compressed instructions
	 * are not supported this will never happen since the patch can just be 8
	 * bytes long (or 4, in case of failure)
	 */
	bool padding_is_needed;

	struct range nop_trampoline;
};

/*
 * A section_list struct contains information about sections where
 * libsyscall_intercept looks for jump destinations among symbol addresses.
 * Generally, only two sections are used for this, so 16 should be enough
 * for the maximum number of headers to be stored.
 *
 * See the calls to the add_table_info routine in the intercept_desc.c source
 * file.
 */
struct section_list {
	Elf64_Half count;
	Elf64_Shdr headers[0x10];
};

struct intercept_desc {

	/*
	 * uses_trampoline_table - For now this is decided runtime
	 * to make it easy to compare the operation of the library
	 * with and without it. If it is OK, we can remove this
	 * flag, and just always use the trampoline table.
	 */
	bool uses_trampoline_table;

	/*
	 * delta between vmem addresses and addresses in symbol tables,
	 * non-zero for dynamic objects
	 */
	unsigned char *base_addr;

	/* where the object is in fs */
	const char *path;

	/*
	 * Some sections of the library from which information
	 * needs to be extracted.
	 * The text section is where the code to be hotpatched
	 * resides.
	 * The symtab, and dynsym sections provide information on
	 * the whereabouts of symbols, whose address in the text
	 * section.
	 */
	Elf64_Half text_section_index;
	Elf64_Shdr sh_text_section;

	struct section_list symbol_tables;
	struct section_list rela_tables;

	/* Where the text starts inside the shared object */
	unsigned long text_offset;

	/*
	 * Where the text starts and ends in the virtual memory seen by the
	 * current process.
	 */
	unsigned char *text_start;
	unsigned char *text_end;


	struct patch_desc *items;
	unsigned count;
	unsigned char *jump_table;

	size_t nop_count;
	size_t max_nop_count;
	struct range *nop_table;

	unsigned char *trampoline_table;
	size_t trampoline_table_size;

	unsigned char *next_trampoline;
};

bool has_jump(const struct intercept_desc *desc, unsigned char *addr);
void mark_jump(const struct intercept_desc *desc, const unsigned char *addr);

void allocate_trampoline_table(struct intercept_desc *desc);
void find_syscalls(struct intercept_desc *desc);

void init_patcher(void);
#if defined(__x86_64__) || defined(_M_X64) || defined(__riscv)
	void create_patch_wrappers(struct intercept_desc *desc, unsigned char **dst);
#else
	void create_patch_wrappers(struct intercept_desc *desc);
#endif
void mprotect_asm_wrappers(void);

/*
 * Actually overwrite instructions in glibc.
 */
void activate_patches(struct intercept_desc *desc);

#if defined(__x86_64__) || defined(_M_X64)
	#define PARAM_BY_ARCH(opt1, opt2, opt3) opt1
	#define CALL_OPCODE 0xe8
	#define JMP_OPCODE 0xe9
	#define SHORT_JMP_OPCODE 0xeb
	#define PUSH_IMM_OPCODE 0x68
	#define NOP_OPCODE 0x90
	#define INT3_OPCODE 0xCC
#elif defined(__riscv)
	#define PARAM_BY_ARCH(opt1, opt2, opt3) opt2
#elif defined(__aarch64__) || defined(_M_ARM64)
	#define PARAM_BY_ARCH(opt1, opt2, opt3) opt3
	#define INSTRUCTION_SIZE 4
#else
	#error "Unsupported ISA"
#endif

#define SYSCALL_INS_SIZE PARAM_BY_ARCH(2,4,4)
#define JUMP_INS_SIZE PARAM_BY_ARCH(5,8,4)
#define SYSCALL_NR PARAM_BY_ARCH(context->rax,context->a[7],context->x8)
#define THREAD_PID PARAM_BY_ARCH(context->rax,context->a[0],context->x0)
#define FIRST_ARG_REG PARAM_BY_ARCH(context->rdi,context->a[0],context->x0)
#define SECOND_ARG_REG PARAM_BY_ARCH(context->rsi,context->a[1],context->x1)
#define THIRD_ARG_REG PARAM_BY_ARCH(context->rdx,context->a[2],context->x2)
#define FOURTH_ARG_REG PARAM_BY_ARCH(context->r10,context->a[3],context->x3)
#define FIFTH_ARG_REG PARAM_BY_ARCH(context->r8,context->a[4],context->x4)
#define SIXTH_ARG_REG PARAM_BY_ARCH(context->r9,context->a[5],context->x5)
#define FIRST_RET_REG PARAM_BY_ARCH(.rax,.a[0],.x0)
#define SECOND_RET_REG PARAM_BY_ARCH(.rdx,.a[1],.x1)
#define ABS_MAX_NEG_OFFSET PARAM_BY_ARCH(INT32_MAX,((long)INT32_MAX+0x800))
#define ABS_MAX_POS_OFFSET PARAM_BY_ARCH(INT32_MAX,INT32_MAX-0x801)

bool is_overwritable_nop(const struct intercept_disasm_result *ins);

#if defined(__x86_64__) || defined(_M_X64)
	void create_jump(unsigned char opcode, unsigned char *from, void *to);
#elif defined(__aarch64__) || defined(_M_ARM64)
	unsigned char *create_jump(unsigned char *from, void *to);
	extern size_t page_size;
#endif

extern const char *cmdline;

#define PAGE_SIZE ((size_t)0x1000)

#if !defined(__aarch64__) && !defined(_M_ARM64)
	static inline unsigned char *
	round_down_address(unsigned char *address)
	{
		return (unsigned char *)(((uintptr_t)address) & ~(PAGE_SIZE - 1));
	}
#endif

/* The size of an asm wrapper instance */
extern size_t asm_wrapper_tmpl_size;

#endif
