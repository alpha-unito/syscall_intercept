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

/*
 * execution of this test must be preloaded with the corresponding interception
 * library, i.e. from the syscall_intercept/test directory
 * LD_LIBRARY_PATH=.:../build LD_PRELOAD=intercept_sys_write.so ./write_test
 */

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>

int main() {
    int test_fd = openat(AT_FDCWD, "testfile.txt", O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (test_fd == -1) {
        printf("intercepted_openat error nr: %d\n", errno);
        return 1;
    }
    char buf[128] = "original_syscall\n";
    write(test_fd, buf, strlen(buf));
    lseek(test_fd, 0, SEEK_SET);
    char test_buf[128];
    int n = read(test_fd, test_buf, strlen(buf));
    test_buf[n] = '\0';
    assert(strcmp(test_buf, "intercepted_call\n") == 0);
    char test_ok[128] = "WRITE TEST - OK\n";
    write(1, test_ok, strlen(test_ok));
    return 0;
}
