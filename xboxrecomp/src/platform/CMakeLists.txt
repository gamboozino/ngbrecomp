cmake_minimum_required(VERSION 3.20)
project(platform C)

# Platform layer
#   xbox_winnt.h   - Win32 / NT type vocabulary (header only)
#   win32_compat.* - POSIX implementation of generic Win32 host primitives
#
# On Windows the real <windows.h> is used, so this is an INTERFACE library
# that only contributes the src/ include path. On Linux it is a real static
# library carrying the POSIX primitive implementations.

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# src/, the directory this one lives in. Resolved relative to here rather than
# via CMAKE_SOURCE_DIR, which points at the *consuming* project's root when a
# game project pulls xboxrecomp in with add_subdirectory() -- the documented
# integration pattern -- and would send "platform/xbox_winnt.h" looking in the
# game's own src/ tree. PROJECT_SOURCE_DIR is no good either: this file
# declares its own project().
get_filename_component(XBOXRECOMP_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)

if(WIN32 AND XBOXRECOMP_WIN_VCPU)
    # The virtual CPU on Windows (vcpu.h): the scheduler, the host side it
    # needs (win32_host.c), and win32_vcpu.h ahead of every file that links
    # this, routing the Win32 waits through it. The two files here call the
    # real functions.
    add_library(platform STATIC vcpu.c win32_host.c)
    target_include_directories(platform PUBLIC ${XBOXRECOMP_SRC_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_definitions(platform PUBLIC XBOX_WIN_VCPU)
    set_source_files_properties(vcpu.c win32_host.c PROPERTIES COMPILE_DEFINITIONS XBOX_VCPU_WRAP_IMPL)
    target_compile_options(platform PUBLIC "SHELL:-include ${CMAKE_CURRENT_SOURCE_DIR}/win32_vcpu.h")
    find_package(Threads REQUIRED)
    target_link_libraries(platform PUBLIC Threads::Threads)
elseif(WIN32)
    add_library(platform INTERFACE)
    target_include_directories(platform INTERFACE ${XBOXRECOMP_SRC_DIR})
else()
    add_library(platform STATIC win32_compat.c vcpu.c)
    target_include_directories(platform PUBLIC
        ${XBOXRECOMP_SRC_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
    find_package(Threads REQUIRED)
    # m: _clearfp/_controlfp sit on <fenv.h>.
    target_link_libraries(platform PUBLIC Threads::Threads m)
    target_compile_options(platform PRIVATE -Wall -Wextra -Wno-unused-parameter ${XBOXRECOMP_OPT_LEVEL})
    if(APPLE)
        # win32_compat.c's weak g_xbox_mem_offset (the kernel's): Apple's linker wants undefined weak
        # symbols named, or the platform's own tests (no kernel) fail to link.
        target_link_options(platform INTERFACE "LINKER:-U,_g_xbox_mem_offset")
    endif()
endif()
