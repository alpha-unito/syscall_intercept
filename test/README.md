# Test directory #

All the source files for executables and initialization libraries and utility
files for testing are stored here. In the current directory only architecture
agnostic tests are present, while architecture specific tests are stored in a
dedicated directory inside `arch/`. As an example, inside `arch/x86_64/` some
assembly tests are stored, which must be optionally compiled just when building
on amd64 CPUs.


**RISC-V specific tests are coming soon**. However, the architecture agnostic
test suite can be run properly.

# How to build #

Tests are automatically built when building the library. If the library still
has to be built, from the root directory of the project run
```shell
mkdir build
cd build
cmake ..
make
```

[CMakeLists.txt](CMakeLists.txt) is written to automatically detect the CPU
architecture and select which tests must be compiled accordingly.

In the end, running
```shell
make test
```
will run the compiled test suite.

# Testing logic #

Different approaches are used to test syscall_intercept but an overall
distinction can be made between high-level and low-level coded tests.
- **Matching tests**: these tests verify the correct execution of the system 
    call interception. Some of them use the library logging logic to produce a 
    log file which gets then inspected to make sure it matches the expected output.
    I.e., [syscall_format.c](syscall_format.c) makes a lot of system calls of
    in a mock context just to create a log file which will be compared to
    [syscall_format.log.match](syscall_format.log.match). One mismatch is enough
    to fail the test.<br>Since system calls implementation may vary across
    different glibc versions, the .match files could need to be updated to grant
    a correct testing process. Depending on the glibc versions an `fstat` system
    call could actually call `fstat` or `newfstatat`, or a `fstat` call with a
    negative file descriptor could be forwarded to the kernel (invoking
    syscall_intercept logic) or return before invoking the kernel (not invoking
    syscall_intercept logic at all, and so the logging). Current .match files are
    designed with glibc 2.35 and 2.39 versions in mind, which are the default
    shipped versions with Ubuntu 22.04 and 24.04 respectively. Moreover, even the
    same version on different architectures could involve different implementations.
    <br>For these reasons, failing of a test of such type does not necessarily
    imply that syscall_intercept is malfunctioning.
- **Assertion tests**: these tests basically assert a condition which can't possibly be true unless
  the hook function is executed as expected thanks to a correct interception of
  the tested system call. As an example, the `write` test writes a string to a
  file, then reads the written string from that file and in the end asserts that
  the two strings are perfectly equal. By looking at the content of
  [write_test.c](src/write_test.c) it is obvious how the assertion could never be
  true since the written string and the expected string are different. The hook
  function which will be executed before forwarding the system call to the kernel
  will make sure to modify the string that is going to be written to the file so
  that the actual written string and the expected string match.
  All the tests implementing such pattern are named in the suite as `<syscall-name>_generic`.
- **Patching tests**: these tests verify the correct execution of the patching 
  process. They work in couples with an input and an output pattern file, both
  coded in assembly. The input object file will be patched by syscall_intercept
  producing a patched binary which will be compared with the output file by
  `memcmp()`. Failing of such tests implies that the patching process is not
  writing the expected binary code.<br>**RISC-V tests of such type are coming soon.**

Assertion tests must be considered as couples containing an executable
and an initialization library, named as follows:
- `<syscall-name>_test.c`
- `intercept_sys_<syscall-name>.c`

Granted that syscall_intercept is already built, assertion tests can alternatively
be compiled in this directory with
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