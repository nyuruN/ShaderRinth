cmake_minimum_required(VERSION 3.31.0)

# Other arguments
set(CMAKE_TOOLCHAIN_FILE "~/.local/share/vcpkg/scripts/buildsystems/vcpkg.cmake")
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

include(FetchContent)

project(
  ShaderRinth
  VERSION 0.0.1
  DESCRIPTION "Placeholder"
  LANGUAGES CXX
)

add_executable(
  app
  src/main.cpp
)

# Fetch libraries
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        v1.91.5-docking
)
FetchContent_MakeAvailable(imgui)
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.4
)
FetchContent_MakeAvailable(glfw)
find_package(OpenGL REQUIRED)

# Setup Imgui
add_library(imgui INTERFACE)
target_include_directories(imgui INTERFACE ${imgui_SOURCE_DIR})

# Setup Imgui Backend
add_library(imgui_impl STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/imgui_demo.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
  ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
)
target_include_directories(imgui_impl PRIVATE ${imgui_SOURCE_DIR} ${glfw_SOURCE_DIR}/include)
target_link_libraries(imgui_impl PRIVATE imgui glfw OpenGL::GL)

# Setup spdlog
add_library(spdlog STATIC IMPORTED)
set_target_properties(spdlog PROPERTIES
  IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/extern/spdlog/build/libspdlog.a"
  INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/extern/spdlog/include"
)

# Link imgui glfw opengl backends
target_link_libraries(app PRIVATE OpenGL::GL glfw imgui imgui_impl spdlog)
target_include_directories(app PRIVATE 
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/misc/cpp
  ${imgui_SOURCE_DIR}/backends
  ${glfw_SOURCE_DIR}/include
)

# compile options
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
  target_compile_options(app PRIVATE /W4)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_compile_options(app PRIVATE -Wall -Wextra -Wpedantic)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  # other macOS-specific flags for Clang
endif()

