#pragma once

#include "non_copyable.h"
#include "types.h"

#include <slang-rhi.h>
#include <memory>
#include <string_view>

namespace yab {

struct VertexColor {
  Vec3f position;
  Color color;
};
using VertexDataColor = std::vector<VertexColor>;

struct VertexUV {
  Vec3f position;
  Vec2f uv;
};
using VertexDataUV = std::vector<VertexUV>;

class SlangVertexBuffer : NonCopyable {
public:
  SlangVertexBuffer() = delete;
  SlangVertexBuffer(rhi::IDevice* device, const VertexDataColor& data, const std::string_view& label);
  SlangVertexBuffer(rhi::IDevice* device, const VertexDataUV& data, const std::string_view& label);
  ~SlangVertexBuffer();

  Slang::ComPtr<rhi::IBuffer> GetBuffer() const;
  Slang::ComPtr<rhi::IInputLayout> GetInputLayout() const;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace yab
