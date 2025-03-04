#pragma once

//! Std
#include <any>        // IWYU pragma: export
#include <filesystem> // IWYU pragma: export
#include <fstream>    // IWYU pragma: export
#include <map>        // IWYU pragma: export
#include <memory>     // IWYU pragma: export
#include <optional>   // IWYU pragma: export
#include <string>     // IWYU pragma: export
#include <vector>     // IWYU pragma: export

//! OpenGL
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

//! ImGui
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_stdlib.h>

//! ImNodes
#include "imnodes.h" // IWYU pragma: export

//! Portable-File-Dialogs
#include "portable-file-dialogs.h" // IWYU pragma: export

//! spdlog
// #include <spdlog/sinks/ringbuffer_sink.h>
// #include <spdlog/sinks/stdout_color_sinks.h>
// #include <spdlog/spdlog.h>
