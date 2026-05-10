#include "pch.h"
#include "slang_context.h"
#include "slang_helper.h"

#include <filesystem>

constexpr uint32_t kSwapchainImageCount = 3;

namespace fs = std::filesystem;
using namespace yab;
using namespace sgl;

struct SlangContext::Impl {
  Slang::ComPtr<rhi::IDevice> device_;
  Slang::ComPtr<rhi::ICommandQueue> command_queue_;
  Slang::ComPtr<rhi::ISurface> surface_;
  Slang::ComPtr<rhi::ICommandEncoder> current_command_encoder_;
  Slang::ComPtr<rhi::ITexture> current_render_target_;


  Impl(HWND hwnd, const Size& size) {
    // Create device
    device_ = SlangHelper::CreateDevice();

    // Get command queue
    command_queue_ = device_->getQueue(rhi::QueueType::Graphics);

    // Create surface
    {
      surface_ = device_->createSurface(rhi::WindowHandle::fromHwnd(hwnd));

      rhi::SurfaceConfig surface_config{};
      surface_config.format = surface_->getInfo().preferredFormat;
      surface_config.width = size.width;
      surface_config.height = size.height;
      surface_config.desiredImageCount = kSwapchainImageCount;
      surface_->configure(surface_config);
    }
  }

  ~Impl() {
    command_queue_->waitOnHost();
  }

  SlangContext::FrameContext BeginFrame() {
    current_command_encoder_ = command_queue_->createCommandEncoder();
    current_render_target_ = surface_->acquireNextImage();
    if (!current_command_encoder_ || !current_render_target_) {
      current_command_encoder_ = nullptr;
      current_render_target_ = nullptr;
      return {};
    }

    FrameContext frame_context;
    frame_context.command_encoder = current_command_encoder_.get();
    frame_context.render_target = current_render_target_.get();
    return frame_context;
  }

  void EndFrame() {
    if (!current_command_encoder_ || !current_render_target_) {
      return;
    }

    command_queue_->submit(current_command_encoder_->finish());
    surface_->present();

    current_command_encoder_ = nullptr;
    current_render_target_ = nullptr;
  }
};

SlangContext::SlangContext(HWND hwnd, const Size& size)
  : impl_{ std::make_unique<Impl>(hwnd, size) }
{
}

SlangContext::~SlangContext() {
}

rhi::IDevice* SlangContext::GetDevice() const {
  return impl_->device_.get();
}

SlangContext::FrameContext SlangContext::BeginFrame() {
  return impl_->BeginFrame();
}

void SlangContext::EndFrame() {
  impl_->EndFrame();
}
