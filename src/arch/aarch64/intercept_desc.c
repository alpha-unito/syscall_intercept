/*
 * Copyright 2016-2017, Intel Corporation
 * intercept_desc.c COPYRIGHT FUJITSU LIMITED 2019
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

#include <assert.h>
#include <syscall.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

#include <intercept.h>
#include <intercept_util.h>
#include <disasm_wrapper.h>

/*
 * open_orig_file
 *
 * Instead of looking for the needed metadata in already mmap library,
 * all this information is read from the file, thus its original place,
 * the file where the library is in an FS. The loaded library is mmaped
 * already of course, but not necessarily the whole file is mapped as one
 * readable mem mapping -- only some segments are present in memory, but
 * information about the file's sections, and the sections themselves might
 * only be present in the original file.
 * Note on naming: memory has segments, the object file has sections.
 */
static int
open_orig_file(const struct intercept_desc *desc)
{
	int fd;

	fd = syscall_no_intercept(SYS_openat, AT_FDCWD, desc->path, O_RDONLY);


	xabort_on_syserror(fd, __func__);

	return fd;
}

/*
 * add_text_info -- Fill the appropriate fields in an intercept_desc struct
 * about the corresponding code text.
 */
static void
add_text_info(struct intercept_desc *desc, const Elf64_Shdr *header,
		Elf64_Half index)
{
	desc->text_offset = header->sh_offset;
	desc->text_start = desc->base_addr + header->sh_addr;
	desc->text_end = desc->text_start + header->sh_size - 1;
	desc->text_section_index = index;
}

/*
 * find_sections
 *
 * See: man elf
 */
static void
find_sections(struct intercept_desc *desc, int fd)
{
	Elf64_Ehdr elf_header;

	desc->symbol_tables.count = 0;
	desc->rela_tables.count = 0;

	xread(fd, &elf_header, sizeof(elf_header));

	Elf64_Shdr sec_headers[elf_header.e_shnum];

	xlseek(fd, elf_header.e_shoff, SEEK_SET);
	xread(fd, sec_headers, elf_header.e_shnum * sizeof(Elf64_Shdr));

	char sec_string_table[sec_headers[elf_header.e_shstrndx].sh_size];

	xlseek(fd, sec_headers[elf_header.e_shstrndx].sh_offset, SEEK_SET);
	xread(fd, sec_string_table,
	    sec_headers[elf_header.e_shstrndx].sh_size);

	bool text_section_found = false;

	for (Elf64_Half i = 0; i < elf_header.e_shnum; ++i) {
		const Elf64_Shdr *section = &sec_headers[i];
		char *name = sec_string_table + section->sh_name;

		debug_dump("looking at section: \"%s\" type: %ld\n",
		    name, (long)section->sh_type);
		if (strcmp(name, ".text") == 0) {
			text_section_found = true;
			add_text_info(desc, section, i);
		}
	}

	if (!text_section_found)
		xabort("text section not found");
}

/*
 * has_pow2_count
 * Checks if the positive number of patches in a struct intercept_desc
 * is a power of two or not.
 */
static bool
has_pow2_count(const struct intercept_desc *desc)
{
	return (desc->count & (desc->count - 1)) == 0;
}

/*
 * add_new_patch
 * Acquires a new patch entry, and allocates memory for it if
 * needed.
 */
static struct patch_desc *
add_new_patch(struct intercept_desc *desc)
{
	if (desc->count == 0) {

		/* initial allocation */
		desc->items = xmmap_anon(sizeof(desc->items[0]));

	} else if (has_pow2_count(desc)) {

		/* if count is a power of two, double the allocate space */
		size_t size = desc->count * sizeof(desc->items[0]);

		desc->items = xmremap(desc->items, size, 2 * size);
	}

	return &(desc->items[desc->count++]);
}

/*
 * crawl_text
 * Crawl the text section, disassembling it all.
 * This routine collects information about potential addresses to patch.
 *
 * The addresses of all syscall instructions are stored, together with
 * a description of the preceding, and following instructions.
 *
 * A lookup table of all addresses which appear as jump destination is
 * generated, to help determine later, whether an instruction is suitable
 * for being overwritten -- of course, if an instruction is a jump destination,
 * it can not be merged with the preceding instruction to create a
 * new larger one.
 *
 * Note: The actual patching can not yet be done in this disassembling phase,
 * as it is not known in advance, which addresses are jump destinations.
 */
static void
crawl_text(struct intercept_desc *desc)
{
	unsigned char *code = desc->text_start;
	struct intercept_disasm_context *context =
	    intercept_disasm_init(desc->text_start, desc->text_end);

	while (code <= desc->text_end) {
		struct intercept_disasm_result result;

		result = intercept_disasm_next_instruction(context, code);

		if (result.length == 0) {
			code += INSTRUCTION_SIZE;
			continue;
		}

		if (result.is_syscall) {
			struct patch_desc *patch = add_new_patch(desc);
			patch->containing_lib_path = desc->path;
			patch->syscall_addr = code;

			ptrdiff_t syscall_offset = patch->syscall_addr -
			    (desc->text_start - desc->text_offset);
			assert(syscall_offset >= 0);
			patch->syscall_offset = (unsigned long)syscall_offset;
		}
		code += result.length;
	}

	intercept_disasm_destroy(context);
}

/*
 * get_min_address
 * Looks for the lowest address that might be mmap-ed. This is
 * useful while looking for space for a trampoline table close
 * to some text section.
 */
static uintptr_t
get_min_address(void)
{
	static uintptr_t min_address;

	if (min_address != 0)
		return min_address;

	min_address = 0x10000; /* best guess */

	int fd = syscall_no_intercept(SYS_openat, AT_FDCWD,
					"/proc/sys/vm/mmap_min_addr", O_RDONLY);

	if (fd >= 0) {
		char line[64];
		ssize_t r;
		r = syscall_no_intercept(SYS_read, fd, line, sizeof(line) - 1);
		if (r > 0) {
			line[r] = '\0';
			min_address = (uintptr_t)atoll(line);
		}

		syscall_no_intercept(SYS_close, fd);
	}

	return min_address;
}

/*
 * allocate_trampoline_table
 * Allocates memory close to a text section (close enough
 * to be reachable with 32 bit displacements in jmp instructions).
 * Using mmap syscall with MAP_FIXED flag.
 */
void
allocate_trampoline_table(struct intercept_desc *desc)
{
	/* aarch64 doesn't use the extra trampoline table. */
	desc->uses_trampoline_table = false;

	if (!desc->uses_trampoline_table) {
		desc->trampoline_table = NULL;
		desc->trampoline_table_size = 0;
		desc->trampoline_table = NULL;
		return;
	}

	FILE *maps;
	char line[0x100];
	unsigned char *guess; /* Where we would like to allocate the table */
	size_t size;

	if ((uintptr_t)desc->text_end < INT32_MAX) {
		/* start from the bottom of memory */
		guess = (void *)0;
	} else {
		/*
		 * start from the lowest possible address, that can be reached
		 * from the text segment using a 32 bit displacement.
		 * Round up to a memory page boundary, as this address must be
		 * mappable.
		 */
		guess = desc->text_end - INT32_MAX;
		guess = (unsigned char *)(((uintptr_t)guess)
				& ~((uintptr_t)(0xfff))) + 0x1000;
	}

	if ((uintptr_t)guess < get_min_address())
		guess = (void *)get_min_address();

	size = 64 * 0x1000; /* XXX: don't just guess */

	if ((maps = fopen("/proc/self/maps", "r")) == NULL)
		xabort("fopen /proc/self/maps");

	while ((fgets(line, sizeof(line), maps)) != NULL) {
		unsigned char *start;
		unsigned char *end;

		if (sscanf(line, "%p-%p", (void **)&start, (void **)&end) != 2)
			xabort("sscanf from /proc/self/maps");

		/*
		 * Let's see if an existing mapping overlaps
		 * with the guess!
		 */
		if (end < guess)
			continue; /* No overlap, let's see the next mapping */

		if (start >= guess + size) {
			/* The rest of the mappings can't possibly overlap */
			break;
		}

		/*
		 * The next guess is the page following the mapping seen
		 * just now.
		 */
		guess = end;

		if (guess + size >= desc->text_start + INT32_MAX) {
			/* Too far away */
			xabort("unable to find place for trampoline table");
		}
	}

	fclose(maps);

	desc->trampoline_table = mmap(guess, size,
					PROT_READ | PROT_WRITE | PROT_EXEC,
					MAP_FIXED | MAP_PRIVATE | MAP_ANON,
					-1, 0);

	if (desc->trampoline_table == MAP_FAILED)
		xabort("unable to allocate space for trampoline table");

	desc->trampoline_table_size = size;

	desc->next_trampoline = desc->trampoline_table;
}

/*
 * find_syscalls
 * The routine that disassembles a text section. Here is some higher level
 * logic for finding syscalls, finding overwritable NOP instructions, and
 * finding out what instructions around syscalls can be overwritten or not.
 * This code is intentionally independent of the disassembling library used,
 * such specific code is in wrapper functions in the disasm_wrapper.c source
 * file.
 */
void
find_syscalls(struct intercept_desc *desc)
{
	debug_dump("find_syscalls in %s "
	    "at base_addr 0x%016" PRIxPTR "\n",
	    desc->path,
	    (uintptr_t)desc->base_addr);

	desc->count = 0;

	int fd = open_orig_file(desc);

	find_sections(desc, fd);
	debug_dump(
	    "%s .text mapped at 0x%016" PRIxPTR " - 0x%016" PRIxPTR " \n",
	    desc->path,
	    (uintptr_t)desc->text_start,
	    (uintptr_t)desc->text_end);

	syscall_no_intercept(SYS_close, fd);

	crawl_text(desc);
}
