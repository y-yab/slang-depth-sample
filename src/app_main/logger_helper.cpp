#include "pch.h"
#include "logger_helper.h"

using namespace yab;

namespace fs = std::filesystem;

namespace {
namespace detail {

std::string FindAvailableFileName(
  const fs::path& dir, const std::string& base_name, const std::string& ext)
{
  static std::regex re(std::format("^{}-(0|[1-9]+[0-9]*)\\.{}$", base_name, ext));
  int largest_suffix = -1;
  for (const auto& it : fs::directory_iterator(dir)) {
    auto filename = it.path().filename().string();
    if (std::smatch match; std::regex_match(filename, match, re)) {
      try {
        largest_suffix = std::max(largest_suffix, std::stoi(match[1].str()));
      }
      catch (const std::exception&) {
        // Ignore files with unexpected names
      }
    }
  }
  return std::format("{}-{}.{}", (dir / base_name).string(), largest_suffix + 1, ext);
}

} // namespace detail
} // namespace (anonymous)

void LoggerHelper::InitLog(const LogDesc& desc) {
  std::vector<spdlog::sink_ptr> sinks;

  // Default stderr output
  // sinks.emplace_back(std::make_shared<spdlog::sinks::wincolor_stderr_sink_mt>());

  // Setup console logger
  if (desc.enable_console_logger_) {
    sinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
  }

  // Setup file logger
  if (desc.enable_file_logger_) {
    std::filesystem::create_directories(desc.log_dir_);

    if (desc.file_logger_type_ == FileLoggerType::AlwaysNew) {
      // Find an available filename
      auto log_file = detail::FindAvailableFileName(desc.log_dir_, desc.log_base_name_, desc.log_file_ext_);
      sinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_file));
    }
    else if (desc.file_logger_type_ == FileLoggerType::RotatingNew) {
      auto log_file = std::format("{}.{}",
        (desc.log_dir_ / desc.log_base_name_).string(), desc.log_file_ext_);
      sinks.emplace_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        log_file, desc.max_file_size_, desc.max_num_files_, true));
    }
    else {
      auto truncate = desc.file_logger_type_ == FileLoggerType::Overwrite;
      auto log_file = std::format("{}.{}",
        (desc.log_dir_ / desc.log_base_name_).string(), desc.log_file_ext_);
      sinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_file, truncate));
    }
  }

  // Setup callback logger
  if (desc.enable_callback_logger_) {
    sinks.emplace_back(std::make_shared<spdlog::sinks::callback_sink_mt>(desc.callback_func_));
  }

  std::shared_ptr<spdlog::logger> logger;
  if (desc.async_) {
    spdlog::init_thread_pool(128 * 1024, 2); // queue with 128k items and 2 backing thread.
    logger = std::make_shared<spdlog::async_logger>(
      desc.log_base_name_, sinks.begin(), sinks.end(), spdlog::thread_pool());
  }
  else {
    logger = std::make_shared<spdlog::logger>(
      desc.log_base_name_, sinks.begin(), sinks.end());
  }
  spdlog::set_default_logger(logger);
  spdlog::flush_on(spdlog::level::err);
#ifdef _DEBUG
  spdlog::set_level(spdlog::level::debug);
  spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e][%n][%l][%s %# %!] %v");
#else
  spdlog::set_level(spdlog::level::info);
  spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e][%n][%l][---] %v");  // Mask source location
#endif
}

void LoggerHelper::FlushLog() {
  if (spdlog::default_logger()) {
    spdlog::default_logger()->flush();
  }
}

void LoggerHelper::TermLog() {
  spdlog::drop_all();
  spdlog::shutdown();
}
