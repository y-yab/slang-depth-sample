#include "pch.h"
#include "slang_renderer_main.h"
#include "slang_context.h"
#include "slang_helper.h"
#include "slang_renderer_box.h"
#include "utils.h"

using namespace DirectX;
using namespace yab;

struct SlangRendererMain::Impl {
  std::shared_ptr<SlangContext> context_;
  std::filesystem::path shader_dir_;
  Slang::ComPtr<rhi::ITexture> depth_texture_;
  std::unique_ptr<SlangRendererBox> box_renderer1_;
  std::unique_ptr<SlangRendererBox> box_renderer2_;
  bool is_reverse_z_{false};

  Impl(std::shared_ptr<SlangContext> context) : context_(context) {
    // Create depth texture
    {
      auto device = context_->GetDevice();
      auto surface_size_ = context_->GetSurfaceSize();

      rhi::TextureDesc depth_desc{};
      depth_desc.type = rhi::TextureType::Texture2D;
      depth_desc.size.width = surface_size_.width;
      depth_desc.size.height = surface_size_.height;
      depth_desc.size.depth = 1;
      depth_desc.format = rhi::Format::D32Float;
      depth_desc.usage = rhi::TextureUsage::DepthStencil;
      depth_desc.defaultState = rhi::ResourceState::DepthWrite;

      CHECKSLANG(
        device->createTexture(depth_desc, nullptr, depth_texture_.writeRef()),
        "Failed to create depth texture");
    }

    // Create renderers
    box_renderer1_ = std::make_unique<SlangRendererBox>(context_, Vec3f{0.2f, 0.2f, 8.0f});
    box_renderer2_ = std::make_unique<SlangRendererBox>(context_, Vec3f{1.0f, 1.0f, 0.2f});
  }

  ~Impl() {
  }

  void Render() {
    // View matrix
    DirectX::XMVECTOR eye = DirectX::XMVectorSet(0.5f, 0.8f, 1.f, 1.f);
    DirectX::XMVECTOR at = DirectX::XMVectorSet(0.f, 0.f, 0.f, 1.f);
    DirectX::XMVECTOR up_dir = DirectX::XMVectorSet(0.f, 1.f, 0.f, 1.f);
    auto view = DirectX::XMMatrixLookAtLH(eye, at, up_dir);

    // Projection matrix
    auto surface_size = context_->GetSurfaceSize();
    float fov = DirectX::XMConvertToRadians(90);
    auto aspect = static_cast<float>(surface_size.width) / surface_size.height;
    auto z_near = is_reverse_z_ ? FLT_MAX : 0.1f;
    auto z_far = is_reverse_z_ ? 0.1f : FLT_MAX;
    auto proj = DirectX::XMMatrixPerspectiveFovLH(fov, aspect, z_near, z_far);

    // Begin frame
    auto frame_context = context_->BeginFrame();

    // Render box
    {
      Pose pose{.position = { 0.f, 0.f, -3.2f }, .orientation = { 0.f, 0.f, 0.f, 1.f } };
      auto world = Util::ToXmMatrix(pose);
      box_renderer1_->Render(
        frame_context.command_encoder.get(),
        frame_context.render_target.get(),
        depth_texture_.get(),
        true,
        world, view, proj);
    }
    {
      Euler rotation{ .roll = XMConvertToRadians(-90.f), .pitch = 0.f, .yaw = XMConvertToRadians(180.f) };
      auto world = Util::ToXmMatrix({0.f, 0.f, 0.f}, rotation);
      box_renderer2_->Render(
        frame_context.command_encoder.get(),
        frame_context.render_target.get(),
        depth_texture_.get(),
        false,
        world, view, proj);
    }

    // End frame
    context_->EndFrame(frame_context);
    context_->Present();
  }

  void ReloadShader() {
    box_renderer1_->ReloadShader();
    box_renderer2_->ReloadShader();
  }
};

SlangRendererMain::SlangRendererMain(
  std::shared_ptr<SlangContext> context) : impl_{ std::make_unique<Impl>(context) }
{
}

SlangRendererMain::~SlangRendererMain() {
}

void SlangRendererMain::Render() {
  impl_->Render();
}

void SlangRendererMain::ReloadShader() {
  impl_->ReloadShader();
}
