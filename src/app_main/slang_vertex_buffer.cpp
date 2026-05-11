#include "pch.h"
#include "slang_vertex_buffer.h"
#include "slang_helper.h"

using namespace yab;

struct SlangVertexBuffer::Impl {
  Slang::ComPtr<rhi::IBuffer> buffer_;
  Slang::ComPtr<rhi::IInputLayout> input_layout_;

  Impl(rhi::IDevice* device, const VertexDataColor& data, const std::string_view& label) {
    // Vertex Buffer
    CreateVertexBuffer(
      device,
      data.data(),
      static_cast<uint64_t>(data.size() * sizeof(VertexColor)),
      static_cast<uint32_t>(sizeof(VertexColor)),
      label);

    // Input Layout
    std::array<rhi::InputElementDesc, 2> elems{ {
      {"POSITION", 0, rhi::Format::RGB32Float, offsetof(VertexColor, position) },
      {"COLOR", 0, rhi::Format::RGBA32Float, offsetof(VertexColor, color) },
    } };
    CHECKSLANG(
      device->createInputLayout(
        sizeof(VertexColor),
        elems.data(), static_cast<uint32_t>(elems.size()), input_layout_.writeRef()),
      "Failed to create input layout");
  }

  Impl(rhi::IDevice* device, const VertexDataUV& data, const std::string_view& label) {
    // Vertex Buffer
    CreateVertexBuffer(
      device,
      data.data(),
      static_cast<uint64_t>(data.size() * sizeof(VertexUV)),
      static_cast<uint32_t>(sizeof(VertexUV)),
      label);

    // Input Layout
    std::array<rhi::InputElementDesc, 2> input_elements{ {
      {"POSITION", 0, rhi::Format::RGB32Float, offsetof(VertexUV, position) },
      {"TEXCOORD", 0, rhi::Format::RG32Float, offsetof(VertexUV, uv) },
    } };
    CHECKSLANG(
      device->createInputLayout(
        sizeof(VertexUV),
        input_elements.data(), static_cast<uint32_t>(input_elements.size()), input_layout_.writeRef()),
      "Failed to create input layout");
  }

  void CreateVertexBuffer(rhi::IDevice* device, const void* data, uint64_t size_in_bytes, uint32_t element_size, const std::string_view& label) {
    rhi::BufferDesc desc{};
    desc.format = rhi::Format::RGB32Float;
    desc.size = size_in_bytes;
    desc.elementSize = element_size;
    desc.usage = rhi::BufferUsage::VertexBuffer;
    desc.label = label.data();
    CHECKSLANG(
      device->createBuffer(desc, data, buffer_.writeRef()),
      "Failed to create vertex buffer");
  }
};

SlangVertexBuffer::SlangVertexBuffer(rhi::IDevice* device, const VertexDataColor& data, const std::string_view& label)
  : impl_{ std::make_unique<Impl>(device, data, label) }
{
}

SlangVertexBuffer::SlangVertexBuffer(rhi::IDevice* device, const VertexDataUV& data, const std::string_view& label)
  : impl_{ std::make_unique<Impl>(device, data, label) }
{
}

SlangVertexBuffer::~SlangVertexBuffer() {
}

Slang::ComPtr<rhi::IBuffer> SlangVertexBuffer::GetBuffer() const {
  return impl_->buffer_;
}

Slang::ComPtr<rhi::IInputLayout> SlangVertexBuffer::GetInputLayout() const {
  return impl_->input_layout_;
}
