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
  Slang::ComPtr<slang::IGlobalSession> slang_global_session_;
  Slang::ComPtr<slang::ISession> slang_session_;
  Slang::ComPtr<rhi::ICommandQueue> command_queue_;
  Slang::ComPtr<rhi::ISurface> surface_;
  Slang::ComPtr<rhi::ITexture> depth_texture_;
  Size surface_size_;
  rhi::Format surface_format_;
  std::filesystem::path shader_dir_;


  Impl(HWND hwnd, const Size& size, const std::filesystem::path& shader_dir)
    : surface_size_(size), shader_dir_(shader_dir)
  {
    // Create device
    device_ = SlangHelper::CreateDevice();

    // Create Slang session
    CHECKSLANG(
      slang::createGlobalSession(slang_global_session_.writeRef()),
      "Failed to create Slang global session");
    slang_session_ = SlangHelper::CreateSession(slang_global_session_.get(), shader_dir);

    // Get command queue
    command_queue_ = device_->getQueue(rhi::QueueType::Graphics);

    // Create surface
    {
      surface_ = device_->createSurface(rhi::WindowHandle::fromHwnd(hwnd));
      surface_format_ = surface_->getInfo().preferredFormat;

      rhi::SurfaceConfig surface_config{};
      surface_config.format = surface_format_;
      surface_config.width = surface_size_.width;
      surface_config.height = surface_size_.height;
      surface_config.desiredImageCount = kSwapchainImageCount;
      surface_->configure(surface_config);
    }

    // Create a persistent depth texture shared by all render passes in a frame.
    {
      rhi::TextureDesc depth_desc{};
      depth_desc.type = rhi::TextureType::Texture2D;
      depth_desc.size.width = surface_size_.width;
      depth_desc.size.height = surface_size_.height;
      depth_desc.size.depth = 1;
      depth_desc.format = rhi::Format::D32Float;
      depth_desc.usage = rhi::TextureUsage::DepthStencil;
      depth_desc.defaultState = rhi::ResourceState::DepthWrite;

      CHECKSLANG(
        device_->createTexture(depth_desc, nullptr, depth_texture_.writeRef()),
        "Failed to create depth texture");
    }
  }

  ~Impl() {
    command_queue_->waitOnHost();
  }

  SlangContext::FrameContext BeginFrame() {
    return {
      .command_encoder = command_queue_->createCommandEncoder(),
      .render_target = surface_->acquireNextImage(),
      .depth_target = depth_texture_,
    };
  }

  void EndFrame(rhi::ICommandEncoder* command_encoder) {
    command_queue_->submit(command_encoder->finish());
  }

  void Present() {
    surface_->present();
  }
};

SlangContext::SlangContext(HWND hwnd, const Size& size, const std::filesystem::path& shader_dir)
  : impl_{ std::make_unique<Impl>(hwnd, size, shader_dir) }
{
}

SlangContext::~SlangContext() {
}

Slang::ComPtr<rhi::IDevice> SlangContext::GetDevice() const {
  return impl_->device_;
}

Slang::ComPtr<slang::ISession> SlangContext::GetSession() const {
  return impl_->slang_session_;
}

std::filesystem::path SlangContext::GetShaderDir() const {
  return impl_->shader_dir_;
}

Size SlangContext::GetSurfaceSize() const {
  return impl_->surface_size_;
}

rhi::Format SlangContext::GetSurfaceFormat() const {
  return impl_->surface_format_;
}

SlangContext::FrameContext SlangContext::BeginFrame() {
  return impl_->BeginFrame();
}

void SlangContext::EndFrame(FrameContext frame_context) {
  impl_->EndFrame(frame_context.command_encoder.get());
}

void SlangContext::Present() {
  impl_->Present();
}

void SlangContext::RefreshSession() {
  auto new_session = SlangHelper::CreateSession(
    impl_->slang_global_session_.get(), impl_->shader_dir_);

  impl_->command_queue_->waitOnHost();
  impl_->slang_session_ = new_session;
}

void SlangContext::WaitOnHost() {
  impl_->command_queue_->waitOnHost();
}
