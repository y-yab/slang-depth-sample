#include "pch.h"
#include "slang_helper.h"
#include "slang_debug_printer.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cmath>
#include <vector>

namespace yab {

static constexpr auto kTargetProfile = "sm_6_1";

class DebugCallback : public rhi::IDebugCallback {
public:
  static DebugCallback* GetInstance() {
    static DebugCallback instance;
    return &instance;
  }

  virtual SLANG_NO_THROW void SLANG_MCALL handleMessage(
    rhi::DebugMessageType type,
    rhi::DebugMessageSource source,
    const char* message) override
  {
    switch (type) {
      case rhi::DebugMessageType::Error:
        SPDLOG_ERROR("[RHI][{}]: {}", magic_enum::enum_name(source), message);
        break;
      case rhi::DebugMessageType::Warning:
        SPDLOG_WARN("[RHI][{}]: {}", magic_enum::enum_name(source), message);
        break;
      case rhi::DebugMessageType::Info:
        SPDLOG_INFO("[RHI][{}]: {}", magic_enum::enum_name(source), message);
        break;
      default:
        SPDLOG_INFO("[RHI][{}]: {}", magic_enum::enum_name(source), message);
        break;
    }
  }
};

class Strings {
  std::vector<std::string> values_;
  std::vector<const char*> values_ref_;
  bool finalized_{ false };

public:
  void Append(const std::string_view& value) {
    assert(!finalized_);
    values_.emplace_back(value);
  }

  void Finalize() {
    assert(!finalized_);
    values_ref_.resize(values_.size());
    std::transform(values_.begin(), values_.end(), values_ref_.begin(), [](const std::string& str) {
      return str.c_str();
    });
    finalized_ = true;
  }

  auto Data() const {
    assert(finalized_);
    return values_ref_.data();
  }

  auto Size() const {
    assert(finalized_);
    return values_ref_.size();
  }
};

}

using namespace yab;
using namespace sgl;

Slang::ComPtr<rhi::IDevice> SlangHelper::CreateDevice() {
  rhi::DeviceDesc desc{};
  desc.deviceType = rhi::DeviceType::D3D12;

#ifdef _DEBUG
  rhi::getRHI()->enableDebugLayers();
  desc.enableValidation = true;
  desc.debugCallback = DebugCallback::GetInstance();
#endif

  std::array<slang::CompilerOptionEntry, 2> options{ {
    {slang::CompilerOptionName::EmitSpirvDirectly, {slang::CompilerOptionValueKind::Int, 1}},
    {slang::CompilerOptionName::DebugInformation, {slang::CompilerOptionValueKind::Int, SLANG_DEBUG_INFO_LEVEL_STANDARD}},
  } };

  desc.slang.compilerOptionEntries = options.data();
  desc.slang.compilerOptionEntryCount = static_cast<uint32_t>(options.size());

  return rhi::getRHI()->createDevice(desc);
}
