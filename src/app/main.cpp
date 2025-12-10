#include <execinfo.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QIcon>
#include <QStandardPaths>
#include <QString>
#include <array>
#include <csignal>
#include <cstdlib>
#include <format>
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
    case SIGBUS:
      signal_name = "SIGBUS";
      break;
    default:
      signal_name = "UNKNOWN";
      break;
  }

  LOG_CRITICAL() << "Received signal: " << signal_name << " (" << sig << ")";

  // Print stack trace
  std::array<void*, kMaxStackTraceFrames> array{};
  const size_t size = backtrace(array.data(), kMaxStackTraceFrames);

  LOG_CRITICAL() << "Stack trace (" << size << " frames):";
  // NOLINTNEXTLINE(cppcoreguidelines-owning-memory,
  // bugprone-multi-level-implicit-pointer-conversion) backtrace_symbols is a C
  // library function that uses malloc and returns char** The conversion from
  // char** to void* is required by the C API
  // NOLINTNEXTLINE(bugprone-multi-level-implicit-pointer-conversion)
  void* messages_void = backtrace_symbols(array.data(), static_cast<int>(size));
  // NOLINTNEXTLINE(bugprone-multi-level-implicit-pointer-conversion)
  char** messages = static_cast<char**>(messages_void);
  if (messages != nullptr) {
    for (size_t i = 0; i < size; i++) {
      LOG_CRITICAL() << "  [" << i << "] "
                     << (messages[i] != nullptr ? messages[i] : "?");
    }
    // NOLINTNEXTLINE(cppcoreguidelines-no-malloc,bugprone-multi-level-implicit-pointer-conversion)
    free(messages);
  }

  spdlog::shutdown();
  std::abort();
}

auto SetupLogging() -> void {
  // Get log file path
  QString app_name = QApplication::applicationName();
  if (app_name.isEmpty()) {
    app_name = "NIRMaterialEditor";
  }

  QString data_dir =
    QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  if (data_dir.isEmpty()) {
    data_dir = QDir::currentPath();
  }

  const QDir dir(data_dir);
  if (!dir.exists()) {
    dir.mkpath(".");
  }

  const QString log_path = dir.absoluteFilePath(app_name + "_log.txt");

  // Create file sink
  auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
    log_path.toStdString(), true);
  file_sink->set_level(spdlog::level::trace);

  // Create console sink
  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  console_sink->set_level(spdlog::level::info);

  // Combine sinks
  std::vector<spdlog::sink_ptr> sinks{file_sink, console_sink};
  auto logger =
    std::make_shared<spdlog::logger>("multi_sink", sinks.begin(), sinks.end());

  logger->set_level(spdlog::level::trace);
  logger->flush_on(spdlog::level::warn);  // Flush on warnings and above
  logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%s:%#] %v");

  spdlog::set_default_logger(logger);
  const std::string log_path_str = log_path.toStdString();
  LOG_INFO_FMT("Logging initialized. Log file: {}", log_path_str.c_str());
}

auto main(int argc, char* argv[]) -> int {
  // Install signal handlers for crash diagnostics
  std::signal(SIGSEGV, CrashHandler);
  std::signal(SIGABRT, CrashHandler);
  std::signal(SIGFPE, CrashHandler);
  std::signal(SIGILL, CrashHandler);
  std::signal(SIGBUS, CrashHandler);

  // QApplication cannot be const because exec() modifies internal state
  // NOLINTNEXTLINE(misc-const-correctness)
  QApplication app(argc, argv);
  QApplication::setApplicationName("NIRMaterialEditor");
  QApplication::setWindowIcon(QIcon(":/icons/app.svg"));

  SetupLogging();
  LOG_INFO() << "Application starting";

  MainWindow main_window;
  main_window.show();

  LOG_INFO() << "MainWindow shown, entering event loop";

  // app.exec() modifies internal state, so app cannot be const
  // NOLINTNEXTLINE(readability-static-accessed-through-instance)
  const int result = app.exec();

  LOG_INFO() << "Application exiting with code: " << result;
  spdlog::shutdown();

  return result;
}
