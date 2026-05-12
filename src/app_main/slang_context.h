#pragma once

#include "non_copyable.h"
#include "types.h"

#include <slang-rhi.h>

#include <filesystem>
#include <memory>
#include <stdint.h>
#include <windef.h>

namespace yab {

class SlangContext : public NonCopyable {
public:
  struct FrameContext {
    Slang::ComPtr<rhi::ICommandEncoder> command_encoder;
    Slang::ComPtr<rhi::ITexture> render_target;
    Slang::ComPtr<rhi::ITexture> depth_target;

    bool IsValid() const { return command_encoder && render_target && depth_target; }
  };

  SlangContext() = delete;
  SlangContext(
    HWND hwnd, const Size& surface_size,
    const std::filesystem::path& shader_dir);
  ~SlangContext();

  Slang::ComPtr<rhi::IDevice> GetDevice() const;
  Slang::ComPtr<slang::ISession> GetSession() const;
  std::filesystem::path GetShaderDir() const;

  Size GetSurfaceSize() const;
  rhi::Format GetSurfaceFormat() const;

  FrameContext BeginFrame();
  void EndFrame(FrameContext frame_context);

  void Present();

  void RefreshSession();
  void WaitOnHost();

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}
