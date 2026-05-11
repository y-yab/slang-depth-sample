#include "pch.h"
#include "slang_index_buffer.h"
#include "slang_helper.h"

using namespace yab;

struct SlangIndexBuffer::Impl {
  Slang::ComPtr<rhi::IBuffer> buffer_;
  uint32_t num_indices_{};

  Impl(rhi::IDevice* device, const std::vector<uint32_t>& data, const std::string_view& label) {
    num_indices_ = static_cast<uint32_t>(data.size());

    rhi::BufferDesc desc{};
    desc.size = static_cast<uint64_t>(num_indices_ * sizeof(uint32_t));
    desc.usage = rhi::BufferUsage::IndexBuffer;
    desc.defaultState = rhi::ResourceState::IndexBuffer;
    desc.label = label.data();

    CHECKSLANG(
      device->createBuffer(desc, data.data(), buffer_.writeRef()),
      "Failed to create index buffer");
  }
};

SlangIndexBuffer::SlangIndexBuffer(rhi::IDevice* device, const std::vector<uint32_t>& data, const std::string_view& label)
  : impl_{ std::make_unique<Impl>(device, data, label) }
{
}

SlangIndexBuffer::~SlangIndexBuffer() {
}

Slang::ComPtr<rhi::IBuffer> SlangIndexBuffer::GetBuffer() const {
  return impl_->buffer_;
}

uint32_t SlangIndexBuffer::GetNumIndices() const {
  return impl_->num_indices_;
}
