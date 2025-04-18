﻿# CMakeList.txt : CMake project for KitsuneEngine, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.30)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()



project(KitsuneEngine VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)



set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")

add_subdirectory(src)


# CPM setup
file(DOWNLOAD https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.40.7/CPM.cmake
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake"
    EXPECTED_HASH SHA256=c0fc82149e00c43a21febe7b2ca57b2ffea2b8e88ab867022c21d6b81937eb50
)
include(${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)



CPMAddPackage(NAME SDL3 GITHUB_REPOSITORY libsdl-org/SDL GIT_TAG release-3.2.8 OPTIONS "SDL_VULKAN ON" "SDL_STATIC OFF")
CPMAddPackage(NAME fmt GITHUB_REPOSITORY fmtlib/fmt GIT_TAG 10.2.1)
CPMAddPackage(NAME Vulkan-Headers GITHUB_REPOSITORY KhronosGroup/Vulkan-Headers GIT_TAG v1.4.310)
CPMAddPackage(NAME glm GITHUB_REPOSITORY g-truc/glm GIT_TAG 1.0.1)

find_program(GLSL_VALIDATOR glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)

## find all the shader files under the shaders folder
file(GLOB_RECURSE GLSL_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/shaders/*.frag"
    "${PROJECT_SOURCE_DIR}/shaders/*.vert"
    "${PROJECT_SOURCE_DIR}/shaders/*.comp"
    )

    ## iterate each shader
foreach(GLSL ${GLSL_SOURCE_FILES})
  message(STATUS "BUILDING SHADER")
  get_filename_component(FILE_NAME ${GLSL} NAME)
  set(SPIRV "${PROJECT_SOURCE_DIR}/bin/shaders/${FILE_NAME}.spv")
  message(STATUS ${GLSL})
  ##execute glslang command to compile that specific shader
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
    DEPENDS ${GLSL})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)


    add_custom_target(
    shaders 
    ALL DEPENDS ${SPIRV_BINARY_FILES}
    )

