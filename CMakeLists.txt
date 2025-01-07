cmake_minimum_required(VERSION 3.31.0)

# Other arguments
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
add_definitions(-w)

project(
  ShaderRinth
  VERSION 0.0.1
  DESCRIPTION "Placeholder"
  LANGUAGES CXX
)

set(EXECUTABLE app)

add_executable(
  ${EXECUTABLE}
  src/main.cpp
  src/editor.cpp
  src/utils.cpp
  src/graph.cpp
  imnodes/imnodes.cpp
)
target_include_directories(${EXECUTABLE} PRIVATE 
  include

  ${CMAKE_SOURCE_DIR}
  imnodes
  zep/include
  portable-file-dialogs
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
find_package(OpenGL REQUIRED)
target_link_libraries(${EXECUTABLE} PRIVATE OpenGL::GL)

# Link zep
add_definitions(-DZEP_FEATURE_CPP_FILE_SYSTEM)
add_definitions(-DZEP_SINGLE_HEADER=1)
set(APP_ROOT ${CMAKE_CURRENT_LIST_DIR})
configure_file(${CMAKE_CURRENT_LIST_DIR}/cmake/config_app.h.cmake ${CMAKE_BINARY_DIR}/config_app.h)
target_include_directories(${EXECUTABLE} PRIVATE 
  ${CMAKE_BINARY_DIR} # config_app.h
)

# compile options
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
  target_compile_options(${EXECUTABLE} PRIVATE /W4)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_compile_options(${EXECUTABLE} PRIVATE -Wall -Wextra -Wpedantic)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  # other macOS-specific flags for Clang
endif()

