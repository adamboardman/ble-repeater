#pragma once

#include "CircularBuffer.h"

#define DEBUG_BUILD

extern CircularBuffer<char> serialLogBuffer;
void printAvailableLogging();

#if defined(DEBUG_BUILD)
#if (PICO_RP2040 || PICO_RP2350)
#define LOG_DEBUG(...) serialLogBuffer.writeF(__VA_ARGS__);
#define LOG_INFO(...) serialLogBuffer.writeF(__VA_ARGS__);
#define LOG_WARN(...) serialLogBuffer.writeF(__VA_ARGS__);
#define LOG_ERROR(...) serialLogBuffer.writeF(__VA_ARGS__);
#define LOG_CRIT(...) serialLogBuffer.writeF(__VA_ARGS__);
#define LOG_TRACE(...) serialLogBuffer.writeF(__VA_ARGS__);
#else
#define LOG_DEBUG(...) printf(__VA_ARGS__)
#define LOG_INFO(...) printf(__VA_ARGS__)
#define LOG_WARN(...) printf(__VA_ARGS__)
#define LOG_ERROR(...) printf(__VA_ARGS__)
#define LOG_CRIT(...) printf(__VA_ARGS__)
#define LOG_TRACE(...) printf(__VA_ARGS__)
#endif
#else
#define LOG_DEBUG(...)
#define LOG_INFO(...)
#define LOG_WARN(...)
#define LOG_ERROR(...)
#define LOG_CRIT(...)
#define LOG_TRACE(...)
#endif
