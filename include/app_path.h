#pragma once

#if defined(_WIN32)

#include <string>
#include <windows.h>

static std::string getAppPath() {
  char path[MAX_PATH];
  GetModuleFileNameA(NULL, path, MAX_PATH);
  return std::string(path);
}

#elif defined(__linux__)

#include <limits.h>
#include <string>
#include <unistd.h>

static std::string getAppPath() {
  char path[PATH_MAX];
  ssize_t count = readlink("/proc/self/exe", path, PATH_MAX);
  return (count != -1) ? std::string(path, count) : "";
}

#elif defined(__APPLE__)

#include <mach-o/dyld.h>
#include <string>

static std::string getAppPath() {
  char path[1024];
  uint32_t size = sizeof(path);
  if (_NSGetExecutablePath(path, &size) == 0) {
    return std::string(path);
  }
  return "";
}

#endif

#include <filesystem>

static inline std::filesystem::path getAppDir() {
  return std::filesystem::path(getAppPath()).parent_path();
}
