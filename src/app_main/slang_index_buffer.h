#pragma once

#include "non_copyable.h"
#include "types.h"

#include <slang-rhi.h>
#include <memory>
#include <string_view>
#include <vector>

namespace yab {

class SlangIndexBuffer : NonCopyable {
public:
  SlangIndexBuffer() = delete;
  SlangIndexBuffer(rhi::IDevice* device, const std::vector<uint32_t>& data, const std::string_view& label);
  ~SlangIndexBuffer();

  Slang::ComPtr<rhi::IBuffer> GetBuffer() const;
  uint32_t GetNumIndices() const;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace yab
