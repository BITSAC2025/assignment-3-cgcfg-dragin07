# Specify the minimum version of CMake required to build the project
cmake_minimum_required(VERSION 3.23)

project(Answers)

if (DEFINED ENV{SVF_DIR})
    set(SVF_DIR $ENV{SVF_DIR})
else ()
    message(FATAL_ERROR "\
    WARNING: The SVF_DIR var was not set !\n\
Please set this to environment variable to point to the LLVM_DIR directory or set this variable to cmake configuration\n(e.g. on linux: export SVF_DIR=/path/to/SVF/dir)")
endif ()
# Find the SVF CMake package (pass $SVF_DIR as a (prioritised) hint)
find_package(SVF REQUIRED CONFIG HINTS ${SVF_DIR} ${SVF_DIR}/Debug-build ${SVF_DIR}/Release-build)
message(STATUS "SVF STATUS:
    Found:                              ${SVF_FOUND}
    Version:                            ${SVF_VERSION}
    Build mode:                         ${SVF_BUILD_TYPE}
    C++ standard:                       ${SVF_CXX_STANDARD}
    RTTI enabled:                       ${SVF_ENABLE_RTTI}
    Exceptions enabled:                 ${SVF_ENABLE_EXCEPTIONS}
    Install root directory:             ${SVF_INSTALL_ROOT}
    Install binary directory:           ${SVF_INSTALL_BIN_DIR}
    Install library directory:          ${SVF_INSTALL_LIB_DIR}
    Install include directory:          ${SVF_INSTALL_INCLUDE_DIR}
    Install 'extapi.bc' file path:      ${SVF_INSTALL_EXTAPI_FILE}
    Install BC:                         ${SVF_INSTALL_EXTAPI_BC}"
        )
if (NOT ${SVF_FOUND})
    message(FATAL_ERROR "\
    WARNING: package SVF not installed !\n\
Please go to $ENV{SVF_DIR} and type './build.sh' or './build.sh debug' to install SVF")
endif ()
include_directories(SYSTEM ${SVF_INSTALL_INCLUDE_DIR})
link_directories(${SVF_INSTALL_LIB_DIR})
set(SVF_LIB SvfLLVM SvfCore)

# If the LLVM_DIR environment variable is set, configure CMake build flags and standards for C++ and C
if (DEFINED ENV{LLVM_DIR})
    set(LLVM_DIR $ENV{LLVM_DIR})
else ()
    set(LLVM_DIR ${SVF_DIR}/llvm-16.0.0.obj)
endif ()
# Locate and use the LLVM package for the project
find_package(LLVM 16.0.0 REQUIRED CONFIG HINTS
        ${LLVM_DIR}/lib/cmake/llvm)
if (NOT "${LLVM_FOUND}")
    message(FATAL_ERROR "Failed to find supported LLVM version")
endif ()
message(STATUS "LLVM STATUS:
    Version:                            ${LLVM_VERSION}
    Includes:                           ${LLVM_INCLUDE_DIRS}
    Libraries:                          ${LLVM_LIBRARY_DIRS}
    Build type:                         ${LLVM_BUILD_TYPE}
    RTTI enabled:                       ${LLVM_ENABLE_RTTI}
    Exceptions enabled:                 ${LLVM_ENABLE_EH}
    Dynamic lib:                        ${LLVM_LINK_LLVM_DYLIB}"
        )
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(AddLLVM)

# Find and configure Z3 package, first trying the system Z3 with CMake, then fallback to SVF's Z3 instance
# Find Z3 and its include directory from the top-level include file
find_library(Z3_LIBRARIES REQUIRED NAMES z3 HINTS ${Z3_DIR} ENV Z3_DIR PATH_SUFFIXES bin lib)
find_path(Z3_INCLUDES REQUIRED NAMES z3++.h HINTS ${Z3_DIR} ENV Z3_DIR PATH_SUFFIXES include z3)
message(STATUS "Z3 STATUS:
    Z3 library file:                    ${Z3_LIBRARIES}
    Z3 include directory:               ${Z3_INCLUDES}"
        )

# Set the C++ standard to C++17 and configure compiler flags based on the build type
set(CMAKE_CXX_STANDARD 17)
if (CMAKE_BUILD_TYPE MATCHES "Debug")
    set(CMAKE_CXX_FLAGS "-fPIC -std=gnu++17 -O0 -fno-rtti -Wno-deprecated -Werror")
else ()
    set(CMAKE_CXX_FLAGS "-fPIC -std=gnu++17 -O3 -fno-rtti -Wno-deprecated -Werror")
endif ()
set(CMAKE_C_FLAGS "-fPIC")
# Check if compiler is GNU and version is less than 9
if (CMAKE_COMPILER_IS_GNUCXX AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9)
    # Link filesystem library globally
    link_libraries(stdc++fs)
endif ()

set(LLVM_LIB LLVM)


if (DEFINED SUBDIRS)
    foreach (subdir IN LISTS SUBDIRS)
#        message(${subdir})
        string(STRIP "${subdir}" subdir_clean)
        if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${subdir_clean})
            add_subdirectory(${subdir_clean})
        endif ()
    endforeach ()
else ()
    add_subdirectory(Assignment-2-IR)
    add_subdirectory(Assignment-3-CGCFG)
endif ()



