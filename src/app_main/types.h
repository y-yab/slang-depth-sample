#pragma once

#include <stdint.h>

namespace yab {

struct Size {
  uint32_t width;
  uint32_t height;
};

struct Vec2f {
  float x;
  float y;
};

struct Vec3f {
  float x;
  float y;
  float z;
};

struct Color {
  float r;
  float g;
  float b;
  float a;
};

} // namespace yab
