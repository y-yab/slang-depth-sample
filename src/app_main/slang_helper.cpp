#include "pch.h"
#include "slang_helper.h"
#include "slang_debug_printer.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cmath>
#include <vector>

namespace {

constexpr auto kTargetProfile = "sm_6_1";

namespace detail {

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

} // namespace detail
} // namespace (anonymous)

using namespace yab;
using namespace sgl;

Slang::ComPtr<rhi::IDevice> SlangHelper::CreateDevice() {
  rhi::DeviceDesc desc{};
  desc.deviceType = rhi::DeviceType::D3D12;

#ifdef _DEBUG
  rhi::getRHI()->enableDebugLayers();
  desc.enableValidation = true;
  desc.debugCallback = detail::DebugCallback::GetInstance();
#endif

  std::array<slang::CompilerOptionEntry, 2> options{ {
    {slang::CompilerOptionName::EmitSpirvDirectly, {slang::CompilerOptionValueKind::Int, 1}},
    {slang::CompilerOptionName::DebugInformation, {slang::CompilerOptionValueKind::Int, SLANG_DEBUG_INFO_LEVEL_STANDARD}},
  } };

  desc.slang.compilerOptionEntries = options.data();
  desc.slang.compilerOptionEntryCount = static_cast<uint32_t>(options.size());

  return rhi::getRHI()->createDevice(desc);
}

Slang::ComPtr<slang::ISession> SlangHelper::CreateSession(
  slang::IGlobalSession* global_session, const std::filesystem::path& shader_path)
{
  Slang::ComPtr<slang::ISession> session;

  if (!global_session) {
    return session;
  }

  detail::Strings search_paths;
  search_paths.Append(shader_path.string());
  search_paths.Finalize();

  slang::TargetDesc target_desc{};
  target_desc.format = SLANG_DXIL;
  target_desc.profile = global_session->findProfile(kTargetProfile);

  std::array<slang::PreprocessorMacroDesc, 1> macros{ {
    #ifdef _DEBUG
    {"SGL_ENABLE_PRINT", "1"},
    #else
    {"SGL_ENABLE_PRINT", "0"},
    #endif
  } };

  slang::SessionDesc session_desc{};
  session_desc.searchPathCount = static_cast<uint32_t>(search_paths.Size());
  session_desc.searchPaths = search_paths.Data();
  session_desc.preprocessorMacroCount = static_cast<uint32_t>(macros.size());
  session_desc.preprocessorMacros = macros.data();
  session_desc.targetCount = 1;
  session_desc.targets = &target_desc;

  CHECKSLANG(
    global_session->createSession(session_desc, session.writeRef()),
    "Failed to create Slang session");

  return session;
}

Slang::ComPtr<rhi::IRenderPipeline> SlangHelper::CreateRenderPipeline(
  rhi::IDevice* device, slang::ISession* session,
  const std::filesystem::path& shader_file,
  const std::string_view& vs_entry_point_name,
  const std::string_view& fs_entry_point_name,
  rhi::Format render_target_format,
  Slang::ComPtr<rhi::IInputLayout> input_layout,
  const std::string_view& label)
{
  // Load shader
  Slang::ComPtr<rhi::IShaderProgram> shader;
  {
    Slang::ComPtr<slang::IBlob> diagnostics;
    auto module = session->loadModule(shader_file.string().c_str(), diagnostics.writeRef());
    DIAGNOSESLANG(diagnostics);
    if (!module) {
      auto msg = std::format("Failed to load shader file({})", shader_file.string());
      SPDLOG_ERROR(msg);
      SlangHelper::ThrowException(msg);
      return nullptr;
    }

    Slang::ComPtr<slang::IEntryPoint> vs;
    CHECKSLANG(
      module->findEntryPointByName(vs_entry_point_name.data(), vs.writeRef()),
      "Failed to find vertex shader entry point");

    Slang::ComPtr<slang::IEntryPoint> fs;
    CHECKSLANG(
      module->findEntryPointByName(fs_entry_point_name.data(), fs.writeRef()),
      "Failed to find fragment shader entry point");

    std::vector<slang::IComponentType*> component_types;
    component_types.push_back(module);
    component_types.push_back(vs.get());
    component_types.push_back(fs.get());

    Slang::ComPtr<slang::IComponentType> linked_program;
    auto res = session->createCompositeComponentType(
      component_types.data(),
      component_types.size(),
      linked_program.writeRef(),
      diagnostics.writeRef());
    DIAGNOSESLANG(diagnostics);
    CHECKSLANG(res, "Failed to link shader program");

    rhi::ShaderProgramDesc desc{};
    desc.slangGlobalScope = linked_program;
    CHECKSLANG(
      device->createShaderProgram(desc, shader.writeRef()),
      "Failed to create shader program");
  }

  // Create pipeline
  Slang::ComPtr<rhi::IRenderPipeline> pipeline;
  {
    rhi::ColorTargetDesc color_target;
    color_target.format = render_target_format;
    color_target.enableBlend = true;
    color_target.color.srcFactor = rhi::BlendFactor::SrcAlpha;
    color_target.color.dstFactor = rhi::BlendFactor::InvSrcAlpha;
    color_target.color.op = rhi::BlendOp::Add;
    color_target.alpha.srcFactor = rhi::BlendFactor::One;
    color_target.alpha.dstFactor = rhi::BlendFactor::InvSrcAlpha;
    color_target.alpha.op = rhi::BlendOp::Add;

    rhi::RenderPipelineDesc desc{};
    desc.inputLayout = input_layout.get();
    desc.program = shader;
    desc.targetCount = 1;
    desc.targets = &color_target;
    desc.depthStencil.depthTestEnable = true;
    desc.depthStencil.depthWriteEnable = true;
    desc.primitiveTopology = rhi::PrimitiveTopology::TriangleList;
    desc.rasterizer.cullMode = rhi::CullMode::Back;
    if (!label.empty()) {
      desc.label = label.data();
    }

    CHECKSLANG(
      device->createRenderPipeline(desc, pipeline.writeRef()),
      "Failed to create render pipeline");
  }

  return pipeline;
}

void SlangHelper::ThrowException(const std::string_view& message) {
  SPDLOG_CRITICAL(message.data());
  throw std::runtime_error(message.data());
}

void SlangHelper::Diagnose(Slang::ComPtr<slang::IBlob> diagnostics) {
  if (diagnostics) {
    SPDLOG_INFO("[RHI][DIAG]: {}", static_cast<const char*>(diagnostics->getBufferPointer()));
  }
}
