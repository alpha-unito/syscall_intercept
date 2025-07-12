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
#include <sched.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

int child_func(void *arg) {
    int fd = openat(AT_FDCWD, "testfile.txt", O_RDONLY);
    char buf[128];
    int n = read(fd, buf, sizeof(buf));
    buf[n] = '\0';
    n = atoi(buf);
    assert(n == getpid());
    return 0;
}

int main() {
    int fd = openat(AT_FDCWD, "testfile.txt", O_CREAT | O_TRUNC | O_RDWR, 0666);
    int fd2 = openat(AT_FDCWD, "testfile2.txt", O_CREAT | O_TRUNC | O_RDWR, 0666);

    char child_stack[8192];

    pid_t pid = clone(child_func, child_stack + sizeof(child_stack),
                      SIGCHLD, NULL);

    if (pid == -1) {
        perror("Clone failed");
        return 1;
    }

    int status;
    wait(&status);
    if (WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT) {
        fprintf(stderr, "Child assertion failed\n");
        return 1;
    }

    char buf[128];
    int n = read(fd2, buf, sizeof(buf));
    buf[n] = '\0';
    n = atoi(buf);
    assert(n == getpid());
    write(1, "CLONE TEST - OK\n", 16);
    return 0;
}


