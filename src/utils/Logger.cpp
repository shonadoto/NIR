#include "Logger.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QStandardPaths>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QStringConverter>
#endif

Logger& Logger::instance() {
  static Logger instance;
  return instance;
}

void Logger::initialize(const QString& logFilePath) {
  QMutexLocker locker(&mutex_);

  if (initialized_) {
    return;
  }

  if (logFilePath.isEmpty()) {
    // Default: save in application data directory or current directory
    QString appName = QCoreApplication::applicationName();
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

    log_file_path_ = dir.absoluteFilePath(appName + "_log.txt");
  } else {
    log_file_path_ = logFilePath;
  }

  log_file_.setFileName(log_file_path_);
  if (!log_file_.open(QIODevice::WriteOnly | QIODevice::Append |
                      QIODevice::Text)) {
    // Fallback to stderr
    qWarning() << "Failed to open log file:" << log_file_path_;
    log_file_path_.clear();
  } else {
    log_stream_.setDevice(&log_file_);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    log_stream_.setCodec("UTF-8");
#else
    log_stream_.setEncoding(QStringConverter::Utf8);
#endif
  }

  initialized_ = true;

  // Log initialization
  QString initMsg =
    QString("=== Logging started at %1 ===\n")
      .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz"));
  writeToFile(initMsg);
}

void Logger::shutdown() {
  // Don't block on mutex during shutdown - use tryLock
  if (!mutex_.tryLock(100)) {
    // Can't lock, just try to flush and close without logging
    if (log_file_.isOpen()) {
      log_stream_.flush();
      log_file_.flush();
      log_file_.close();
    }
    return;
  }

  // Manual lock management - ensure unlock
  if (!initialized_) {
    mutex_.unlock();
    return;
  }

  QString shutdownMsg =
    QString("=== Logging stopped at %1 ===\n\n")
      .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz"));
  writeToFile(shutdownMsg);

  if (log_file_.isOpen()) {
    log_stream_.flush();
    log_file_.flush();
    log_file_.close();
  }

  initialized_ = false;
  mutex_.unlock();  // Always unlock
}

Logger::~Logger() {
  shutdown();
}

void Logger::log(LogLevel level, const QString& message, const QString& file,
                 int line, const QString& function) {
  // Check if initialized first (without lock) to avoid unnecessary locking
  if (!initialized_) {
    // Initialize without holding lock to avoid deadlock
    initialize();
  }

  // Try to lock, but don't block forever - if mutex is locked, skip logging
  // This prevents deadlocks during shutdown
  if (!mutex_.tryLock(100)) {
    // If we can't lock quickly, just output to stderr
    qWarning() << "[Logger locked, outputting to stderr]" << message;
    return;
  }

  // Manual lock management - ensure unlock in all paths
  QString logEntry = formatLogEntry(level, message, file, line, function);
  writeToFile(logEntry);
  mutex_.unlock();  // Always unlock before returning

  // Also output to stderr for critical errors (after unlock)
  if (level == LogLevel::Critical || level == LogLevel::Error) {
    qCritical() << logEntry.trimmed();
  }
}

void Logger::writeToFile(const QString& logEntry) {
  if (log_file_.isOpen()) {
    log_stream_ << logEntry;
    log_stream_.flush();
    // Force OS-level flush to ensure data is written to disk immediately
    // This is critical for crash debugging
    log_file_.flush();
  } else {
    // Fallback to stderr
    qWarning() << logEntry.trimmed();
  }
}

QString Logger::levelToString(LogLevel level) const {
  switch (level) {
    case LogLevel::Debug:
      return "DEBUG";
    case LogLevel::Info:
      return "INFO ";
    case LogLevel::Warning:
      return "WARN ";
    case LogLevel::Error:
      return "ERROR";
    case LogLevel::Critical:
      return "CRIT ";
    default:
      return "UNKN ";
  }
}

QString Logger::formatLogEntry(LogLevel level, const QString& message,
                               const QString& file, int line,
                               const QString& function) const {
  QString timestamp =
    QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
  QString levelStr = levelToString(level);

  QString entry = QString("[%1] [%2] ").arg(timestamp, levelStr);

  if (!file.isEmpty() && line > 0) {
    QFileInfo fileInfo(file);
    entry += QString("%1:%2 ").arg(fileInfo.fileName()).arg(line);
  }

  if (!function.isEmpty()) {
    entry += QString("(%1) ").arg(function);
  }

  entry += message + "\n";

  return entry;
}
