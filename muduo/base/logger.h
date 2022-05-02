
#ifndef MACOS_MUDUO_BASE_LOGGER_H
#define MACOS_MUDUO_BASE_LOGGER_H

#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"

#define LOG_TRACE(...) spdlog::trace(__VA_ARGS__)
#define LOG_INFO(...) spdlog::info(__VA_ARGS__)
#define LOG_WARN(...) spdlog::warn(__VA_ARGS__)
#define LOG_ERROR(...) spdlog::error(__VA_ARGS__)
#define LOG_FATAL(...) spdlog::critical(__VA_ARGS__)



#endif  // MACOS_MUDUO_BASE_LOGGER_H
