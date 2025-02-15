cmake_minimum_required(VERSION 3.31.0)

# Other arguments
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
add_definitions(-w)

project(
  ShaderRinth
  VERSION 0.0.1
  DESCRIPTION "A Shadertoy-like GLSL Editor written in C++"
  LANGUAGES CXX
)

set(EXECUTABLE ShaderRinth)

add_executable(
  ${EXECUTABLE}
  src/main.cpp
  src/editor.cpp
  src/utils.cpp
  src/data.cpp
  src/graph.cpp
  src/texture.cpp
  src/theme.cpp
  src/geometry.cpp
  src/shader.cpp
  src/app.cpp
  src/assets.cpp

  src/widgets/node_editor_widget.cpp
  src/widgets/editor_widget.cpp
  
  extern/imnodes/imnodes.cpp
)
target_include_directories(${EXECUTABLE} PRIVATE 
  include

  ${CMAKE_SOURCE_DIR}
  extern/imnodes
  extern/zep/include
  extern/portable-file-dialogs
)

# Link libraries (vcpkg)
find_package(imgui CONFIG REQUIRED)
target_link_libraries(${EXECUTABLE} PRIVATE imgui::imgui)
find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(${EXECUTABLE} PRIVATE glfw)
find_package(spdlog CONFIG REQUIRED)
target_link_libraries(${EXECUTABLE} PRIVATE spdlog::spdlog)
find_package(cereal CONFIG REQUIRED)
target_link_libraries(${EXECUTABLE} PRIVATE cereal::cereal)
find_package(Stb REQUIRED)
target_include_directories(${EXECUTABLE} PRIVATE ${Stb_INCLUDE_DIR})
find_package(OpenGL REQUIRED)
target_link_libraries(${EXECUTABLE} PRIVATE OpenGL::GL)

# Link tomlplusplus
find_package(PkgConfig REQUIRED)
pkg_check_modules(tomlplusplus REQUIRED IMPORTED_TARGET tomlplusplus)
target_link_libraries(${EXECUTABLE} PRIVATE PkgConfig::tomlplusplus)

# Link zep
add_definitions(-DZEP_FEATURE_CPP_FILE_SYSTEM)
add_definitions(-DZEP_SINGLE_HEADER=1)
set(APP_ROOT ${CMAKE_CURRENT_LIST_DIR})
configure_file(${CMAKE_CURRENT_LIST_DIR}/cmake/config_app.h.cmake ${CMAKE_BINARY_DIR}/config_app.h)
target_include_directories(${EXECUTABLE} PRIVATE 
  ${CMAKE_BINARY_DIR} # config_app.h
)

# Precompile headers
option(USE_PCH "Enable precompiled headers" ON)
if(USE_PCH)
    message(STATUS "Using precompiled headers")
    target_precompile_headers(${EXECUTABLE} PRIVATE cmake/pch.h)
else()
    message(STATUS "Building without precompiled headers")
endif()

# compile options
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
  target_compile_options(${EXECUTABLE} PRIVATE /W4)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_compile_options(${EXECUTABLE} PRIVATE -Wall -Wextra -Wpedantic)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  # other macOS-specific flags for Clang
endif()

