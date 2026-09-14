/**
 * @file library_loader.c
 * @brief Implementation of dynamic library loader
 *
 * Provides cross-platform dynamic library loading for uplink-c.
 *
 * The addon and libuplink are shipped side by side in
 * native/prebuilds/<platform>/. The library is therefore located relative to
 * the addon file itself (never relative to the process working directory), so
 * loading works no matter where the consuming application was started from.
 * UPLINK_LIBRARY_PATH lets a deployment keep the library anywhere else.
 */

#include "library_loader.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
    #include <windows.h>
    #define GET_SYMBOL(handle, name) GetProcAddress((HMODULE)handle, name)
    #define CLOSE_LIBRARY(handle) FreeLibrary((HMODULE)handle)
    #define LIB_EXT ".dll"
    #define PATH_SEP '\\'
#else
    #include <dlfcn.h>
    #define GET_SYMBOL(handle, name) dlsym(handle, name)
    #define CLOSE_LIBRARY(handle) dlclose(handle)
    #define PATH_SEP '/'
    #ifdef __APPLE__
        #define LIB_EXT ".dylib"
    #else
        #define LIB_EXT ".so"
    #endif
#endif

#define LIB_NAME "libuplink" LIB_EXT
#define PATH_MAX_LEN 1024

void* uplink_lib_handle = NULL;
static char loaded_path[PATH_MAX_LEN] = {0};

/**
 * Get the platform-specific directory name.
 */
static const char* get_platform_dir(void) {
#ifdef _WIN32
    #ifdef _M_X64
        return "win32-x64";
    #else
        return "win32-ia32";
    #endif
#elif defined(__APPLE__)
    #ifdef __aarch64__
        return "darwin-arm64";
    #else
        return "darwin-x64";
    #endif
#else
    #ifdef __aarch64__
        return "linux-arm64";
    #else
        return "linux-x64";
    #endif
#endif
}

#ifdef _WIN32
/**
 * Does the path contain a directory separator (i.e. is it more than a bare
 * file name)?
 */
static int has_dir_component(const char* path) {
    return strchr(path, '/') != NULL || strchr(path, '\\') != NULL;
}
#endif

/**
 * Open a shared library. Full paths are loaded exactly as given; on Windows
 * LOAD_WITH_ALTERED_SEARCH_PATH makes any dependent DLLs resolve next to the
 * library instead of next to node.exe.
 */
static void* open_library(const char* path) {
#ifdef _WIN32
    if (has_dir_component(path)) {
        return (void*)LoadLibraryExA(path, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    }
    return (void*)LoadLibraryA(path);
#else
    return dlopen(path, RTLD_NOW | RTLD_LOCAL);
#endif
}

/**
 * Attempt to load the library from a specific path.
 */
static int try_load_library(const char* path) {
    if (path == NULL || path[0] == '\0') {
        return -1;
    }
    LOG_DEBUG("Attempting to load library from: %s", path);

    uplink_lib_handle = open_library(path);

    if (uplink_lib_handle != NULL) {
        snprintf(loaded_path, sizeof(loaded_path), "%s", path);
        LOG_INFO("Successfully loaded library from: %s", path);
        return 0;
    }

#ifndef _WIN32
    LOG_DEBUG("Failed to load: %s", dlerror());
#else
    LOG_DEBUG("Failed to load: error code %lu", GetLastError());
#endif

    return -1;
}

/**
 * Directory that contains this addon (uplink_native.node), without a trailing
 * separator. Returns -1 when the platform cannot tell us.
 */
static int get_addon_dir(char* out, size_t out_size) {
#ifdef _WIN32
    HMODULE self = NULL;
    if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            (LPCSTR)&load_uplink_library, &self)) {
        return -1;
    }
    DWORD len = GetModuleFileNameA(self, out, (DWORD)out_size);
    if (len == 0 || len >= out_size) {
        return -1;
    }
#else
    Dl_info info;
    if (dladdr((void*)&load_uplink_library, &info) == 0 || info.dli_fname == NULL) {
        return -1;
    }
    if (snprintf(out, out_size, "%s", info.dli_fname) >= (int)out_size) {
        return -1;
    }
#endif
    char* slash = strrchr(out, '/');
    char* backslash = strrchr(out, '\\');
    if (backslash != NULL && (slash == NULL || backslash > slash)) {
        slash = backslash;
    }
    if (slash == NULL) {
        return -1;
    }
    *slash = '\0';
    return 0;
}

/**
 * Is the path an existing regular file (not a directory)?
 */
static int is_regular_file(const char* path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        return 0;
    }
    return (st.st_mode & S_IFMT) == S_IFREG;
}

static int is_directory(const char* path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        return 0;
    }
    return (st.st_mode & S_IFMT) == S_IFDIR;
}

/**
 * Build "<dir>/<rest>" into out. Returns -1 if it does not fit.
 */
static int join_path(char* out, size_t out_size, const char* dir, const char* rest) {
    int n = snprintf(out, out_size, "%s%c%s", dir, PATH_SEP, rest);
    return (n < 0 || (size_t)n >= out_size) ? -1 : 0;
}

/**
 * Try to load the library from a directory, i.e. "<dir>/libuplink.<ext>".
 */
static int try_load_from_dir(const char* dir) {
    char path[PATH_MAX_LEN];
    if (join_path(path, sizeof(path), dir, LIB_NAME) != 0) {
        return -1;
    }
    if (!is_regular_file(path)) {
        LOG_DEBUG("No %s in %s", LIB_NAME, dir);
        return -1;
    }
    return try_load_library(path);
}

/**
 * Resolve UPLINK_LIBRARY_PATH to an absolute, existing library file. The value
 * may name either the library file itself or a directory containing it.
 * Returns 0 and fills resolved on success.
 */
static int resolve_env_library(const char* env_path, char* resolved, size_t resolved_size) {
    /* Reject paths containing ".." to prevent directory traversal */
    if (strstr(env_path, "..") != NULL) {
        LOG_ERROR("UPLINK_LIBRARY_PATH rejected: contains '..' traversal sequence");
        return -1;
    }
    if (strlen(env_path) >= resolved_size) {
        LOG_ERROR("UPLINK_LIBRARY_PATH rejected: path too long");
        return -1;
    }

    /*
     * Resolve to an absolute canonical path before passing to dlopen/LoadLibrary.
     * This eliminates symlink-based traversal and ensures we load exactly
     * what the user intended. CodeQL: cpp/uncontrolled-process-operation
     */
    char absolute[PATH_MAX_LEN];
#ifdef _WIN32
    DWORD len = GetFullPathNameA(env_path, sizeof(absolute), absolute, NULL);
    if (len == 0 || len >= sizeof(absolute)) {
        LOG_ERROR("UPLINK_LIBRARY_PATH rejected: cannot resolve path");
        return -1;
    }
#else
    if (realpath(env_path, absolute) == NULL) {
        LOG_ERROR("UPLINK_LIBRARY_PATH rejected: cannot resolve path (%s)", env_path);
        return -1;
    }
#endif

    /* A directory means "<dir>/libuplink.<ext>" */
    if (is_directory(absolute)) {
        if (join_path(resolved, resolved_size, absolute, LIB_NAME) != 0) {
            LOG_ERROR("UPLINK_LIBRARY_PATH rejected: path too long");
            return -1;
        }
    } else if (snprintf(resolved, resolved_size, "%s", absolute) >= (int)resolved_size) {
        LOG_ERROR("UPLINK_LIBRARY_PATH rejected: path too long");
        return -1;
    }

    /* Verify the resolved path ends with the expected library extension */
    size_t rlen = strlen(resolved);
    size_t elen = strlen(LIB_EXT);
    if (rlen < elen || strcmp(resolved + rlen - elen, LIB_EXT) != 0) {
        LOG_ERROR("UPLINK_LIBRARY_PATH rejected: does not end with '%s'", LIB_EXT);
        return -1;
    }
    if (!is_regular_file(resolved)) {
        LOG_ERROR("UPLINK_LIBRARY_PATH rejected: %s does not exist or is a directory", resolved);
        return -1;
    }
    return 0;
}

int load_uplink_library(void) {
    if (uplink_lib_handle != NULL) {
        LOG_DEBUG("Library already loaded");
        return 0;
    }

    char path[PATH_MAX_LEN];
    const char* platform_dir = get_platform_dir();

    /* Try 1: UPLINK_LIBRARY_PATH — file or directory, validated before use */
    const char* env_path = getenv("UPLINK_LIBRARY_PATH");
    if (env_path != NULL && env_path[0] != '\0') {
        if (resolve_env_library(env_path, path, sizeof(path)) == 0 &&
            try_load_library(path) == 0) {
            return 0;
        }
        LOG_WARN("UPLINK_LIBRARY_PATH did not yield a loadable library; "
                 "falling back to the bundled search paths");
    }

    /* Try 2 & 3: relative to the addon file, independent of the working directory */
    char addon_dir[PATH_MAX_LEN];
    if (get_addon_dir(addon_dir, sizeof(addon_dir)) == 0) {
        /* 2: next to the addon (native/prebuilds/<platform>/) */
        if (try_load_from_dir(addon_dir) == 0) return 0;

        /* 3: addon in build/Release/, library in native/prebuilds/<platform>/ */
        char rel[PATH_MAX_LEN];
        if (snprintf(rel, sizeof(rel), "..%c..%cnative%cprebuilds%c%s",
                     PATH_SEP, PATH_SEP, PATH_SEP, PATH_SEP, platform_dir) < (int)sizeof(rel) &&
            join_path(path, sizeof(path), addon_dir, rel) == 0 &&
            try_load_from_dir(path) == 0) {
            return 0;
        }
    } else {
        LOG_DEBUG("Could not determine the addon's own directory");
    }

    /* Try 4 & 5: relative to the working directory (legacy layouts) */
    snprintf(path, sizeof(path), "native/prebuilds/%s/%s", platform_dir, LIB_NAME);
    if (try_load_library(path) == 0) return 0;

    snprintf(path, sizeof(path), "prebuilds/%s/%s", platform_dir, LIB_NAME);
    if (try_load_library(path) == 0) return 0;

    /* Try 6: system library search path */
    if (try_load_library(LIB_NAME) == 0) return 0;

    LOG_ERROR("Failed to load %s from any location (set UPLINK_LIBRARY_PATH to its file or directory)",
              LIB_NAME);
    return -1;
}

void unload_uplink_library(void) {
    if (uplink_lib_handle != NULL) {
        CLOSE_LIBRARY(uplink_lib_handle);
        uplink_lib_handle = NULL;
        loaded_path[0] = '\0';
        LOG_INFO("Unloaded uplink library");
    }
}

void* get_uplink_function(const char* name) {
    if (uplink_lib_handle == NULL) {
        LOG_ERROR("Library not loaded, cannot get function: %s", name);
        return NULL;
    }

    void* fn = GET_SYMBOL(uplink_lib_handle, name);
    if (fn == NULL) {
        LOG_ERROR("Function not found: %s", name);
    }
    return fn;
}

int is_library_loaded(void) {
    return uplink_lib_handle != NULL;
}

const char* get_loaded_library_path(void) {
    return loaded_path;
}
