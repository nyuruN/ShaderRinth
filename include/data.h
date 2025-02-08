#pragma once

#include <GLES3/gl3.h>
#include <any>
#include <array>
#include <optional>

#include <cereal/cereal.hpp>

// DataType:
// Type of the data transferred between nodes
enum DataType {
  Int = 0,       // int
  IVec2 = 1,     // std::array<int, 2>
  IVec3 = 2,     // std::array<int, 2>
  IVec4 = 3,     // std::array<int, 2>
  Float = 4,     // float
  Vec2 = 5,      // std::array<float, 2>
  Vec3 = 6,      // std::array<float, 2>
  Vec4 = 7,      // std::array<float, 2>
  Texture2D = 8, // GLuint
};

struct Data {
  using Int = int;
  using IVec2 = std::array<int, 2>;
  using IVec3 = std::array<int, 2>;
  using IVec4 = std::array<int, 2>;
  using Float = float;
  using Vec2 = std::array<float, 2>;
  using Vec3 = std::array<float, 2>;
  using Vec4 = std::array<float, 2>;
  using Texture2D = GLuint;

  inline static constexpr DataType ALL[] = {
      DataType::Int,   DataType::IVec2, DataType::IVec3,
      DataType::IVec4, DataType::Float, DataType::Vec2,
      DataType::Vec3,  DataType::Vec4,  DataType::Texture2D,
  };
  inline static constexpr char *NAME[] = {
      "Int",       // Int
      "IVec2",     // IVec2
      "IVec3",     // IVec3
      "IVec4",     // IVec4
      "Float",     // Float
      "Vec2",      // Vec2
      "Vec3",      // Vec3
      "Vec4",      // Vec4
      "Texture2D", // Texture2D
  };
  // Pin + Link colors
  static const unsigned int COLORS[];
  // Pin + Link colors
  static const unsigned int COLORS_HOVER[];

  DataType type;
  std::any data;

  Data(DataType type = DataType(-1), std::any data = nullptr)
      : type(type), data(data) {}
  constexpr bool operator==(DataType t) { return type == t; }
  operator bool() const { return data.has_value(); }

  template <typename T> T get() { return std::any_cast<T>(data); }
  template <typename T> std::optional<T> try_get() {
    try {
      return std::optional(get<T>());
    } catch (std::bad_any_cast) {
      return std::optional<T>();
    }
  }
  void set(std::any d) { data = d; }
  void reset() { data.reset(); }
  constexpr static char *type_name(DataType type) { return NAME[type]; }
  char *type_name() { return type_name(type); }
  template <class Archive> void serialize(Archive &ar) { ar(type); }
};
