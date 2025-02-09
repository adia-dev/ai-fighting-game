#pragma once

#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <deque>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>

class Logger {

  enum class LogLevel {
    Trace = 0,
    Debug,
    Info,
    Warn,
    Error,
    Fatal,
  };

  struct LoggerConfig {
    LogLevel level = LogLevel::Debug;
    // std::string log_file = std::string(LOG_DIR) + "/app.log";
    bool enable_colors = true;
  };

public:
  static void init() {
    auto &logger = get();
    std::lock_guard<std::mutex> lock(logger.m_mutex);
  }

  static const std::deque<std::pair<LogLevel, std::string>> &messages() {
    return get().m_messages;
  }
  static void clear_messages() { get().m_messages.clear(); }

  template <typename... Args>
  static void trace(const std::string &format_str, Args &&...args) {
    get().log(LogLevel::Trace, format_str, std::forward<Args>(args)...);
  }

  template <typename... Args>
  static void debug(const std::string &format_str, Args &&...args) {
    get().log(LogLevel::Debug, format_str, std::forward<Args>(args)...);
  }

  template <typename... Args>
  static void info(const std::string &format_str, Args &&...args) {
    get().log(LogLevel::Info, format_str, std::forward<Args>(args)...);
  }

  template <typename... Args>
  static void warn(const std::string &format_str, Args &&...args) {
    get().log(LogLevel::Warn, format_str, std::forward<Args>(args)...);
  }

  template <typename... Args>
  static void error(const std::string &format_str, Args &&...args) {
    get().log(LogLevel::Error, format_str, std::forward<Args>(args)...);
  }

  template <typename... Args>
  static void fatal(const std::string &format_str, Args &&...args) {
    std::string message =
        get().format_exception_message(format_str, std::forward<Args>(args)...);
    get().log(LogLevel::Fatal, message);

    throw std::runtime_error(message);
  }

  template <typename... Args>
  static void fatal(const std::exception &ex, const std::string &format_str,
                    Args &&...args) {
    std::string message =
        get().format_exception_message(format_str, std::forward<Args>(args)...);
    get().log(LogLevel::Fatal, message + ": " + ex.what());

    throw std::runtime_error(message);
  }

private:
  static Logger &get() {
    static Logger instance;
    return instance;
  }

  template <typename... Args>
  void log(LogLevel level, const std::string &format_str, Args &&...args) {
    if (level < m_config.level)
      return;

    std::string message;
    if constexpr (sizeof...(args) > 0) {
      constexpr size_t BUFFER_SIZE = 1024;
      char buffer[BUFFER_SIZE];
      int ret = std::snprintf(buffer, BUFFER_SIZE, format_str.c_str(), args...);
      message = (ret >= 0 && static_cast<size_t>(ret) < BUFFER_SIZE)
                    ? buffer
                    : format_str;
    } else {
      message = format_str;
    }

    std::string formatted_message = format_message(level, message);
    std::lock_guard<std::mutex> lock(m_mutex);

    m_messages.push_back({level, formatted_message});

    const size_t MAX_LOG_SIZE = 1000;
    if (m_messages.size() > MAX_LOG_SIZE) {
      m_messages.pop_front();
    }

    if (m_config.enable_colors) {
      const char *color_code = level_color_code(level);
      std::cout << color_code << formatted_message << "\033[0m" << std::endl;
    } else {
      std::cout << formatted_message << std::endl;
    }
  }

  std::string format_message(LogLevel level, const std::string &message) {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    char time_buffer[20];
#if defined(_MSC_VER)
    std::tm tm_now;
    localtime_s(&tm_now, &now_c);
    std::strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S",
                  &tm_now);
#else
    std::tm tm_now;
    localtime_r(&now_c, &tm_now);
    std::strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S",
                  &tm_now);
#endif
    std::ostringstream oss;
    oss << "[" << time_buffer << "][" << log_level_to_string(level) << "] "
        << message;
    return oss.str();
  }

  const char *log_level_to_string(LogLevel level) {
    switch (level) {
    case LogLevel::Trace:
      return "TRACE";
    case LogLevel::Debug:
      return "DEBUG";
    case LogLevel::Info:
      return "INFO";
    case LogLevel::Warn:
      return "WARN";
    case LogLevel::Error:
      return "ERROR";
    case LogLevel::Fatal:
      return "FATAL";
    default:
      return "UNKNOWN";
    }
  }

  const char *level_color_code(LogLevel level) {
    switch (level) {
    case LogLevel::Trace:
      return "\033[37m"; // White
    case LogLevel::Debug:
      return "\033[36m"; // Cyan
    case LogLevel::Info:
      return "\033[32m"; // Green
    case LogLevel::Warn:
      return "\033[33m"; // Yellow
    case LogLevel::Error:
      return "\033[31m"; // Red
    case LogLevel::Fatal:
      return "\033[1;31m"; // Bold Red
    default:
      return "";
    }
  }

  template <typename... Args>
  std::string format_exception_message(const std::string &format_str,
                                       Args &&...args) {
    std::string message;
    if constexpr (sizeof...(args) > 0) {
      constexpr size_t BUFFER_SIZE = 1024;
      char buffer[BUFFER_SIZE];
      int ret = std::snprintf(buffer, BUFFER_SIZE, format_str.c_str(), args...);
      message = (ret >= 0 && static_cast<size_t>(ret) < BUFFER_SIZE)
                    ? buffer
                    : format_str;
    } else {
      message = format_str;
    }
    return message;
  }

private:
  Logger() = default;
  ~Logger() {}

  Logger(const Logger &) = delete;
  Logger(Logger &&) = delete;
  Logger &operator=(const Logger &) = delete;
  Logger &operator=(Logger &&) = delete;

  LoggerConfig m_config;
  std::mutex m_mutex;
  std::deque<std::pair<LogLevel, std::string>> m_messages;
};
