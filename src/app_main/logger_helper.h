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

class LoggerHelper {
public:
  static void InitLog(const LogDesc& desc);
  static void FlushLog();
  static void TermLog();
};

class LoggerRAII {
public:
  LoggerRAII(const LogDesc& desc) {
    LoggerHelper::InitLog(desc);
  }

  ~LoggerRAII() {
    LoggerHelper::TermLog();
  }
};

} // namespace yab
