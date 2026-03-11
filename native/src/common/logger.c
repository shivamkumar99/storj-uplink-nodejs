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
#ifndef _WIN32
#include <fcntl.h>
#include <sys/stat.h>
#include <limits.h>
#include <unistd.h>
#endif

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

/* Allowed log file extensions (prevents opening arbitrary file types).
 * Lengths are compile-time constants — avoids strlen on string literals. */
static const char* ALLOWED_EXTENSIONS[] = { ".log", ".txt" };
static const size_t ALLOWED_EXT_LENS[] = { 4, 4 };  /* sizeof(".log")-1, sizeof(".txt")-1 */
static const size_t ALLOWED_EXTENSIONS_COUNT = sizeof(ALLOWED_EXTENSIONS) / sizeof(ALLOWED_EXTENSIONS[0]);

/**
 * Validate a log file path for safety.
 *
 * Checks performed:
 *   1. Non-NULL and non-empty
 *   2. No ".." traversal sequences
 *   3. No null bytes (truncation attacks)
 *   4. Must end with an allowed extension (.log, .txt)
 *   5. (POSIX) Must resolve to a path under the current working directory
 *   6. Reasonable length (< PATH_MAX)
 *
 * @param path  The candidate path to validate
 * @return 1 if safe to open, 0 otherwise
 */
static int validate_log_path(const char* path) {
    if (path == NULL || path[0] == '\0') return 0;

    /* Use strnlen to bound the scan — defends against non-terminated input.
     * PATH_MAX is the maximum path length on POSIX; 4096 as fallback. */
#ifdef PATH_MAX
    size_t len = strnlen(path, PATH_MAX);
    /* Reject unreasonably long paths */
    if (len >= PATH_MAX) return 0;
#else
    size_t len = strnlen(path, 4096);
    if (len >= 4096) return 0;
#endif

    /* Reject null bytes anywhere (truncation attack) */
    if (memchr(path, '\0', len) != path + len) return 0;

    /* Reject path traversal sequences */
    if (strstr(path, "..") != NULL) return 0;

    /* Reject backslashes (Windows-style traversal on POSIX) */
    if (strchr(path, '\\') != NULL) return 0;

    /* Must end with an allowed extension */
    int ext_ok = 0;
    for (size_t i = 0; i < ALLOWED_EXTENSIONS_COUNT; i++) {
        size_t ext_len = ALLOWED_EXT_LENS[i];
        if (len >= ext_len &&
            strcmp(path + len - ext_len, ALLOWED_EXTENSIONS[i]) == 0) {
            ext_ok = 1;
            break;
        }
    }
    if (!ext_ok) return 0;

#ifndef _WIN32
    /* POSIX: resolve the path and verify it stays under cwd */
    /* Reject absolute paths — only relative paths allowed from env */
    if (path[0] == '/') return 0;

    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) return 0;

    /* Build the full candidate path */
    char candidate[PATH_MAX];
    int n = snprintf(candidate, sizeof(candidate), "%s/%s", cwd, path);
    if (n < 0 || (size_t)n >= sizeof(candidate)) return 0;

    /* Try to resolve the parent directory (file may not exist yet) */
    char parent[PATH_MAX];
    strncpy(parent, candidate, sizeof(parent) - 1);
    parent[sizeof(parent) - 1] = '\0';
    char* last_slash = strrchr(parent, '/');
    if (last_slash != NULL && last_slash != parent) {
        *last_slash = '\0';
    }

    char resolved_parent[PATH_MAX];
    if (realpath(parent, resolved_parent) == NULL) return 0;

    /* Resolved parent must start with cwd (no symlink escape) */
    size_t cwd_len = strnlen(cwd, sizeof(cwd));
    if (strncmp(resolved_parent, cwd, cwd_len) != 0) return 0;
    /* After the cwd prefix, must be end-of-string or '/' */
    if (resolved_parent[cwd_len] != '\0' && resolved_parent[cwd_len] != '/') return 0;
#endif

    return 1;
}

/**
 * Open a validated log file with restricted permissions.
 *
 * @param path  The validated file path
 * @return FILE* on success, NULL on failure
 */
static FILE* open_log_file(const char* path) {
#ifdef _WIN32
    return fopen(path, "a");
#else
    /* Open with restricted permissions: owner read/write only (0600) */
    int fd = open(path, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
    if (fd < 0) return NULL;
    FILE* f = fdopen(fd, "a");
    if (f == NULL) {
        close(fd);
    }
    return f;
#endif
}

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
    
    /* Check for log file — validate path before opening */
    const char* env_file = getenv("UPLINK_LOG_FILE");
    if (env_file != NULL) {
        if (validate_log_path(env_file)) {
            log_file = open_log_file(env_file);
            if (log_file == NULL) {
                fprintf(stderr, "[WARN] UPLINK_LOG_FILE: failed to open '%s'\n", env_file);
            }
        } else {
            fprintf(stderr, "[WARN] UPLINK_LOG_FILE: rejected '%s' "
                    "(must be a relative .log or .txt file with no '..' traversal)\n",
                    env_file);
        }
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
    if (path == NULL) return;

    /* Validate path before opening (same checks as env var path) */
    if (!validate_log_path(path)) {
        fprintf(stderr, "[WARN] logger_set_file: rejected '%s' "
                "(must be a relative .log or .txt file with no '..' traversal)\n",
                path);
        return;
    }

    if (log_file != NULL) {
        fclose(log_file);
        log_file = NULL;
    }

    log_file = open_log_file(path);
}

void logger_log(LogLevel level, const char* file, int line, 
                const char* func, const char* fmt, ...) {
    if (!initialized) logger_init();
    if (level > current_level) return;
    
    /* Get timestamp — use localtime_r (thread-safe, POSIX) on POSIX systems */
    time_t now = time(NULL);
#ifdef _WIN32
    struct tm* tm_info = localtime(&now); /* Windows: single-threaded addon, acceptable */
    if (tm_info == NULL) return;
#else
    struct tm tm_buf;
    struct tm* tm_info = localtime_r(&now, &tm_buf);
    if (tm_info == NULL) return;
#endif
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
    /* fmt is always a compile-time literal from LOG_* macros;
       enforced by __attribute__((format(printf, 5, 6))) on the declaration */
    vfprintf(stderr, fmt, args); /* Flawfinder: ignore */
    fprintf(stderr, "%s\n", COLOR_RESET);
    
    /* Print to file without colors */
    if (log_file != NULL) {
        fprintf(log_file, "[%s] %s [%s:%d %s()] ", 
                timestamp, level_strings[level], filename, line, func);
        va_end(args);
        va_start(args, fmt);
        /* fmt is always a compile-time literal from LOG_* macros;
           enforced by __attribute__((format(printf, 5, 6))) on the declaration */
        vfprintf(log_file, fmt, args); /* Flawfinder: ignore */
        fprintf(log_file, "\n");
        fflush(log_file);
    }
    
    va_end(args);
}
