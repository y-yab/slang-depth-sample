#pragma once

#include <functional>
#include <memory>
#include <string>
#include <string_view>

#include <slang-rhi.h>

namespace rhi {
struct ShaderCursor;
}

namespace sgl {

class DebugPrinter {
public:
  DebugPrinter() = delete;
  DebugPrinter(const DebugPrinter&) = delete;
  DebugPrinter& operator=(const DebugPrinter&) = delete;

  DebugPrinter(rhi::IDevice* device, uint64_t buf_size);
  ~DebugPrinter();

  void LoadStrings(slang::ProgramLayout* slang_reflection);
  void Sink(rhi::IDevice* device, std::function<void(const std::string_view&)> sink);

  void Bind(rhi::ShaderCursor cursor);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}
