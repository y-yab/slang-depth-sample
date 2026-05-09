#include "pch.h"
#include "app.h"
#include "logger_helper.h"
#include "utils.h"

using namespace yab;

struct App::Impl {
  std::unique_ptr<LoggerRAII> logger_raii_;

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
};

App::App() : impl_(std::make_unique<Impl>()) {}
App::~App() = default;

int App::Run() {
  return 0;
}
