#pragma once

#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>
#include "pubsuber/Pubsuber.h"

namespace pubsuber::logger {

  inline void ChangeLevel(spdlog::level::level_enum level) { spdlog::get("pubsuber")->set_level(level); }

  inline void Setup(std::shared_ptr<pubsuber::LogSink> sink, spdlog::level::level_enum level) {
    if (!sink) {
      sink = std::make_shared<spdlog::sinks::stdout_sink_mt>();
      sink->set_pattern("%D-%T.%f [%t] [%L]: %v");
      sink->set_level(spdlog::level::trace);
    }

    auto logger = std::make_shared<spdlog::logger>("pubsuber", sink);
    spdlog::register_logger(logger);
    ChangeLevel(level);
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
