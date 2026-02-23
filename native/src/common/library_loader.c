/**
 * @file library_loader.c
 * @brief Implementation of dynamic library loader
 * 
 * Provides cross-platform dynamic library loading for uplink-c.
 */

#include "library_loader.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #include <windows.h>
    #define LOAD_LIBRARY(path) LoadLibraryA(path)
    #define GET_SYMBOL(handle, name) GetProcAddress((HMODULE)handle, name)
    #define CLOSE_LIBRARY(handle) FreeLibrary((HMODULE)handle)
    #define LIB_EXT ".dll"
#else
    #include <dlfcn.h>
    #define LOAD_LIBRARY(path) dlopen(path, RTLD_NOW | RTLD_LOCAL)
    #define GET_SYMBOL(handle, name) dlsym(handle, name)
    #define CLOSE_LIBRARY(handle) dlclose(handle)
    #ifdef __APPLE__
        #define LIB_EXT ".dylib"
    #else
        #define LIB_EXT ".so"
    #endif
#endif

void* uplink_lib_handle = NULL;
static char loaded_path[1024] = {0};

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

/**
 * Attempt to load the library from a specific path.
 */
static int try_load_library(const char* path) {
    LOG_DEBUG("Attempting to load library from: %s", path);
    
    if (path == NULL) {
        LOG_ERROR("Cannot load library: path is NULL");
        return -1;
    }

    uplink_lib_handle = LOAD_LIBRARY(path);
    
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

int load_uplink_library(void) {
    if (uplink_lib_handle != NULL) {
        LOG_DEBUG("Library already loaded");
        return 0;
    }
    
    char path[1024];
    const char* platform_dir = get_platform_dir();
    const char* lib_name = "libuplink" LIB_EXT;
    
    /* Try 1: Environment variable path */
    const char* env_path = getenv("UPLINK_LIBRARY_PATH");
    if (env_path != NULL) {
        if (try_load_library(env_path) == 0) return 0;
    }
    
    /* Try 2: native/prebuilds/<platform>/ */
    snprintf(path, sizeof(path), "native/prebuilds/%s/%s", platform_dir, lib_name);
    if (try_load_library(path) == 0) return 0;
    
    /* Try 3: ./prebuilds/<platform>/ */
    snprintf(path, sizeof(path), "prebuilds/%s/%s", platform_dir, lib_name);
    if (try_load_library(path) == 0) return 0;
    
    /* Try 4: System library path */
    if (try_load_library(lib_name) == 0) return 0;
    
    LOG_ERROR("Failed to load uplink library from any location");
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
