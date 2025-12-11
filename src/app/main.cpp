#include <execinfo.h>
#include <spdlog/common.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QIcon>
#include <QStandardPaths>
#include <QString>
#include <array>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>
#include <vector>

#include "ui/MainWindow.h"
#include "utils/Logging.h"

namespace {
constexpr int kMaxStackTraceFrames = 50;
}  // namespace

// Signal handler for crash diagnostics
auto CrashHandler(int sig) -> void {
  const char* signal_name = "UNKNOWN";
  switch (sig) {
    case SIGSEGV:
      signal_name = "SIGSEGV";
      break;
    case SIGABRT:
      signal_name = "SIGABRT";
      break;
    case SIGFPE:
      signal_name = "SIGFPE";
      break;
    case SIGILL:
      signal_name = "SIGILL";
      break;
    // NOLINTNEXTLINE(misc-include-cleaner) SIGBUS is defined in <csignal>
    case SIGBUS:
      signal_name = "SIGBUS";
      break;
    default:
      signal_name = "UNKNOWN";
      break;
  }

  // Try to log, but if logging is not initialized, write to stderr
  try {
    LOG_CRITICAL() << "Received signal: " << signal_name << " (" << sig << ")";
  } catch (...) {
    // Logging not available, write to stderr
    // NOLINTNEXTLINE(modernize-use-std-print) std::println may not be available
    std::cerr << "CRITICAL: Received signal: " << signal_name << " (" << sig
              << ")\n";
  }

  // Print stack trace
  std::array<void*, kMaxStackTraceFrames> array{};
  const size_t size = backtrace(array.data(), kMaxStackTraceFrames);

  // Try to log stack trace, but if logging is not initialized, write to stderr
  try {
    LOG_CRITICAL() << "Stack trace (" << size << " frames):";
  } catch (...) {
    // NOLINTNEXTLINE(modernize-use-std-print) std::println may not be available
    std::cerr << "Stack trace (" << size << " frames):\n";
  }
  // bugprone-multi-level-implicit-pointer-conversion) backtrace_symbols is a C
  // library function that uses malloc and returns char** The conversion from
  // char** to void* is required by the C API
  char** messages = backtrace_symbols(array.data(), static_cast<int>(size));
  if (messages != nullptr) {
    for (size_t i = 0; i < size; i++) {
      try {
        LOG_CRITICAL() << "  [" << i << "] "
                       << (messages[i] != nullptr ? messages[i] : "?");
      } catch (...) {
        // NOLINTNEXTLINE(modernize-use-std-print) std::println may not be
        // available
        std::cerr << "  [" << i << "] "
                  << (messages[i] != nullptr ? messages[i] : "?") << "\n";
      }
    }
    free(reinterpret_cast<void*>(
      messages));  // NOLINT(cppcoreguidelines-no-malloc,bugprone-multi-level-implicit-pointer-conversion)
  }

  spdlog::shutdown();
  std::abort();
}

auto SetupLogging() -> void {
  // Get log file path
  // above
  QString app_name = QApplication::applicationName();
  if (app_name.isEmpty()) {
    app_name = "NIRMaterialEditor";
  }

  QString data_dir =
    QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  if (data_dir.isEmpty()) {
    data_dir = QDir::currentPath();
  }

  QDir dir(data_dir);
  if (!dir.exists()) {
    if (!dir.mkpath(".")) {
      // Failed to create directory, fallback to current directory
      data_dir = QDir::currentPath();
      dir = QDir(data_dir);
    }
  }

  const QString log_path = dir.absoluteFilePath(app_name + "_log.txt");

  // Create file sink with error handling
  std::shared_ptr<spdlog::sinks::basic_file_sink_mt> file_sink;
  try {
    file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
      log_path.toStdString(), true);
    file_sink->set_level(spdlog::level::trace);
  } catch (const std::exception&) {  // NOLINT(bugprone-empty-catch)
    // Failed to create file sink, continue with console sink only
    // This is not critical - application can work without file logging
    // Exception is intentionally ignored to allow graceful degradation
  }

  // Create console sink
  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  console_sink->set_level(spdlog::level::info);

  // Combine sinks (only add file_sink if it was created successfully)
  std::vector<spdlog::sink_ptr> sinks;
  if (file_sink != nullptr) {
    sinks.push_back(file_sink);
  }
  sinks.push_back(console_sink);
  auto logger =
    std::make_shared<spdlog::logger>("multi_sink", sinks.begin(), sinks.end());

  logger->set_level(spdlog::level::trace);
  logger->flush_on(spdlog::level::warn);  // Flush on warnings and above
  logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%s:%#] %v");

  spdlog::set_default_logger(logger);
  if (file_sink != nullptr) {
    const std::string log_path_str = log_path.toStdString();
    LOG_INFO_FMT("Logging initialized. Log file: {}", log_path_str.c_str());
  } else {
    LOG_INFO() << "Logging initialized (console only, file logging failed)";
  }
}

auto main(int argc, char* argv[]) -> int {
  // Install signal handlers for crash diagnostics
  std::signal(SIGSEGV, CrashHandler);
  std::signal(SIGABRT, CrashHandler);
  std::signal(SIGFPE, CrashHandler);
  std::signal(SIGILL, CrashHandler);
  std::signal(SIGBUS, CrashHandler);

  // QApplication cannot be const because exec() modifies internal state
  // However, we can make it const until exec() is called
  const QApplication app(argc, argv);
  QApplication::setApplicationName("NIRMaterialEditor");
  QApplication::setWindowIcon(QIcon(":/icons/app.svg"));

  SetupLogging();
  LOG_INFO() << "Application starting";

  try {
    MainWindow main_window;
    main_window.show();

    LOG_INFO() << "MainWindow shown, entering event loop";

    // app.exec() modifies internal state, so app cannot be const
    const int result = QApplication::exec();

    LOG_INFO() << "Application exiting with code: " << result;
    spdlog::shutdown();

    return result;
  } catch (const std::exception& e) {
    LOG_CRITICAL() << "Unhandled exception in main: " << e.what();
    spdlog::shutdown();
    return 1;
  } catch (...) {
    LOG_CRITICAL() << "Unhandled unknown exception in main";
    spdlog::shutdown();
    return 1;
  }
}
