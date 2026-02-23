/**
 * @file logger.c
 * @brief Implementation of logging utilities
 * 
 * Provides thread-safe logging with timestamps, colors for console,
 * and optional file output.
 */

#include "logger.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static LogLevel current_level = LOG_LEVEL_INFO;
static FILE* log_file = NULL;
static int initialized = 0;

static const char* level_strings[] = {
    "NONE", "ERROR", "WARN", "INFO", "DEBUG", "TRACE"
};

static const char* level_colors[] = {
    "",           /* NONE */
    "\x1b[31m",   /* ERROR - Red */
    "\x1b[33m",   /* WARN - Yellow */
    "\x1b[32m",   /* INFO - Green */
    "\x1b[36m",   /* DEBUG - Cyan */
    "\x1b[90m"    /* TRACE - Gray */
};

#define COLOR_RESET "\x1b[0m"

/* Log level name → value lookup table */
typedef struct {
    const char* name;
    LogLevel level;
} LogLevelEntry;

static const LogLevelEntry LOG_LEVEL_MAP[] = {
    { "error", LOG_LEVEL_ERROR },
    { "warn",  LOG_LEVEL_WARN },
    { "info",  LOG_LEVEL_INFO },
    { "debug", LOG_LEVEL_DEBUG },
    { "trace", LOG_LEVEL_TRACE },
    { "none",  LOG_LEVEL_NONE },
};

static const size_t LOG_LEVEL_MAP_SIZE = sizeof(LOG_LEVEL_MAP) / sizeof(LOG_LEVEL_MAP[0]);

void logger_init(void) {
    if (initialized) return;
    
    /* Check environment variable for log level */
    const char* env_level = getenv("UPLINK_LOG_LEVEL");
    if (env_level != NULL) {
        for (size_t i = 0; i < LOG_LEVEL_MAP_SIZE; i++) {
            if (strcmp(env_level, LOG_LEVEL_MAP[i].name) == 0) {
                current_level = LOG_LEVEL_MAP[i].level;
                break;
            }
        }
    }
    
    /* Check for log file */
    const char* env_file = getenv("UPLINK_LOG_FILE");
    if (env_file != NULL) {
        log_file = fopen(env_file, "a");
    }
    
    initialized = 1;
}

void logger_shutdown(void) {
    if (log_file != NULL) {
        fclose(log_file);
        log_file = NULL;
    }
    initialized = 0;
}

void logger_set_level(LogLevel level) {
    current_level = level;
}

void logger_set_file(const char* path) {
    if (log_file != NULL) {
        fclose(log_file);
    }
    log_file = fopen(path, "a");
}

void logger_log(LogLevel level, const char* file, int line, 
                const char* func, const char* fmt, ...) {
    if (!initialized) logger_init();
    if (level > current_level) return;
    
    /* Get timestamp */
    time_t now = time(NULL);
    const struct tm* tm_info = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    
    /* Extract filename from path */
    const char* filename = strrchr(file, '/');
    filename = filename ? filename + 1 : file;
    
    /* Format message */
    va_list args;
    va_start(args, fmt);
    
    /* Print to stderr with colors */
    fprintf(stderr, "%s[%s] %s [%s:%d %s()] ", 
            level_colors[level], timestamp, level_strings[level],
            filename, line, func);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "%s\n", COLOR_RESET);
    
    /* Print to file without colors */
    if (log_file != NULL) {
        fprintf(log_file, "[%s] %s [%s:%d %s()] ", 
                timestamp, level_strings[level], filename, line, func);
        va_end(args);
        va_start(args, fmt);
        vfprintf(log_file, fmt, args);
        fprintf(log_file, "\n");
        fflush(log_file);
    }
    
    va_end(args);
}
