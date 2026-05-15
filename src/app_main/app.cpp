#include "pch.h"
#include "app.h"
#include "config.h"
#include "logger_helper.h"
#include "utils.h"
#include "slang_context.h"
#include "slang_renderer_main.h"
#include "window.h"
#include "window_event_handler.h"

#include <atomic>

using namespace yab;

namespace {

constexpr auto kAppName = L"Slang Depth Sample";

} // namespace (anonymous)

struct App::Impl : public IWindowEventHandler {
  std::unique_ptr<LoggerRAII> logger_raii_;
  std::atomic<bool> running_{true};

  Impl() {
    auto exe_dir = Util::GetExecutionDir();

    LogDesc log_desc;
    log_desc.log_base_name_ = "slang_depth_sample";
    log_desc.enable_file_logger_ = true;
    log_desc.file_logger_type_ = FileLoggerType::Overwrite;
    log_desc.log_dir_ = exe_dir / "logs";
    logger_raii_ = std::make_unique<LoggerRAII>(log_desc);
    SPDLOG_INFO("App Begin");

    // Load config
    auto& config = Config::GetInstance();
    auto yaml_file = exe_dir / "slang_depth_sample.yaml";
    config.Load(yaml_file);
  }
  ~Impl() {
    SPDLOG_INFO("App End");
  }

  void OnWindowClose() override {
    running_ = false;
  }
  void OnWindowMove(int32_t x, int32_t y) override {}
  void OnWindowResize(uint32_t width, uint32_t height) override {}
  void OnKeyboard(uint32_t key_code, bool is_keydown) override {
  }
  void OnMouseButton(MouseButton button, MouseAction action, float x, float y) override {
  }
  void OnMouseMove(float x, float y) override {
  }
  void OnMouseWheel(float delta) override {
  }
};

App::App() : impl_(std::make_shared<Impl>()) {}
App::~App() = default;

int App::Run() {
  Size window_size{1280, 720};

  Window window({
    kAppName,
    {0, 0, window_size.width, window_size.height},
    false, // fullscreen
    false, // topmost
    true,  // hook_f10key
  });
  window.AddWindowEventHandler(impl_);
  window.Show();

  auto context = std::make_shared<SlangContext>(
    window.GetWindowHandle(), window_size, Util::GetExecutionDir() / "shader");

  SlangRendererMain renderer(context);

  // Main loop
  SPDLOG_INFO("Entering main loop");
  while (impl_->running_) {
    renderer.Render();
    window.ProcessEvent();
  }
  SPDLOG_INFO("Exiting main loop");

  return 0;
}
