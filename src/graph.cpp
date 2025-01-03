#include <GLFW/glfw3.h>
#include <cstdlib>
#include <graph.h>
#include <spdlog/spdlog.h>

const std::string to_string(DataType type) noexcept {
  return TYPESTRMAP.at(type);
};
