#pragma once

#pragma warning(push)
#pragma warning(disable:4275)
#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/callback_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#pragma warning(pop)

#include <filesystem>
#include <functional>
#include <format>
#include <regex>
#include <string_view>

namespace yab {

enum class FileLoggerType {
  AlwaysNew,
  RotatingNew,
  Append,
  Overwrite,
};

struct LogDesc {
  std::string log_base_name_;
  bool enable_console_logger_{};
  bool enable_callback_logger_{};
  bool enable_file_logger_{};
  FileLoggerType file_logger_type_{FileLoggerType::Overwrite};
  std::filesystem::path log_dir_;
  std::string log_file_ext_{"log"};
  uint32_t max_file_size_{4 * 1024 * 1024};
  uint32_t max_num_files_{3};
  std::function<void(const spdlog::details::log_msg &)> callback_func_;
  bool async_{};
};

inline void InitLog(const LogDesc& desc) {
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
      static std::regex re(std::format("^{}-(0|[1-9]+[0-9]*).log$", desc.log_base_name_));
      int largest_suffix = -1;
      for (const auto& it : std::filesystem::directory_iterator(desc.log_dir_)) {
        auto filename = it.path().filename().string();
        if (std::smatch match; std::regex_match(filename, match, re)) {
          largest_suffix = std::max(largest_suffix, std::stoi(match[1].str()));
        }
      }
      auto log_file = std::format("{}-{}.{}",
        (desc.log_dir_ / desc.log_base_name_).string(), largest_suffix + 1, desc.log_file_ext_);
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
};

inline void FlushLog() {
  if (spdlog::default_logger()) {
    spdlog::default_logger()->flush();
  }
}

inline void TermLog() {
  spdlog::drop_all();
  spdlog::shutdown();
}

class LoggerRAII {
public:
  LoggerRAII(const LogDesc& desc) {
    InitLog(desc);
  }

  ~LoggerRAII() {
    TermLog();
  }
};

}
