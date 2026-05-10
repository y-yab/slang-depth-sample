#pragma once

#include "non_copyable.h"
#include "types.h"

#include <memory>
#include <stdint.h>
#include <windef.h>

namespace rhi {
class IDevice;
class ICommandEncoder;
class ITexture;
}

namespace yab {

class SlangContext : public NonCopyable {
public:
  struct FrameContext {
    rhi::ICommandEncoder* command_encoder = nullptr;
    rhi::ITexture* render_target = nullptr;
  };

  SlangContext() = delete;
  SlangContext(HWND hwnd, const Size& size);
  ~SlangContext();

  rhi::IDevice* GetDevice() const;
  FrameContext BeginFrame();
  void EndFrame();

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}
