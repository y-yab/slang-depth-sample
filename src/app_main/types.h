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

struct Vec4f {
  float x;
  float y;
  float z;
  float w;
};

struct Color {
  float r;
  float g;
  float b;
  float a;
};

struct Pose {
  Vec3f position;
  Vec4f orientation; // Quaternion (x, y, z, w)
};

} // namespace yab
