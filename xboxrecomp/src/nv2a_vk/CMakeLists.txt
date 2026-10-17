# nv2a_vk: xemu's NV2A model and Vulkan renderer (see README.md).
# Linux for now; third-party sources come from third_party/fetch.sh.

set(TP ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party)
if(NOT EXISTS ${TP}/volk/volk.c)
    message(FATAL_ERROR "nv2a_vk: run third_party/fetch.sh first")
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(NV2A_GLIB REQUIRED IMPORTED_TARGET glib-2.0)
find_package(SDL2 REQUIRED)
# glslang's C interface. Its CMake package when there is one: a static
# glslang is several archives (MachineIndependent, GenericCodeGen, ...) that
# only the package knows to link. Otherwise the libraries by name.
find_package(glslang CONFIG QUIET)
if(TARGET glslang::glslang AND TARGET glslang::glslang-default-resource-limits)
    set(NV2A_GLSLANG glslang::glslang glslang::glslang-default-resource-limits)
else()
    find_library(GLSLANG_LIB NAMES libglslang.a glslang REQUIRED)
    find_library(GLSLANG_RL_LIB NAMES libglslang-default-resource-limits.a
                 glslang-default-resource-limits REQUIRED)
    find_library(SPIRV_OPT_LIB NAMES libSPIRV-Tools-opt.a SPIRV-Tools-opt)
    find_library(SPIRV_TOOLS_LIB NAMES libSPIRV-Tools.a SPIRV-Tools)
    set(NV2A_GLSLANG ${GLSLANG_LIB} ${GLSLANG_RL_LIB}
        $<$<BOOL:${SPIRV_OPT_LIB}>:${SPIRV_OPT_LIB}>
        $<$<BOOL:${SPIRV_TOOLS_LIB}>:${SPIRV_TOOLS_LIB}>)
endif()

set(X ${CMAKE_CURRENT_SOURCE_DIR}/xemu)
file(GLOB NV2A_VK_PGRAPH  ${X}/pgraph/*.c)
file(GLOB NV2A_VK_GLSL    ${X}/pgraph/glsl/*.c)
file(GLOB NV2A_VK_VK      ${X}/pgraph/vk/*.c)

# OBJECT, not STATIC: the renderers register themselves from static
# constructors in files nothing else references, which an archive would drop.
add_library(xbox_nv2a_vk OBJECT
    ${X}/pbus.c ${X}/pcrtc.c ${X}/pfb.c ${X}/pfifo.c ${X}/pmc.c
    ${X}/pramdac.c ${X}/prmcio.c ${X}/prmdio.c ${X}/prmvio.c ${X}/ptimer.c
    ${X}/pvideo.c ${X}/stubs.c ${X}/user.c
    ${NV2A_VK_PGRAPH} ${NV2A_VK_GLSL} ${NV2A_VK_VK}
    ${X}/pgraph/null/renderer.c
    shim/util/fast-hash.c
    shim/util/vma.cc
    host/nv2a_host.c
    host/nv2a_dirty.c
    host/present_sdl.c
    host/nv2a_runtime.c
    ${TP}/volk/volk.c
    ${TP}/SPIRV-Reflect/spirv_reflect.c
    ${TP}/nv2a_vsh_cpu/src/nv2a_vsh_cpu.c
    ${TP}/nv2a_vsh_cpu/src/nv2a_vsh_disassembler.c
    ${TP}/nv2a_vsh_cpu/src/nv2a_vsh_emulator.c
    ${TP}/nv2a_vsh_cpu/src/nv2a_vsh_emulator_execution_state.c
)
target_include_directories(xbox_nv2a_vk
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/host
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/shim
            ${X}
            ${X}/pgraph/vk
            ${TP}/Vulkan-Headers/include
            ${TP}/volk
            ${TP}/VulkanMemoryAllocator/include
            ${TP}/SPIRV-Reflect
            ${TP}/nv2a_vsh_cpu/src
            ${TP}/xxHash
            ${TP}/stb)
target_compile_definitions(xbox_nv2a_vk PRIVATE
    VK_NO_PROTOTYPES $<$<PLATFORM_ID:Linux>:VK_USE_PLATFORM_XLIB_KHR VK_USE_PLATFORM_WAYLAND_KHR>
    VMA_STATIC_VULKAN_FUNCTIONS=0 VMA_DYNAMIC_VULKAN_FUNCTIONS=0
    XXH_INLINE_ALL _GNU_SOURCE)
target_compile_options(xbox_nv2a_vk PRIVATE
    $<$<COMPILE_LANGUAGE:C>:-std=gnu11>
    -O2 -fno-strict-aliasing -Wno-unused-parameter -Wno-sign-compare
    -Wno-missing-field-initializers -Wno-unused-function)
target_link_libraries(xbox_nv2a_vk PUBLIC
    xbox_kernel xbox_video platform
    PkgConfig::NV2A_GLIB SDL2::SDL2
    ${NV2A_GLSLANG}
    $<IF:$<PLATFORM_ID:Windows>,winpthread,${CMAKE_DL_LIBS} m pthread>
    $<$<PLATFORM_ID:Windows>:dxgi dxguid>)   # dxgi: the display's vertical blank (nv2a_host.c)   # libstdc++: the C++ link brings it (statically under -static-libstdc++)
