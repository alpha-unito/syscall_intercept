cmake_minimum_required(VERSION 3.10)
project(capstone)
include(FetchContent)

#####################################
# Import external project from git
#####################################
FetchContent_Declare(capstone
        GIT_REPOSITORY https://github.com/capstone-engine/capstone.git
        # the following git tag refers to capstone version 5.0.6
        GIT_TAG accf4df62f1fba6f92cae692985d27063552601c
)

FetchContent_MakeAvailable(capstone)

