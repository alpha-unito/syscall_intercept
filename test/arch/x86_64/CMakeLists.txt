#
# Copyright 2017-2020, Intel Corporation
# Copyright 2025, University of Turin
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in
#       the documentation and/or other materials provided with the
#       distribution.
#
#     * Neither the name of the copyright holder nor the names of its
#       contributors may be used to endorse or promote products derived
#       from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# See: https://cmake.org/Wiki/CMake/Testing_With_CTest

set(CMAKE_ASM_CREATE_SHARED_LIBRARY ${CMAKE_C_CREATE_SHARED_LIBRARY})

add_executable(asm_pattern asm_pattern.c
        $<TARGET_OBJECTS:syscall_intercept_base_c>
        $<TARGET_OBJECTS:syscall_intercept_base_asm>)
target_link_libraries(asm_pattern PUBLIC ${CAPSTONE_LIB_FOLDER}/libcapstone.so)

target_link_libraries(asm_pattern
        PRIVATE ${CMAKE_DL_LIBS} ${capstone_LDFLAGS})

set(asm_patterns
        nosyscall
        pattern1
        pattern2
        pattern3
        pattern4
        pattern_loop
        pattern_loop2
        pattern_symbol_boundary0
        pattern_symbol_boundary1
        pattern_symbol_boundary2
        pattern_symbol_boundary3
        pattern_nop_padding0
        pattern_nop_padding1
        pattern_nop_padding2
        pattern_nop_padding3
        pattern_nop_padding4
        pattern_nop_padding5
        pattern_nop_padding6
        pattern_nop_padding7
        pattern_nop_padding8
        pattern_nop_padding9
        pattern_lea_rip_rdi
        pattern_lea_rip_r12)

try_compile(ASSEMBLER_SUPPORTS_ENDBR64 ${CMAKE_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/pattern_endbr64.in.S
        CMAKE_FLAGS "-DCMAKE_ASM_LINK_EXECUTABLE='echo skip linking'")
if(ASSEMBLER_SUPPORTS_ENDBR64)
    list(APPEND asm_patterns pattern_endbr64)
endif()

set(asm_patterns_failing
        pattern_double_syscall
        pattern_rets
        pattern_jmps)

macro(add_asm_test test_name failing)
    add_library(${test_name}.in SHARED ${test_name}.in.S)
    add_library(${test_name}.out SHARED ${test_name}.out.S)
    if(LINKER_HAS_NOSTDLIB)
        set_target_properties(${test_name}.in
                PROPERTIES LINK_FLAGS "-nostdlib")
        set_target_properties(${test_name}.out
                PROPERTIES LINK_FLAGS "-nostdlib")
    endif()
    if(HAS_NOUNUSEDARG)
        target_compile_options(${test_name}.in BEFORE
                PRIVATE "-Wno-unused-command-line-argument")
        target_compile_options(${test_name}.out BEFORE
                PRIVATE "-Wno-unused-command-line-argument")

    endif()
    add_test(NAME "asm_pattern_${test_name}"
            COMMAND $<TARGET_FILE:asm_pattern>
            $<TARGET_FILE:${test_name}.in>
            $<TARGET_FILE:${test_name}.out>)
    if(${failing})
        set_tests_properties("asm_pattern_${test_name}"
                PROPERTIES WILL_FAIL ON
                PASS_REGULAR_EXPRESSION "Invalid patch")
    endif()
endmacro()

foreach(name ${asm_patterns})
    add_asm_test(${name} FALSE)
endforeach()

foreach(name ${asm_patterns_failing})
    add_asm_test(${name} TRUE)
endforeach()

add_executable(executable_with_syscall_pie executable_with_syscall.S)
if(HAS_NOUNUSEDARG)
    target_compile_options(executable_with_syscall_pie BEFORE
            PRIVATE "-Wno-unused-command-line-argument")
endif()
set_target_properties(executable_with_syscall_pie
        PROPERTIES POSITION_INDEPENDENT_CODE True)
if(HAS_ARG_PIE)
    target_compile_options(executable_with_syscall_pie PRIVATE "-pie")
    target_link_libraries(executable_with_syscall_pie PRIVATE "-pie")
endif()

add_executable(executable_with_syscall_no_pie executable_with_syscall.S)
if(HAS_NOUNUSEDARG)
    target_compile_options(executable_with_syscall_no_pie BEFORE
            PRIVATE "-Wno-unused-command-line-argument")
endif()
set_target_properties(executable_with_syscall_no_pie
        PROPERTIES POSITION_INDEPENDENT_CODE False)
if(HAS_ARG_NOPIE)
    target_compile_options(executable_with_syscall_no_pie PRIVATE "-nopie")
    target_link_libraries(executable_with_syscall_no_pie PRIVATE "-nopie")
elseif(HAS_ARG_NO_PIE)
    target_compile_options(executable_with_syscall_no_pie PRIVATE "-no-pie")
    target_link_libraries(executable_with_syscall_no_pie PRIVATE "-no-pie")
endif()

add_test(NAME "prog_pie_intercept_libc_only"
        COMMAND ${CMAKE_COMMAND}
        -DTEST_EXTRA_PRELOAD=${TEST_EXTRA_PRELOAD}
        -DTEST_PROG=$<TARGET_FILE:executable_with_syscall_pie>
        -DLIB_FILE=$<TARGET_FILE:intercept_sys_write>
        -DTEST_PROG_ARGS=original_syscall
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check.cmake)
set_tests_properties("prog_pie_intercept_libc_only"
        PROPERTIES PASS_REGULAR_EXPRESSION "original_syscall")

add_test(NAME "prog_no_pie_intercept_libc_only"
        COMMAND ${CMAKE_COMMAND}
        -DTEST_EXTRA_PRELOAD=${TEST_EXTRA_PRELOAD}
        -DTEST_PROG=$<TARGET_FILE:executable_with_syscall_no_pie>
        -DLIB_FILE=$<TARGET_FILE:intercept_sys_write>
        -DTEST_PROG_ARGS=original_syscall
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check.cmake)
set_tests_properties("prog_no_pie_intercept_libc_only"
        PROPERTIES PASS_REGULAR_EXPRESSION "original_syscall")

add_test(NAME "prog_pie_intercept_all"
        COMMAND ${CMAKE_COMMAND}
        -DTEST_EXTRA_PRELOAD=${TEST_EXTRA_PRELOAD}
        -DINTERCEPT_ALL=1
        -DTEST_PROG=$<TARGET_FILE:executable_with_syscall_pie>
        -DLIB_FILE=$<TARGET_FILE:intercept_sys_write>
        -DTEST_PROG_ARGS=original_syscall
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check.cmake)
set_tests_properties("prog_pie_intercept_all"
        PROPERTIES PASS_REGULAR_EXPRESSION "intercepted_call")

add_test(NAME "prog_no_pie_intercept_all"
        COMMAND ${CMAKE_COMMAND}
        -DTEST_EXTRA_PRELOAD=${TEST_EXTRA_PRELOAD}
        -DINTERCEPT_ALL=1
        -DTEST_PROG=$<TARGET_FILE:executable_with_syscall_no_pie>
        -DLIB_FILE=$<TARGET_FILE:intercept_sys_write>
        -DTEST_PROG_ARGS=original_syscall
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check.cmake)
set_tests_properties("prog_no_pie_intercept_all"
        PROPERTIES PASS_REGULAR_EXPRESSION "intercepted_call")