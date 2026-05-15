#include "pch.h"
#include "slang_renderer_texture.h"
#include "slang_context.h"
#include "slang_helper.h"
#include "slang_index_buffer.h"
#include "slang_vertex_buffer.h"

#include <slang-rhi.h>

using namespace yab;

namespace fs = std::filesystem;

namespace {
namespace detail {

constexpr float kIdentity[] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };

} // namespace detail
} // namespace (anonymous)

struct SlangRendererTexture::Impl {
  std::shared_ptr<SlangContext> context_;
  bool is_reverse_z_{false};
  std::unique_ptr<SlangVertexBuffer> vertex_buffer_;
  std::unique_ptr<SlangIndexBuffer> index_buffer_;
  Slang::ComPtr<rhi::IRenderPipeline> pipeline_;
  Slang::ComPtr<rhi::ISampler> sampler_;

  Impl(std::shared_ptr<SlangContext> context, bool is_reverse_z)
    : context_(context), is_reverse_z_(is_reverse_z)
  {
    // Create buffers
    vertex_buffer_ = CreateVertexBuffer();
    index_buffer_ = CreateIndexBuffer();

    // Create pipeline
    pipeline_ = CreatePipeline();

    // Create sampler
    {
      rhi::SamplerDesc desc{};

      CHECKSLANG(
        context_->GetDevice()->createSampler(desc, sampler_.writeRef()),
        "failed to create sampler");
    }
  }

  Slang::ComPtr<rhi::IRenderPipeline> CreatePipeline() {
    auto device = context_->GetDevice();
    auto session = context_->GetSession();
    auto shader_dir = context_->GetShaderDir();

    // Create pipeline
    return SlangHelper::CreateRenderPipeline(
      device.get(), session.get(),
      shader_dir / "draw_texture.slang", "vs_main", "fs_main",
      context_->GetSurfaceFormat(),
      vertex_buffer_->GetInputLayout(),
      "Texture Renderer Pipeline",
      is_reverse_z_ ? rhi::ComparisonFunc::Greater : rhi::ComparisonFunc::Less);
  }

  std::unique_ptr<SlangVertexBuffer> CreateVertexBuffer() {
    std::vector<VertexUV> vertices = { {
      { { -1.f,  1.f, 0.f }, { 0.f, 0.f } },  // Top Left
      { {  1.f,  1.f, 0.f }, { 1.f, 0.f } },  // Top Right
      { { -1.f, -1.f, 0.f }, { 0.f, 1.f } },  // Bottom Left
      { {  1.f, -1.f, 0.f }, { 1.f, 1.f } },  // Bottom Right
    } };

    return std::make_unique<SlangVertexBuffer>(
      context_->GetDevice(), vertices, "Quad Vertex Buffer");
  }

  std::unique_ptr<SlangIndexBuffer> CreateIndexBuffer() {
    // CCW winding in screen space (Y-down): TL,BL,TR and TR,BL,BR
    std::vector<uint32_t> indices = { 0, 2, 1, 1, 2, 3 };

    return std::make_unique<SlangIndexBuffer>(
      context_->GetDevice(), indices, "Quad Index Buffer");
  }

  void Render(
    rhi::ICommandEncoder* encoder,
    rhi::ITexture* src_color_texture,
    rhi::ITexture* src_depth_texture,
    rhi::ITexture* render_target,
    rhi::ITexture* depth_target,
    bool clear_attachments)
  {
    auto rt_size = render_target->getDesc().size;
    auto viewport = rhi::Viewport::fromSize(
      static_cast<float>(rt_size.width), static_cast<float>(rt_size.height));
    auto scissor = rhi::ScissorRect::fromSize(rt_size.width, rt_size.height);

    rhi::RenderPassColorAttachment color_attachment{};
    color_attachment.view = render_target->getDefaultView();
    color_attachment.loadOp = clear_attachments ? rhi::LoadOp::Clear : rhi::LoadOp::Load;

    rhi::RenderPassDepthStencilAttachment depth_attachment{};
    depth_attachment.view = depth_target->getDefaultView();
    depth_attachment.depthLoadOp = clear_attachments ? rhi::LoadOp::Clear : rhi::LoadOp::Load;
    depth_attachment.depthStoreOp = rhi::StoreOp::Store;
    depth_attachment.depthClearValue = is_reverse_z_ ? 0.0f : 1.0f;

    rhi::RenderPassDesc render_desc{};
    render_desc.colorAttachmentCount = 1;
    render_desc.colorAttachments = &color_attachment;
    render_desc.depthStencilAttachment = &depth_attachment;

    auto render_encoder = encoder->beginRenderPass(render_desc);
    {
      auto shader_object = render_encoder->bindPipeline(pipeline_.get());

      rhi::ShaderCursor cursor(shader_object);
      cursor["Uniforms"]["wvp_matrix"].setData(detail::kIdentity, sizeof(float) * 16);
      cursor["Uniforms"]["src_color_texture"].setBinding(src_color_texture->getDefaultView());
      cursor["Uniforms"]["src_depth_texture"].setBinding(src_depth_texture->getDefaultView());
      cursor["Uniforms"]["sampler"].setBinding(sampler_);

      rhi::RenderState state{};
      state.viewportCount = 1;
      state.viewports[0] = viewport;
      state.scissorRectCount = 1;
      state.scissorRects[0] = scissor;
      state.vertexBufferCount = 1;
      state.vertexBuffers[0] = vertex_buffer_->GetBuffer();
      state.indexBuffer.buffer = index_buffer_->GetBuffer();
      state.indexFormat = rhi::IndexFormat::Uint32;
      render_encoder->setRenderState(state);

      rhi::DrawArguments draw_args{};
      draw_args.vertexCount = index_buffer_->GetNumIndices();
      render_encoder->drawIndexed(draw_args);
    }
    render_encoder->end();
  }

  void ReloadShader() {
    pipeline_ = CreatePipeline();
  }
};

SlangRendererTexture::SlangRendererTexture(std::shared_ptr<SlangContext> context, bool is_reverse_z)
  : impl_{ std::make_unique<Impl>(context, is_reverse_z) }
{
}

SlangRendererTexture::~SlangRendererTexture() {
}

void SlangRendererTexture::Render(
    rhi::ICommandEncoder* encoder,
    rhi::ITexture* src_color_texture,
    rhi::ITexture* src_depth_texture,
    rhi::ITexture* render_target,
    rhi::ITexture* depth_target,
    bool clear_attachments)
{
  impl_->Render(
    encoder,
    src_color_texture,
    src_depth_texture,
    render_target,
    depth_target,
    clear_attachments);
}

void SlangRendererTexture::ReloadShader() {
  impl_->ReloadShader();
}
