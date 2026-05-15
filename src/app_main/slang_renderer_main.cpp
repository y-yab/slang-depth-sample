#include "pch.h"
#include "slang_renderer_main.h"
#include "slang_context.h"
#include "slang_helper.h"
#include "slang_renderer_box.h"
#include "slang_renderer_texture.h"
#include "config.h"
#include "utils.h"

using namespace DirectX;
using namespace yab;

struct SlangRendererMain::Impl {
  std::shared_ptr<SlangContext> context_;
  std::filesystem::path shader_dir_;
  Slang::ComPtr<rhi::ITexture> intermediate_color_texture1_;
  Slang::ComPtr<rhi::ITexture> intermediate_color_texture2_;
  Slang::ComPtr<rhi::ITexture> intermediate_depth_texture1_;
  Slang::ComPtr<rhi::ITexture> intermediate_depth_texture2_;
  Slang::ComPtr<rhi::ITexture> depth_texture_;
  std::unique_ptr<SlangRendererBox> box_renderer1_;
  std::unique_ptr<SlangRendererBox> box_renderer2_;
  std::unique_ptr<SlangRendererTexture> texture_renderer1_;
  std::unique_ptr<SlangRendererTexture> texture_renderer2_;
  bool is_box1_reverse_z_{};
  bool is_box2_reverse_z_{};
  bool is_render_target_reverse_z_{};
  bool is_texture_composition_{};

  Impl(std::shared_ptr<SlangContext> context) : context_(context) {
    auto device = context_->GetDevice();
    auto surface_size_ = context_->GetSurfaceSize();

    // Load config
    const auto& config = Config::GetInstance();
    is_box1_reverse_z_ = config.is_box1_reverse_z_;
    is_box2_reverse_z_ = config.is_box2_reverse_z_;
    is_render_target_reverse_z_ = config.is_render_target_reverse_z_;
    is_texture_composition_ = config.is_texture_composition_;

    // Create textures
    intermediate_color_texture1_ = CreateColorTexture(surface_size_, "Intermediate Color Texture 1");
    intermediate_color_texture2_ = CreateColorTexture(surface_size_, "Intermediate Color Texture 2");
    intermediate_depth_texture1_ = CreateDepthTexture(
      surface_size_, "Intermediate Depth Texture 1",
      rhi::TextureUsage::DepthStencil | rhi::TextureUsage::ShaderResource);
    intermediate_depth_texture2_ = CreateDepthTexture(
      surface_size_, "Intermediate Depth Texture 2",
      rhi::TextureUsage::DepthStencil | rhi::TextureUsage::ShaderResource);
    depth_texture_ = CreateDepthTexture(surface_size_, "Depth Texture");

    // Create renderers
    box_renderer1_ = std::make_unique<SlangRendererBox>(context_, Vec3f{0.2f, 0.2f, 8.0f}, is_box1_reverse_z_);
    box_renderer2_ = std::make_unique<SlangRendererBox>(context_, Vec3f{1.0f, 1.0f, 0.2f}, is_box2_reverse_z_);
    texture_renderer1_ = std::make_unique<SlangRendererTexture>(context_, is_render_target_reverse_z_);
    texture_renderer2_ = std::make_unique<SlangRendererTexture>(context_, is_render_target_reverse_z_);
  }

  ~Impl() {
  }

  Slang::ComPtr<rhi::ITexture> CreateColorTexture(
    const Size& size, const std::string_view& label = "",
    rhi::TextureUsage usage = rhi::TextureUsage::RenderTarget | rhi::TextureUsage::ShaderResource,
    rhi::ResourceState default_state = rhi::ResourceState::RenderTarget)
  {
    rhi::TextureDesc desc{};
    desc.type = rhi::TextureType::Texture2D;
    desc.size.width = size.width;
    desc.size.height = size.height;
    desc.format = context_->GetSurfaceFormat();
    desc.usage = usage;
    desc.defaultState = default_state;
    desc.label = label.data();

    Slang::ComPtr<rhi::ITexture> texture;
    CHECKSLANG(
      context_->GetDevice()->createTexture(desc, nullptr, texture.writeRef()),
      "Failed to create color texture");
    return texture;
  }

  Slang::ComPtr<rhi::ITexture> CreateDepthTexture(
    const Size& size, const std::string_view& label = "",
    rhi::TextureUsage usage = rhi::TextureUsage::DepthStencil,
    rhi::ResourceState default_state = rhi::ResourceState::DepthWrite)
  {
    rhi::TextureDesc desc{};
    desc.type = rhi::TextureType::Texture2D;
    desc.size.width = size.width;
    desc.size.height = size.height;
    desc.format = rhi::Format::D32Float;
    desc.usage = usage;
    desc.defaultState = default_state;
    desc.label = label.data();

    Slang::ComPtr<rhi::ITexture> texture;
    CHECKSLANG(
      context_->GetDevice()->createTexture(desc, nullptr, texture.writeRef()),
      "Failed to create depth texture");
    return texture;
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

    // Begin frame
    auto frame_context = context_->BeginFrame();

    if (is_texture_composition_) {
      // Pass 1: Render box1 into intermediate texture set 1.
      {
        auto z_near = is_box1_reverse_z_ ? FLT_MAX : 0.1f;
        auto z_far = is_box1_reverse_z_ ? 0.1f : FLT_MAX;
        auto proj = DirectX::XMMatrixPerspectiveFovLH(fov, aspect, z_near, z_far);

        Pose pose{.position = { 0.f, 0.f, -3.2f }, .orientation = { 0.f, 0.f, 0.f, 1.f } };
        auto world = Util::ToXmMatrix(pose);
        box_renderer1_->Render(
          frame_context.command_encoder.get(),
          intermediate_color_texture1_.get(),
          intermediate_depth_texture1_.get(),
          true,
          world, view, proj);
      }

      // Pass 2: Render box2 into intermediate texture set 2.
      {
        auto z_near = is_box2_reverse_z_ ? FLT_MAX : 0.1f;
        auto z_far = is_box2_reverse_z_ ? 0.1f : FLT_MAX;
        auto proj = DirectX::XMMatrixPerspectiveFovLH(fov, aspect, z_near, z_far);

        Euler rotation{ .roll = XMConvertToRadians(-90.f), .pitch = 0.f, .yaw = XMConvertToRadians(180.f) };
        auto world = Util::ToXmMatrix({0.f, 0.f, 0.f}, rotation);
        box_renderer2_->Render(
          frame_context.command_encoder.get(),
          intermediate_color_texture2_.get(),
          intermediate_depth_texture2_.get(),
          true,
          world, view, proj);
      }

      // Transition intermediate textures to ShaderResource / DepthRead for sampling.
      frame_context.command_encoder->setTextureState(
        intermediate_color_texture1_.get(), rhi::ResourceState::ShaderResource);
      frame_context.command_encoder->setTextureState(
        intermediate_depth_texture1_.get(), rhi::ResourceState::DepthRead);
      frame_context.command_encoder->setTextureState(
        intermediate_color_texture2_.get(), rhi::ResourceState::ShaderResource);
      frame_context.command_encoder->setTextureState(
        intermediate_depth_texture2_.get(), rhi::ResourceState::DepthRead);
      frame_context.command_encoder->globalBarrier();

      // Pass 3: Composite two intermediate textures onto the back buffer with depth.
      texture_renderer1_->Render(
        frame_context.command_encoder.get(),
        intermediate_color_texture1_.get(),
        intermediate_depth_texture1_.get(),
        frame_context.render_target.get(),
        depth_texture_.get(),
        is_box1_reverse_z_,
        true);

      texture_renderer2_->Render(
        frame_context.command_encoder.get(),
        intermediate_color_texture2_.get(),
        intermediate_depth_texture2_.get(),
        frame_context.render_target.get(),
        depth_texture_.get(),
        is_box2_reverse_z_,
        false);

      // Restore intermediate textures to their default states for the next frame.
      frame_context.command_encoder->setTextureState(
        intermediate_color_texture1_.get(), rhi::ResourceState::RenderTarget);
      frame_context.command_encoder->setTextureState(
        intermediate_depth_texture1_.get(), rhi::ResourceState::DepthWrite);
      frame_context.command_encoder->setTextureState(
        intermediate_color_texture2_.get(), rhi::ResourceState::RenderTarget);
      frame_context.command_encoder->setTextureState(
        intermediate_depth_texture2_.get(), rhi::ResourceState::DepthWrite);
      frame_context.command_encoder->globalBarrier();
    }
    else {
      auto z_near = is_render_target_reverse_z_ ? FLT_MAX : 0.1f;
      auto z_far = is_render_target_reverse_z_ ? 0.1f : FLT_MAX;
      auto proj = DirectX::XMMatrixPerspectiveFovLH(fov, aspect, z_near, z_far);

      // Render two boxes directly to back buffer
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
    }

    // End frame
    context_->EndFrame(frame_context);
    context_->Present();
  }

  void ReloadShader() {
    box_renderer1_->ReloadShader();
    box_renderer2_->ReloadShader();
    texture_renderer1_->ReloadShader();
    texture_renderer2_->ReloadShader();
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
