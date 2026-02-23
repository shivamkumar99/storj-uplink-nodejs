/**
 * @file logger.h
 * @brief Logging utilities for uplink-nodejs native module
 * 
 * Provides structured logging with different severity levels.
 * Supports console and file output with timestamps and colors.
 */

#ifndef UPLINK_LOGGER_H
#define UPLINK_LOGGER_H

#include <stdio.h>

/**
 * Log severity levels
 */
typedef enum {
    LOG_LEVEL_NONE  = 0,  /**< No logging */
    LOG_LEVEL_ERROR = 1,  /**< Error messages only */
    LOG_LEVEL_WARN  = 2,  /**< Warnings and errors */
    LOG_LEVEL_INFO  = 3,  /**< Informational, warnings, and errors */
    LOG_LEVEL_DEBUG = 4,  /**< Debug, info, warnings, and errors */
    LOG_LEVEL_TRACE = 5   /**< All messages including trace */
} LogLevel;

/**
 * Initialize the logger.
 * Reads UPLINK_LOG_LEVEL and UPLINK_LOG_FILE from environment.
 */
void logger_init(void);

/**
 * Shutdown the logger and close any open file handles.
 */
void logger_shutdown(void);

/**
 * Set the current log level.
 * @param level The log level to set
 */
void logger_set_level(LogLevel level);

/**
 * Set a file path for log output.
 * @param path Path to the log file
 */
void logger_set_file(const char* path);

/**
 * Core logging function.
 * @param level Log level for this message
 * @param file Source file name
 * @param line Source line number
 * @param func Function name
 * @param fmt Format string
 * @param ... Format arguments
 */
void logger_log(LogLevel level, const char* file, int line, 
                const char* func, const char* fmt, ...);

/* Convenience macros for logging at different levels */
#define LOG_ERROR(fmt, ...) \
    logger_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

#define LOG_WARN(fmt, ...) \
    logger_log(LOG_LEVEL_WARN, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

#define LOG_INFO(fmt, ...) \
    logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

#define LOG_DEBUG(fmt, ...) \
    logger_log(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

#define LOG_TRACE(fmt, ...) \
    logger_log(LOG_LEVEL_TRACE, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

#endif /* UPLINK_LOGGER_H */
