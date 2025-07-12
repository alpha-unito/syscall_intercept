/*
 * Copyright 2025, University of Turin
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

#define _GNU_SOURCE
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <linux/stat.h>
#include <fcntl.h>
#include <assert.h>

int main() {
    int fd = openat(AT_FDCWD, "testfile.txt", O_RDWR | O_CREAT | O_TRUNC, 0666);
    struct statx stx;
    statx(AT_FDCWD, "testfile.txt", 0, STATX_BASIC_STATS | STATX_BTIME, &stx);

    int fd2 = openat(AT_FDCWD, "testfile2.txt", O_RDWR | O_CREAT | O_TRUNC, 0666);
    struct statx stx2;
    statx(AT_FDCWD, "testfile2.txt", 0, STATX_BASIC_STATS | STATX_BTIME, &stx2);

    assert(stx.stx_ino ==  stx2.stx_ino);
    write(1, "STATX TEST - OK\n", 16);

    return 0;
}
