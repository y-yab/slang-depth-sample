#pragma once

#include <slang-rhi.h>

#include <filesystem>
#include <format>
#include <stdexcept>

#ifndef SPDLOG_CRITICAL
#define SPDLOG_CRITICAL(...)
#endif

#ifndef SPDLOG_INFO
#define SPDLOG_INFO(...)
#endif

namespace sgl {
class DebugPrinter;
}

namespace yab {

#define CHECKSLANG(result, prefix, ...)                                     \
  if (SLANG_FAILED(result)) {                                               \
    auto suffix = std::format(": (0x{:x})", static_cast<int32_t>(result));  \
    auto full_message = std::format(prefix, __VA_ARGS__) + suffix;          \
    SPDLOG_CRITICAL(full_message);                                          \
    throw std::runtime_error(full_message);                                 \
  }                                                                         \

#define DIAGNOSESLANG(diagnostics)  \
  if (diagnostics != nullptr) {     \
    SPDLOG_INFO("[RHI][DIAG]: {}", static_cast<const char*>(diagnostics->getBufferPointer())); \
  }                                 \

class GPUPrinting;

class SlangHelper {
public:
  static Slang::ComPtr<rhi::IDevice> CreateDevice();
  static Slang::ComPtr<slang::ISession> CreateSession(
    slang::IGlobalSession* global_session, const std::filesystem::path& shader_path);
  static Slang::ComPtr<rhi::IRenderPipeline> CreateRenderPipeline(
    rhi::IDevice* device, slang::ISession* slang_session,
    const std::filesystem::path& shader_file,
    const std::string_view& vs_entry_point,
    const std::string_view& fs_entry_point,
    rhi::Format render_target_format,
    Slang::ComPtr<rhi::IInputLayout> input_layout,
    const std::string_view& label = "",
    rhi::ComparisonFunc depth_func = rhi::ComparisonFunc::Less);

  static void ThrowException(const std::string_view& message);
  static void Diagnose(Slang::ComPtr<slang::IBlob> diagnostics);
};

}
