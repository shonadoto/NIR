#include <execinfo.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <unistd.h>

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QIcon>
#include <QStandardPaths>
#include <csignal>
#include <cstdlib>

#include "ui/MainWindow.h"
#include "utils/Logging.h"

// Signal handler for crash diagnostics
void crash_handler(int sig) {
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
  }

  LOG_CRITICAL() << "Received signal: " << signal_name << " (" << sig << ")";

  // Print stack trace
  void* array[50];
  size_t size = backtrace(array, 50);

  LOG_CRITICAL() << "Stack trace (" << size << " frames):";
  char** messages = backtrace_symbols(array, size);
  for (size_t i = 0; i < size; i++) {
    LOG_CRITICAL() << "  [" << i << "] " << (messages[i] ? messages[i] : "?");
  }
  free(messages);

  spdlog::shutdown();
  std::abort();
}
#include <format>

void setup_logging() {
  // Get log file path
  QString appName = QApplication::applicationName();
  if (appName.isEmpty()) {
    appName = "NIRMaterialEditor";
  }

  QString dataDir =
    QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  if (dataDir.isEmpty()) {
    dataDir = QDir::currentPath();
  }

  QDir dir(dataDir);
  if (!dir.exists()) {
    dir.mkpath(".");
  }

  QString logPath = dir.absoluteFilePath(appName + "_log.txt");

  // Create file sink
  auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
    logPath.toStdString(), true);
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
  LOG_INFO_FMT("Logging initialized. Log file: {}", logPath.toStdString());
}

int main(int argc, char* argv[]) {
  // Install signal handlers for crash diagnostics
  std::signal(SIGSEGV, crash_handler);
  std::signal(SIGABRT, crash_handler);
  std::signal(SIGFPE, crash_handler);
  std::signal(SIGILL, crash_handler);
  std::signal(SIGBUS, crash_handler);

  QApplication app(argc, argv);
  app.setApplicationName("NIRMaterialEditor");
  app.setWindowIcon(QIcon(":/icons/app.svg"));

  setup_logging();
  LOG_INFO() << "Application starting";

  MainWindow mainWindow;
  mainWindow.show();

  LOG_INFO() << "MainWindow shown, entering event loop";

  int result = app.exec();

  LOG_INFO() << "Application exiting with code: " << result;
  spdlog::shutdown();

  return result;
}
