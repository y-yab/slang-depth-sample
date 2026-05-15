#pragma once

#include "non_copyable.h"
#include "types.h"

#include <filesystem>
#include <memory>

namespace DirectX {
struct XMMATRIX;
};

namespace rhi {
class ICommandEncoder;
class ITexture;
};

namespace yab {

class SlangContext;

class SlangRendererTexture : public NonCopyable {
public:
  SlangRendererTexture() = delete;
  SlangRendererTexture(std::shared_ptr<SlangContext> context, bool is_reverse_z);
  ~SlangRendererTexture();

  void Render(
    rhi::ICommandEncoder* encoder,
    rhi::ITexture* src_color_texture,
    rhi::ITexture* src_depth_texture,
    rhi::ITexture* render_target,
    rhi::ITexture* depth_target,
    bool is_src_reverse_z,
    bool clear_attachments);
  void ReloadShader();

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace yab
