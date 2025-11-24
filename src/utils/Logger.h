#pragma once

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <QDateTime>

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error,
    Critical
};

class Logger {
public:
    static Logger& instance();

    void initialize(const QString& logFilePath = QString());
    void shutdown();

    void log(LogLevel level, const QString& message,
             const QString& file = QString(), int line = 0,
             const QString& function = QString());

    QString logFilePath() const { return log_file_path_; }

private:
    Logger() = default;
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void writeToFile(const QString& logEntry);
    QString levelToString(LogLevel level) const;
    QString formatLogEntry(LogLevel level, const QString& message,
                          const QString& file, int line, const QString& function) const;

    QFile log_file_;
    QTextStream log_stream_;
    QMutex mutex_;
    QString log_file_path_;
    bool initialized_ = false;
};

// Convenience macros
#define LOG_DEBUG(msg) Logger::instance().log(LogLevel::Debug, msg, __FILE__, __LINE__, __FUNCTION__)
#define LOG_INFO(msg) Logger::instance().log(LogLevel::Info, msg, __FILE__, __LINE__, __FUNCTION__)
#define LOG_WARNING(msg) Logger::instance().log(LogLevel::Warning, msg, __FILE__, __LINE__, __FUNCTION__)
#define LOG_ERROR(msg) Logger::instance().log(LogLevel::Error, msg, __FILE__, __LINE__, __FUNCTION__)
#define LOG_CRITICAL(msg) Logger::instance().log(LogLevel::Critical, msg, __FILE__, __LINE__, __FUNCTION__)

