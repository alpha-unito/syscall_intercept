# Testing on RISC-V #

The tests inside `./src` must be considered as couples containing an executable
and an initialization library, named as follows:
- `<syscall-name>_test.c`
- `intercept_sys_<syscall-name>.c`

They can be compiled with
```shell
make
```
and executed with
```shell
make test
```
Alternatively, it is possible to execute a specific test with the following
command, where syscall-name must be replaced with one of the entries in
the TESTS variable in the Makefile
```
make <syscall-name>
```

## How these test work ##

These tests basically assert a condition which can't possibly be true unless
the hook function is executed as expected thanks to a correct interception of
the tested system call. As an example, the `write` test writes a string to a
file, then reads the written string from that file and in the end asserts that
the two strings are perfectly equal. By looking at the content of
[write_test.c](src/write_test.c) it is obvious how the assertion could never be
true since the written string and the expected string are different. The hook
function which will be executed before forwarding the system call to the kernel
will make sure to modify the string that is going to be written to the file so
that the actual written string and the expected string match.
All the tests implement the described pattern.