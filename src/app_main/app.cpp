#include "pch.h"
#include "app.h"
#include "slang_context.h"
#include "logger_helper.h"
#include "utils.h"
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
  }
  ~Impl() {
    SPDLOG_INFO("App End");
  }

  void OnWindowClose() override {
    running_ = false;
  }
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

  Window window{ kAppName, window_size.width, window_size.height };
  window.AddWindowEventHandler(impl_);
  window.Show();

  SlangContext context(window.GetWindowHandle(), window_size);

  // Main loop
  SPDLOG_INFO("Entering main loop");
  while (impl_->running_) {
    auto frame_context = context.BeginFrame();
    context.EndFrame();

    window.ProcessEvent();
  }
  SPDLOG_INFO("Exiting main loop");

  return 0;
}
