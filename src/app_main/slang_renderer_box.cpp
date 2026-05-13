#include "pch.h"
#include "slang_renderer_box.h"
#include "slang_context.h"
#include "slang_helper.h"
#include "slang_index_buffer.h"
#include "slang_vertex_buffer.h"

#include <slang-rhi.h>

using namespace yab;

namespace fs = std::filesystem;

namespace {

constexpr auto kRed = Color{ 1.0f, 0.0f, 0.0f, 1.0f };
constexpr auto kGreen = Color{ 0.0f, 1.0f, 0.0f, 1.0f };
constexpr auto kBlue = Color{ 0.0f, 0.0f, 1.0f, 1.0f };
constexpr auto kYellow = Color{ 1.0f, 1.0f, 0.0f, 1.0f };
constexpr auto kCyan = Color{ 0.0f, 1.0f, 1.0f, 1.0f };
constexpr auto kMagenta = Color{ 1.0f, 0.0f, 1.0f, 1.0f };

} // namespace (anonymous)

struct SlangRendererBox::Impl {
  std::shared_ptr<SlangContext> context_;
  Vec3f size_;
  std::unique_ptr<SlangVertexBuffer> vertex_buffer_;
  std::unique_ptr<SlangIndexBuffer> index_buffer_;
  Slang::ComPtr<rhi::IRenderPipeline> pipeline_;

  Impl(std::shared_ptr<SlangContext> context, const Vec3f& size)
    : context_(context), size_(size)
  {
    // Create buffers
    vertex_buffer_ = CreateVertexBuffer();
    index_buffer_ = CreateIndexBuffer();

    // Create pipeline
    pipeline_ = CreatePipeline();
  }

  Slang::ComPtr<rhi::IRenderPipeline> CreatePipeline() {
    auto device = context_->GetDevice();
    auto session = context_->GetSession();
    auto shader_dir = context_->GetShaderDir();

    // Create pipeline
    return SlangHelper::CreateRenderPipeline(
      device.get(), session.get(),
      shader_dir / "box.slang", "vs_main", "fs_main",
      context_->GetSurfaceFormat(),
      vertex_buffer_->GetInputLayout(),
      "Box Renderer Pipeline");
  }

  std::unique_ptr<SlangVertexBuffer> CreateVertexBuffer() {
    const float half_width = size_.x / 2.0f;
    const float half_height = size_.y / 2.0f;
    const float half_depth = size_.z / 2.0f;

    std::vector<VertexColor> vertices = { {
      // Front face
      { { -half_width, -half_height, -half_depth }, kYellow }, // 0
      { {  half_width, -half_height, -half_depth }, kYellow }, // 1
      { {  half_width,  half_height, -half_depth }, kYellow }, // 2
      { { -half_width,  half_height, -half_depth }, kYellow }, // 3
      // Back face
      { { -half_width, -half_height,  half_depth }, kBlue }, // 4
      { { -half_width,  half_height,  half_depth }, kBlue }, // 5
      { {  half_width,  half_height,  half_depth }, kBlue }, // 6
      { {  half_width, -half_height,  half_depth }, kBlue }, // 7
      // Top face
      { { -half_width,  half_height, -half_depth }, kGreen }, // 8
      { {  half_width,  half_height, -half_depth }, kGreen }, // 9
      { {  half_width,  half_height,  half_depth }, kGreen }, // 10
      { { -half_width,  half_height,  half_depth }, kGreen }, // 11
      // Bottom face
      { { -half_width, -half_height, -half_depth }, kCyan }, // 12
      { { -half_width, -half_height,  half_depth }, kCyan }, // 13
      { {  half_width, -half_height,  half_depth }, kCyan }, // 14
      { {  half_width, -half_height, -half_depth }, kCyan }, // 15
      // Left face
      { { -half_width, -half_height,  half_depth }, kMagenta }, // 16
      { { -half_width, -half_height, -half_depth }, kMagenta }, // 17
      { { -half_width,  half_height, -half_depth }, kMagenta }, // 18
      { { -half_width,  half_height,  half_depth }, kMagenta }, // 19
      // Right face
      { {  half_width, -half_height, -half_depth }, kRed }, // 20
      { {  half_width, -half_height,  half_depth }, kRed }, // 21
      { {  half_width,  half_height,  half_depth }, kRed }, // 22
      { {  half_width,  half_height, -half_depth }, kRed }, // 23
    } };

    return std::make_unique<SlangVertexBuffer>(
      context_->GetDevice(), vertices, "Box Vertex Buffer");
  }

  std::unique_ptr<SlangIndexBuffer> CreateIndexBuffer() {
    std::vector<uint32_t> indices = {
      // Front face
      0, 1, 2,
      0, 2, 3,
      // Back face
      4, 5, 6,
      4, 6, 7,
      // Top face
      8, 9, 10,
      8, 10, 11,
      // Bottom face
      12, 13, 14,
      12, 14, 15,
      // Left face
      16, 17, 18,
      16, 18, 19,
      // Right face
      20, 21, 22,
      20, 22, 23,
    };

    return std::make_unique<SlangIndexBuffer>(
      context_->GetDevice(), indices, "Box Index Buffer");
  }

  void Render(
    rhi::ICommandEncoder* encoder,
    rhi::ITexture* render_target,
    rhi::ITexture* depth_target,
    bool clear_attachments,
    const DirectX::XMMATRIX& world,
    const DirectX::XMMATRIX& view,
    const DirectX::XMMATRIX& proj)
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

    rhi::RenderPassDesc render_desc{};
    render_desc.colorAttachmentCount = 1;
    render_desc.colorAttachments = &color_attachment;
    render_desc.depthStencilAttachment = &depth_attachment;

    auto render_encoder = encoder->beginRenderPass(render_desc);
    {
      auto shader_object = render_encoder->bindPipeline(pipeline_.get());

      rhi::ShaderCursor cursor(shader_object);
      cursor["Uniforms"]["world_matrix"].setData(&world, sizeof(float) * 16);
      cursor["Uniforms"]["view_matrix"].setData(&view, sizeof(float) * 16);
      cursor["Uniforms"]["proj_matrix"].setData(&proj, sizeof(float) * 16);

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

SlangRendererBox::SlangRendererBox(
  std::shared_ptr<SlangContext> context, const Vec3f& size)
  : impl_{ std::make_unique<Impl>(context, size) }
{
}

SlangRendererBox::~SlangRendererBox() {
}

void SlangRendererBox::Render(
    rhi::ICommandEncoder* encoder,
    rhi::ITexture* render_target,
    rhi::ITexture* depth_target,
    bool clear_attachments,
    const DirectX::XMMATRIX& world,
    const DirectX::XMMATRIX& view,
    const DirectX::XMMATRIX& proj)
{
  impl_->Render(
    encoder,
    render_target,
    depth_target,
    clear_attachments,
    world,
    view,
    proj);
}

void SlangRendererBox::ReloadShader() {
  impl_->ReloadShader();
}
