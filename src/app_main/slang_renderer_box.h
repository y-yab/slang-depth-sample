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
};

namespace yab {

class SlangContext;

class SlangRendererBox : public NonCopyable {
public:
  SlangRendererBox() = delete;
  SlangRendererBox(std::shared_ptr<SlangContext> context, const Vec3f& size);
  ~SlangRendererBox();

  void Render(
    rhi::ICommandEncoder* encoder,
    rhi::ITexture* render_target,
    const DirectX::XMMATRIX& world,
    const DirectX::XMMATRIX& view,
    const DirectX::XMMATRIX& proj);
  void ReloadShader();

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace yab
