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
  std::unique_ptr<SlangRendererBox> box_renderer_;
  bool is_reverse_z_{false};

  Impl(std::shared_ptr<SlangContext> context) : context_(context) {
    box_renderer_ = std::make_unique<SlangRendererBox>(context_, Vec3f{1.0f, 1.0f, 1.0f});
  }

  ~Impl() {
  }

  void Render() {
    // View matrix
    DirectX::XMVECTOR eye = DirectX::XMVectorSet(1.f, 1.f, 1.f, 1.f);
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
      Pose pose{.position = { 0.f, 0.f, 0.f }, .orientation = { 0.f, 0.f, 0.f, 1.f } };
      auto world = Util::ToXmMatrix(pose);
      box_renderer_->Render(
        frame_context.command_encoder.get(),
        frame_context.render_target.get(),
        world, view, proj);
    }

    // End frame
    context_->EndFrame(frame_context);
    context_->Present();
  }

  void ReloadShader() {
    box_renderer_->ReloadShader();
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
