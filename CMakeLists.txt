cmake_minimum_required(VERSION 3.31)

# Other arguments
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

project(
  ShaderRinth
  VERSION 0.0.1
  DESCRIPTION "A Shadertoy-like GLSL Editor written in C++"
  LANGUAGES CXX
)

set(EXECUTABLE ShaderRinth)

add_executable(${EXECUTABLE} src/main.cpp)

target_include_directories(${EXECUTABLE} PRIVATE 
  include/

  extern/imnodes/
  extern/portable-file-dialogs/
  extern/IconFontCppHeaders/
)
target_include_directories(${EXECUTABLE} # Suppress warnings
  SYSTEM PRIVATE extern/zep/include/
)
target_sources(${EXECUTABLE} PRIVATE
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
  src/widgets/export_image_popup.cpp
  src/widgets/outliner_widget.cpp
  src/widgets/viewport_widget.cpp
  src/nodes/shader_node.cpp

  extern/imnodes/imnodes.cpp
)

# Link libraries (vcpkg)
find_package(OpenGL REQUIRED)
target_link_libraries(${EXECUTABLE} PRIVATE OpenGL::GL)
find_package(glad CONFIG REQUIRED)
target_link_libraries(${EXECUTABLE} PRIVATE glad::glad)
find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(${EXECUTABLE} PRIVATE glfw)
find_package(imgui CONFIG REQUIRED)
target_link_libraries(${EXECUTABLE} PRIVATE imgui::imgui)
find_package(spdlog CONFIG REQUIRED)
target_link_libraries(${EXECUTABLE} PRIVATE spdlog::spdlog)
find_package(Stb REQUIRED)
target_include_directories(${EXECUTABLE} PRIVATE ${Stb_INCLUDE_DIR})

# Link tomlplusplus
find_package(PkgConfig REQUIRED)
pkg_check_modules(tomlplusplus REQUIRED IMPORTED_TARGET tomlplusplus)
target_link_libraries(${EXECUTABLE} PRIVATE PkgConfig::tomlplusplus)

# Configure zep
add_definitions(-DZEP_FEATURE_CPP_FILE_SYSTEM)
add_definitions(-DZEP_SINGLE_HEADER=1)

# Copy assets
file(COPY assets/ DESTINATION ${CMAKE_BINARY_DIR}/assets)

# Precompile headers
option(USE_PCH "Enable precompiled headers" OFF)
if(USE_PCH)
  message(STATUS "Using precompiled headers")
  target_precompile_headers(${EXECUTABLE} PRIVATE cmake/pch.h)
else()
  message(STATUS "Building without precompiled headers")
endif()

# Compile options
if (WIN32) # Set icon file on Windows
  target_sources(${EXECUTABLE} PRIVATE icon.rc)
endif()
if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  target_compile_options(${EXECUTABLE} PRIVATE /W4)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(${EXECUTABLE} PRIVATE -Wall -Wextra -Wpedantic)
endif()
if (MINGW AND WIN32) # Disable console on startup on Windows
  target_link_options(${EXECUTABLE}
    PRIVATE "-mwindows"
    PRIVATE "-static"
  )
endif()

# Install instructions
install(TARGETS ${EXECUTABLE} DESTINATION .)
install(DIRECTORY assets/ DESTINATION assets)

# CPack Configuration
set(CPACK_PACKAGE_NAME "ShaderRinth") 
set(CPACK_PACKAGE_VERSION "0.0.1")
set(CPACK_PACKAGE_DESCRIPTION "A Shadertoy-like GLSL Editor written in C++")
set(CPACK_GENERATOR "TGZ;ZIP")

include(CPack)
