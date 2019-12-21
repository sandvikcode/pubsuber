#pragma once

#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>

namespace pubsuber::logger {

  inline void Setup() {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_sink_mt>();
    console_sink->set_pattern("%D-%T.%f [%t] [%L]: %v");
    console_sink->set_level(spdlog::level::trace);

    auto logger = std::make_shared<spdlog::logger>("pubsuber", console_sink);
    logger->set_level(spdlog::level::debug);
    spdlog::register_logger(logger);
  }

  template <typename... Args>
  inline void trace(const char *format, Args &&... args) {
    SPDLOG_LOGGER_CALL(spdlog::get("pubsuber"), spdlog::level::trace, format, args...);
  }

  template <typename... Args>
  inline void debug(const char *format, Args &&... args) {
    SPDLOG_LOGGER_CALL(spdlog::get("pubsuber"), spdlog::level::debug, format, args...);
  }

  template <typename... Args>
  inline void info(const char *format, Args &&... args) {
    SPDLOG_LOGGER_CALL(spdlog::get("pubsuber"), spdlog::level::info, format, args...);
  }

  template <typename... Args>
  inline void error(const char *format, Args &&... args) {
    SPDLOG_LOGGER_CALL(spdlog::get("pubsuber"), spdlog::level::err, format, args...);
  }

}  // namespace pubsuber::logger
