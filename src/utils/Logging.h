#pragma once

#include <spdlog/spdlog.h>

#include <sstream>
#include <string>
#include <string_view>

// Helper class for stream-based logging
class LogStream {
 public:
  LogStream(spdlog::level::level_enum level) : level_(level) {}

  ~LogStream() {
    std::string msg = message_.str();
    if (!msg.empty()) {
      const char* msg_cstr = msg.c_str();
      switch (level_) {
        case spdlog::level::trace:
          spdlog::trace("{}", msg_cstr);
          break;
        case spdlog::level::debug:
          spdlog::debug("{}", msg_cstr);
          break;
        case spdlog::level::info:
          spdlog::info("{}", msg_cstr);
          break;
        case spdlog::level::warn:
          spdlog::warn("{}", msg_cstr);
          break;
        case spdlog::level::err:
          spdlog::error("{}", msg_cstr);
          break;
        case spdlog::level::critical:
          spdlog::critical("{}", msg_cstr);
          break;
        default:
          spdlog::log(level_, "{}", msg_cstr);
          break;
      }
    }
  }

  template <typename T>
  LogStream& operator<<(const T& value) {
    message_ << value;
    return *this;
  }

  // Handle std::endl and other stream manipulators
  LogStream& operator<<(std::ostream& (*manip)(std::ostream&)) {
    manip(message_);
    return *this;
  }

 private:
  spdlog::level::level_enum level_;
  std::ostringstream message_;
};

// Macros for stream-based logging
#define LOG_TRACE() LogStream(spdlog::level::trace)
#define LOG_DEBUG() LogStream(spdlog::level::debug)
#define LOG_INFO() LogStream(spdlog::level::info)
#define LOG_WARN() LogStream(spdlog::level::warn)
#define LOG_ERROR() LogStream(spdlog::level::err)
#define LOG_CRITICAL() LogStream(spdlog::level::critical)

// Also provide format-based macros for convenience
#define LOG_TRACE_FMT(...) spdlog::trace(__VA_ARGS__)
#define LOG_DEBUG_FMT(...) spdlog::debug(__VA_ARGS__)
#define LOG_INFO_FMT(...) spdlog::info(__VA_ARGS__)
#define LOG_WARN_FMT(...) spdlog::warn(__VA_ARGS__)
#define LOG_ERROR_FMT(...) spdlog::error(__VA_ARGS__)
#define LOG_CRITICAL_FMT(...) spdlog::critical(__VA_ARGS__)
