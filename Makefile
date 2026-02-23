# Makefile for uplink-nodejs
# Works on: Linux, macOS, Windows (CMD, PowerShell, MinGW/MSYS2)
#
# Installation (one command):
#   npm install @shivam995364/uplink-nodejs-test                           # auto-detect
#   UPLINK_INSTALL=prebuilt npm install @shivam995364/uplink-nodejs-test   # prebuilt only
#   UPLINK_INSTALL=hybrid  npm install @shivam995364/uplink-nodejs-test    # download lib + compile
#   UPLINK_INSTALL=source  npm install @shivam995364/uplink-nodejs-test    # full source build
#   UPLINK_INSTALL=skip    npm install @shivam995364/uplink-nodejs-test    # skip native build
#
# Direct Make Usage:
#   make install              - Auto-detect (prebuilt → hybrid → source)
#   make install-source       - Build everything from source (requires Go)
#   make install-hybrid       - Download lib, build addon (requires C compiler)
#   make install-prebuilt     - Download everything (Node.js only)
#
# Build Commands:
#   make build                - Build TypeScript + Native addon
#   make build-uplink         - Build uplink-c library from source
#   make build-native         - Build Node.js native addon
#
# Other Commands:
#   make help                 - Show this help
#   make verify               - Verify installation
#   make clean                - Clean build artifacts
#   make test                 - Run all tests

# Detect OS
# On Windows GitHub Actions (MSYS2/Git Bash), OS=Windows_NT but shell is bash.
# Detect MSYS/MinGW environment and use POSIX commands in that case.
# WINDOWS_NATIVE is set only when using cmd.exe (not MSYS2/Git Bash).
ifeq ($(OS),Windows_NT)
    DETECTED_OS := windows
    ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
        DETECTED_ARCH := x64
    else ifeq ($(PROCESSOR_ARCHITECTURE),x86)
        DETECTED_ARCH := x86
    else
        DETECTED_ARCH := $(PROCESSOR_ARCHITECTURE)
    endif
    LIB_EXT = .dll
    LIB_PREFIX = lib
    LIB_NAME = libuplink.dll
    PLATFORM = win32-x64
    # Check if running under MSYS2/Git Bash (GitHub Actions Windows runners)
    ifneq ($(MSYSTEM),)
        USE_POSIX_CMDS := 1
    else ifneq ($(shell echo $$BASH_VERSION 2>/dev/null),)
        USE_POSIX_CMDS := 1
    endif
    ifdef USE_POSIX_CMDS
        # MSYS2/Git Bash: use POSIX commands
        MKDIR = mkdir -p $1
        RM = rm -f $1
        RMDIR = rm -rf $1
        CP = cp -f $1 $2
        SEP = /
    else
        # Native Windows (cmd.exe): use Windows commands
        WINDOWS_NATIVE := 1
        MKDIR = if not exist "$(subst /,\,$1)" mkdir "$(subst /,\,$1)"
        RM = if exist "$(subst /,\,$1)" del /q "$(subst /,\,$1)"
        RMDIR = if exist "$(subst /,\,$1)" rmdir /s /q "$(subst /,\,$1)"
        CP = copy /y "$(subst /,\,$1)" "$(subst /,\,$2)"
        SEP = \\
    endif
else
    UNAME_S := $(shell uname -s)
    UNAME_M := $(shell uname -m)
    ifeq ($(UNAME_S),Darwin)
        DETECTED_OS := darwin
        LIB_EXT = .dylib
        LIB_PREFIX = lib
        LIB_NAME = libuplink.dylib
    else
        DETECTED_OS := linux
        LIB_EXT = .so
        LIB_PREFIX = lib
        LIB_NAME = libuplink.so
    endif
    ifeq ($(UNAME_M),x86_64)
        DETECTED_ARCH := x64
    else ifeq ($(UNAME_M),aarch64)
        DETECTED_ARCH := arm64
    else ifeq ($(UNAME_M),arm64)
        DETECTED_ARCH := arm64
    else
        DETECTED_ARCH := $(UNAME_M)
    endif
    # Unix commands
    MKDIR = mkdir -p $1
    RM = rm -f $1
    RMDIR = rm -rf $1
    CP = cp -f $1 $2
    SEP = /
    WHICH = command -v
    NULL_REDIR = /dev/null
    FILE_EXISTS = test -f
    DIR_EXISTS = test -d
    PLATFORM = $(DETECTED_OS)-$(DETECTED_ARCH)
endif

# Directories
PROJECT_DIR := $(CURDIR)
UPLINK_C_DIR := $(PROJECT_DIR)/../uplink-c
PREBUILDS_DIR := $(PROJECT_DIR)/native/prebuilds
INCLUDE_DIR := $(PROJECT_DIR)/native/include
PLATFORM_DIR := $(PREBUILDS_DIR)/$(PLATFORM)
BUILD_DIR := $(PROJECT_DIR)/build
DIST_DIR := $(PROJECT_DIR)/dist

# When PLATFORM is overridden (e.g., PLATFORM=darwin-x64 for cross-compilation),
# derive the target arch from PLATFORM so Go and node-gyp build for the right arch.
PLATFORM_ARCH := $(lastword $(subst -, ,$(PLATFORM)))
ifneq ($(PLATFORM_ARCH),$(DETECTED_ARCH))
    EFFECTIVE_ARCH := $(PLATFORM_ARCH)
else
    EFFECTIVE_ARCH := $(DETECTED_ARCH)
endif

# Go settings for building uplink-c
GO_OS_darwin := darwin
GO_OS_linux := linux
GO_OS_windows := windows
GO_OS := $(GO_OS_$(DETECTED_OS))

GO_ARCH_x64 := amd64
GO_ARCH_arm64 := arm64
GO_ARCH := $(GO_ARCH_$(EFFECTIVE_ARCH))

# ─── Persistent install method (.uplinkrc) ───────────────────────────────────
# If UPLINK_INSTALL is not set, check for a saved method in .uplinkrc.
# The .uplinkrc file lives in the consumer's project root (INIT_CWD), NOT inside
# node_modules, so it survives `rm -rf node_modules && npm install`.
#
# Priority: UPLINK_INSTALL env var  >  .uplinkrc file  >  auto-detect
#
# INIT_CWD is set by npm/yarn/pnpm to the directory where the user ran npm install.
# When running make directly (not via npm), INIT_CWD won't be set, so we skip.
UPLINKRC_DIR := $(or $(INIT_CWD),$(CURDIR))
UPLINKRC_FILE := $(UPLINKRC_DIR)/.uplinkrc

ifndef UPLINK_INSTALL
  # Try to read saved method from .uplinkrc
  _SAVED_METHOD := $(shell cat "$(UPLINKRC_FILE)" 2>/dev/null | tr -d '[:space:]')
  ifneq ($(_SAVED_METHOD),)
    # Validate: only accept known values
    ifneq ($(filter $(_SAVED_METHOD),prebuilt hybrid source skip),)
      UPLINK_INSTALL := $(_SAVED_METHOD)
      _UPLINK_FROM_RC := 1
      $(info [uplink-nodejs] .uplinkrc found at $(UPLINKRC_FILE) → method="$(_SAVED_METHOD)")
    else
      $(info [uplink-nodejs] .uplinkrc found but contains invalid value "$(_SAVED_METHOD)" — ignoring)
    endif
  else
    $(info [uplink-nodejs] No .uplinkrc found at $(UPLINKRC_FILE) — will auto-detect)
  endif
else
  $(info [uplink-nodejs] UPLINK_INSTALL="$(UPLINK_INSTALL)" set via env — .uplinkrc skipped)
endif

# Default target
.PHONY: all
all: prebuild build

# Info (use 'make help' for full command list)
.PHONY: info
info:
	@echo "Build Information:"
	@echo "  OS:           $(DETECTED_OS)"
	@echo "  Architecture: $(DETECTED_ARCH)"
	@echo "  Platform:     $(PLATFORM)"
	@echo "  Library:      $(LIB_NAME)"
	@echo "  Go OS:        $(GO_OS)"
	@echo "  Go Arch:      $(GO_ARCH)"
	@echo "  Project Dir:  $(PROJECT_DIR)"
	@echo "  Uplink-C Dir: $(UPLINK_C_DIR)"
	@echo "  Prebuilds:    $(PLATFORM_DIR)"

# Create directories
$(PLATFORM_DIR):
	$(call MKDIR,$(PLATFORM_DIR))

$(BUILD_DIR):
	$(call MKDIR,$(BUILD_DIR))

$(DIST_DIR):
	$(call MKDIR,$(DIST_DIR))

# Prebuild - prepare platform directory
.PHONY: prebuild
prebuild: $(PLATFORM_DIR)
	@echo "Prebuild directory ready: $(PLATFORM_DIR)"

# Build uplink-c from source (requires Go)
.PHONY: build-uplink
build-uplink: $(PLATFORM_DIR)
	@echo "Building uplink-c for $(PLATFORM)..."
	@echo "Source: $(UPLINK_C_DIR)"
ifdef WINDOWS_NATIVE
	cd "$(UPLINK_C_DIR)" && set CGO_ENABLED=1 && set GOOS=$(GO_OS) && set GOARCH=$(GO_ARCH) && go build -buildmode=c-shared -o $(LIB_NAME)
	$(call CP,$(UPLINK_C_DIR)/$(LIB_NAME),$(PLATFORM_DIR)/$(LIB_NAME))
	@if exist "$(subst /,\,$(UPLINK_C_DIR)/libuplink.h)" $(call CP,$(UPLINK_C_DIR)/libuplink.h,$(INCLUDE_DIR)/uplink.h)
	@if exist "$(subst /,\,$(UPLINK_C_DIR)/uplink_definitions.h)" $(call CP,$(UPLINK_C_DIR)/uplink_definitions.h,$(INCLUDE_DIR)/uplink_definitions.h)
	@if exist "$(subst /,\,$(UPLINK_C_DIR)/uplink_compat.h)" $(call CP,$(UPLINK_C_DIR)/uplink_compat.h,$(INCLUDE_DIR)/uplink_compat.h)
else
	cd "$(UPLINK_C_DIR)" && CGO_ENABLED=1 GOOS=$(GO_OS) GOARCH=$(GO_ARCH) go build -buildmode=c-shared -o $(LIB_NAME)
	$(call CP,$(UPLINK_C_DIR)/$(LIB_NAME),$(PLATFORM_DIR)/$(LIB_NAME))
	@if [ -f "$(UPLINK_C_DIR)/libuplink.h" ]; then $(call CP,$(UPLINK_C_DIR)/libuplink.h,$(INCLUDE_DIR)/uplink.h); fi
	@if [ -f "$(UPLINK_C_DIR)/uplink_definitions.h" ]; then $(call CP,$(UPLINK_C_DIR)/uplink_definitions.h,$(INCLUDE_DIR)/uplink_definitions.h); fi
	@if [ -f "$(UPLINK_C_DIR)/uplink_compat.h" ]; then $(call CP,$(UPLINK_C_DIR)/uplink_compat.h,$(INCLUDE_DIR)/uplink_compat.h); fi
endif
	@echo "Built and installed $(LIB_NAME) to $(PLATFORM_DIR)"
	@echo "Headers installed to $(INCLUDE_DIR)"
	@$(MAKE) fix-header-compat
	@$(MAKE) generate-import-lib

# Copy uplink-c library from parent folder (if already built)
.PHONY: copy-uplink
copy-uplink: $(PLATFORM_DIR)
	@echo "Copying uplink-c library from $(UPLINK_C_DIR)..."
ifdef WINDOWS_NATIVE
	@if exist "$(subst /,\,$(UPLINK_C_DIR)/$(LIB_NAME))" ( \
		$(call CP,$(UPLINK_C_DIR)/$(LIB_NAME),$(PLATFORM_DIR)/$(LIB_NAME)) && \
		echo "Copied $(LIB_NAME) to $(PLATFORM_DIR)" \
	) else ( \
		echo "Library not found. Run 'make build-uplink' first." && exit 1 \
	)
	@if exist "$(subst /,\,$(UPLINK_C_DIR)/libuplink.h)" ( \
		$(call CP,$(UPLINK_C_DIR)/libuplink.h,$(INCLUDE_DIR)/uplink.h) && \
		echo "Copied uplink.h to $(INCLUDE_DIR)" \
	)
	@if exist "$(subst /,\,$(UPLINK_C_DIR)/uplink_definitions.h)" ( \
		$(call CP,$(UPLINK_C_DIR)/uplink_definitions.h,$(INCLUDE_DIR)/uplink_definitions.h) && \
		echo "Copied uplink_definitions.h to $(INCLUDE_DIR)" \
	)
	@if exist "$(subst /,\,$(UPLINK_C_DIR)/uplink_compat.h)" ( \
		$(call CP,$(UPLINK_C_DIR)/uplink_compat.h,$(INCLUDE_DIR)/uplink_compat.h) && \
		echo "Copied uplink_compat.h to $(INCLUDE_DIR)" \
	)
else
	@if [ -f "$(UPLINK_C_DIR)/$(LIB_NAME)" ]; then \
		$(call CP,$(UPLINK_C_DIR)/$(LIB_NAME),$(PLATFORM_DIR)/$(LIB_NAME)); \
		echo "Copied $(LIB_NAME) to $(PLATFORM_DIR)"; \
	else \
		echo "Library not found at $(UPLINK_C_DIR)/$(LIB_NAME)"; \
		echo "Run 'make build-uplink' first."; \
		exit 1; \
	fi
	@if [ -f "$(UPLINK_C_DIR)/libuplink.h" ]; then \
		$(call CP,$(UPLINK_C_DIR)/libuplink.h,$(INCLUDE_DIR)/uplink.h); \
		echo "Copied uplink.h to $(INCLUDE_DIR)"; \
	fi
	@if [ -f "$(UPLINK_C_DIR)/uplink_definitions.h" ]; then \
		$(call CP,$(UPLINK_C_DIR)/uplink_definitions.h,$(INCLUDE_DIR)/uplink_definitions.h); \
		echo "Copied uplink_definitions.h to $(INCLUDE_DIR)"; \
	fi
	@if [ -f "$(UPLINK_C_DIR)/uplink_compat.h" ]; then \
		$(call CP,$(UPLINK_C_DIR)/uplink_compat.h,$(INCLUDE_DIR)/uplink_compat.h); \
		echo "Copied uplink_compat.h to $(INCLUDE_DIR)"; \
	fi
endif
	@$(MAKE) fix-header-compat

# Patch uplink_definitions.h for MSVC compatibility after copying upstream headers.
# MSVC (C mode) forbids empty structs (error C2016). The upstream header has three
# empty "options" structs that only GCC/Clang can handle via #pragma diagnostic.
# This target replaces those empty structs with an #ifdef _MSC_VER / #else block.
DEFS_HEADER := $(INCLUDE_DIR)/uplink_definitions.h
.PHONY: fix-header-compat
fix-header-compat:
	@if [ -f "$(DEFS_HEADER)" ]; then \
		if grep -q '_MSC_VER' "$(DEFS_HEADER)"; then \
			echo "  uplink_definitions.h already has MSVC compat — skipping"; \
		else \
			echo "  Patching uplink_definitions.h for MSVC empty-struct compat..."; \
			node -e " \
				const fs = require('fs'); \
				const f = process.argv[1]; \
				let src = fs.readFileSync(f, 'utf8'); \
				const marker = '// we need to suppress'; \
				const idx = src.indexOf(marker); \
				if (idx === -1) { console.log('  No empty structs found — skipping'); process.exit(0); } \
				const before = src.slice(0, idx); \
				const msvc = [ \
					'#ifdef _MSC_VER', \
					'', \
					'typedef struct UplinkMoveObjectOptions { char _reserved; } UplinkMoveObjectOptions;', \
					'typedef struct UplinkUploadObjectMetadataOptions { char _reserved; } UplinkUploadObjectMetadataOptions;', \
					'typedef struct UplinkCopyObjectOptions { char _reserved; } UplinkCopyObjectOptions;', \
					'', \
					'#else /* GCC / Clang */', \
					'' \
				].join('\n'); \
				const after = src.slice(idx); \
				fs.writeFileSync(f, before + msvc + after + '\n#endif /* _MSC_VER */\n'); \
				console.log('  ✓ Patched'); \
			" "$(DEFS_HEADER)"; \
		fi; \
	fi

# Generate Windows import library (.lib) from the DLL.
# MSVC requires a .lib to link against a DLL at compile time.
# Go's -buildmode=c-shared does not produce one, so we generate it:
#   1. Extract exported symbols from uplink.h → .def file (via Node.js script)
#   2. Create uplink.lib from the .def file
# Tries MSVC lib.exe first (if available via ilammy/msvc-dev-cmd), falls back to
# dlltool (available in MSYS2/Git Bash on GitHub Actions Windows runners).
# This target is called automatically on Windows after building the DLL.
DEF_FILE = $(PLATFORM_DIR)/uplink.def
LIB_FILE = $(PLATFORM_DIR)/uplink.lib
.PHONY: generate-import-lib
generate-import-lib:
ifeq ($(DETECTED_OS),windows)
	@echo "  Generating Windows import library..."
	$(Q)node "$(PROJECT_DIR)/scripts/gen-def.js" "$(INCLUDE_DIR)/uplink.h" "$(DEF_FILE)"
	$(Q)if command -v lib.exe > /dev/null 2>&1; then \
		echo "  Using MSVC lib.exe"; \
		MSYS_NO_PATHCONV=1 lib.exe /def:"$(DEF_FILE)" /out:"$(LIB_FILE)" /machine:x64 /nologo; \
	elif command -v dlltool > /dev/null 2>&1; then \
		echo "  Using dlltool"; \
		dlltool -d "$(DEF_FILE)" -l "$(LIB_FILE)" -D libuplink.dll; \
	else \
		echo "ERROR: Neither lib.exe nor dlltool found. Cannot generate import library."; \
		exit 1; \
	fi
	$(Q)echo "  ✓ Generated $(LIB_FILE)"
endif

# Build TypeScript
.PHONY: build-ts
build-ts:
	@echo "Building TypeScript..."
	npm run build:ts

# Build native addon
.PHONY: build-native
build-native:
	@echo "Building native addon..."
	npm run build:native

# Build all
.PHONY: build
build: build-ts build-native
	@echo "Build complete."

# Test C code
.PHONY: test-c
test-c:
	@echo "Running C tests..."
	npm run test:c

# Test JavaScript
.PHONY: test-js
test-js:
	@echo "Running JavaScript tests..."
	npm test

# Test all
.PHONY: test
test: test-c test-js
	@echo "All tests complete."

# Check all build prerequisites
.PHONY: check
check: check-source-prereqs
	@echo "All prerequisites available."

# Clean build artifacts
.PHONY: clean
clean:
	@echo "Cleaning build artifacts..."
	$(call RMDIR,$(BUILD_DIR))
	$(call RMDIR,$(DIST_DIR))
	$(call RMDIR,$(PROJECT_DIR)/node_modules/.cache)
	@echo "Clean complete."

# Clean everything including prebuilds, downloads, and cloned sources
.PHONY: clean-all
clean-all: clean
	@echo "Cleaning prebuilds..."
	$(call RMDIR,$(PREBUILDS_DIR))
	@echo "Cleaning downloads..."
	$(call RMDIR,$(DOWNLOAD_DIR))
	@echo "Cleaning cloned sources..."
	$(call RMDIR,$(UPLINK_C_BUILD_DIR))
	@echo "Clean all complete."

# Install npm dependencies
.PHONY: npm-install
npm-install:
	npm install

# Install native binaries
# Reads UPLINK_INSTALL env var to select method:
#   UPLINK_INSTALL=prebuilt  → shipped binaries only (no compiler)
#   UPLINK_INSTALL=hybrid    → download lib + compile addon (C compiler)
#   UPLINK_INSTALL=source    → build everything from source (Go + C)
#   UPLINK_INSTALL=skip      → skip native build entirely
#   (unset / auto)           → auto-detect: prebuilt → hybrid → source
#
# Usage:
#   make install                          # auto-detect
#   UPLINK_INSTALL=prebuilt make install  # prebuilt only
#   npm install pkg                       # auto (postinstall calls make install)
#   UPLINK_INSTALL=source npm install pkg # source build
.PHONY: install
install:
	@echo ""
	@echo "╔══════════════════════════════════════════════════════════════╗"
	@echo "║          uplink-nodejs — Native Module Installer             ║"
	@echo "╚══════════════════════════════════════════════════════════════╝"
	@echo ""
	@echo "  Package version : $(ADDON_VERSION)"
	@echo "  Uplink-C version: $(UPLINK_C_VERSION)"
	@echo "  Platform        : $(PLATFORM)"
	@echo "  OS              : $(DETECTED_OS)"
	@echo "  Architecture    : $(DETECTED_ARCH)"
	@echo "  Node.js         : $$(node --version 2>/dev/null || echo 'not found')"
ifdef _UPLINK_FROM_RC
	@echo "  Install method  : $(UPLINK_INSTALL) (from $(UPLINKRC_FILE))"
else
	@echo "  Install method  : $(or $(UPLINK_INSTALL),auto)"
endif
	@echo "  Prebuilds dir   : $(PLATFORM_DIR)"
	@echo ""
ifeq ($(UPLINK_INSTALL),prebuilt)
	@echo "[uplink-nodejs] Method: prebuilt (using shipped binaries)"
	@echo ""
	@$(MAKE) install-prebuilt-shipped
ifndef _UPLINK_FROM_RC
	@echo "$(UPLINK_INSTALL)" > "$(UPLINKRC_FILE)" 2>/dev/null && \
		echo "[uplink-nodejs] ✔ Saved install method to $(UPLINKRC_FILE)" || true
endif
else ifeq ($(UPLINK_INSTALL),hybrid)
	@echo "[uplink-nodejs] Method: hybrid (download lib + compile addon)"
	@echo ""
	@$(MAKE) install-hybrid
ifndef _UPLINK_FROM_RC
	@echo "$(UPLINK_INSTALL)" > "$(UPLINKRC_FILE)" 2>/dev/null && \
		echo "[uplink-nodejs] ✔ Saved install method to $(UPLINKRC_FILE)" || true
endif
else ifeq ($(UPLINK_INSTALL),source)
	@echo "[uplink-nodejs] Method: source (full build from source)"
	@echo ""
	@$(MAKE) install-source
ifndef _UPLINK_FROM_RC
	@echo "$(UPLINK_INSTALL)" > "$(UPLINKRC_FILE)" 2>/dev/null && \
		echo "[uplink-nodejs] ✔ Saved install method to $(UPLINKRC_FILE)" || true
endif
else ifeq ($(UPLINK_INSTALL),skip)
	@echo "[uplink-nodejs] Method: skip — skipping native build"
	@echo ""
ifndef _UPLINK_FROM_RC
	@echo "$(UPLINK_INSTALL)" > "$(UPLINKRC_FILE)" 2>/dev/null && \
		echo "[uplink-nodejs] ✔ Saved install method to $(UPLINKRC_FILE)" || true
endif
else
	@echo "[uplink-nodejs] Method: auto-detect (prebuilt → hybrid → source)"
	@echo ""
	@$(MAKE) install-auto
endif

# Lint
.PHONY: lint
lint:
	npm run lint

# Format
.PHONY: format
format:
	npm run format

# Verify uplink library is installed
.PHONY: verify
verify:
	@echo "Verifying uplink-c installation..."
ifdef WINDOWS_NATIVE
	@if exist "$(subst /,\,$(PLATFORM_DIR)/$(LIB_NAME))" ( \
		echo "OK: $(LIB_NAME) found at $(PLATFORM_DIR)" \
	) else ( \
		echo "ERROR: $(LIB_NAME) not found at $(PLATFORM_DIR)" && \
		echo "Run 'make build-uplink' or 'make copy-uplink' first." && \
		exit 1 \
	)
else
	@if [ -f "$(PLATFORM_DIR)/$(LIB_NAME)" ]; then \
		echo "OK: $(LIB_NAME) found at $(PLATFORM_DIR)"; \
		ls -la "$(PLATFORM_DIR)/$(LIB_NAME)"; \
	else \
		echo "ERROR: $(LIB_NAME) not found at $(PLATFORM_DIR)"; \
		echo "Run 'make build-uplink' or 'make copy-uplink' first."; \
		exit 1; \
	fi
endif

# ==============================================================================
# SPRINT 13: BINARY DISTRIBUTION SYSTEM
# ==============================================================================

# ------------------------------------------------------------------------------
# Configuration
# ------------------------------------------------------------------------------

# Versions
UPLINK_C_VERSION ?= v1.14.0
ADDON_VERSION ?= $(shell node -p "require('./package.json').version" 2>/dev/null || echo "0.1.0")

# GitHub repository for prebuilt binaries
# Override with: make install-hybrid GITHUB_OWNER=your-username
GITHUB_OWNER ?= shivamkumar99
GITHUB_REPO ?= uplink-node-test

# Download URLs (pointing to your repository's releases)
# The prebuilt uplink-c library will be hosted on your repo
UPLINK_C_RELEASE_URL ?= https://github.com/$(GITHUB_OWNER)/$(GITHUB_REPO)/releases/download
ADDON_RELEASE_URL ?= https://github.com/$(GITHUB_OWNER)/$(GITHUB_REPO)/releases/download

# Archive names
UPLINK_C_ARCHIVE = uplink-c-$(UPLINK_C_VERSION)-$(PLATFORM).tar.gz
ADDON_ARCHIVE = uplink-nodejs-v$(ADDON_VERSION)-$(PLATFORM).tar.gz

# Node addon name
NODE_ADDON = uplink_native.node

# Directories
DOWNLOAD_DIR = $(PROJECT_DIR)/.downloads
UPLINK_C_BUILD_DIR ?= $(PROJECT_DIR)/.uplink-c-build

# Flags
VERBOSE ?= 0
FORCE_DOWNLOAD ?= 0
SKIP_VERIFY ?= 0

# Verbose output control
ifeq ($(VERBOSE),1)
    Q =
    CURL_QUIET =
    ECHO_VERBOSE = @echo
else
    Q = @
    CURL_QUIET = -s
    ECHO_VERBOSE = @true
endif

# ------------------------------------------------------------------------------
# Prerequisites Checks
# ------------------------------------------------------------------------------

.PHONY: check-node
check-node:
	$(Q)echo "Checking Node.js..."
ifdef WINDOWS_NATIVE
	$(Q)where node >nul 2>&1 || (echo "ERROR: Node.js not found. Install from https://nodejs.org" && exit 1)
else
	$(Q)which node > /dev/null 2>&1 || (echo "ERROR: Node.js not found. Install from https://nodejs.org" && exit 1)
endif
	$(Q)echo "  ✓ Node.js $$(node --version)"

.PHONY: check-npm
check-npm: check-node
	$(Q)echo "Checking npm..."
ifdef WINDOWS_NATIVE
	$(Q)where npm >nul 2>&1 || (echo "ERROR: npm not found" && exit 1)
else
	$(Q)which npm > /dev/null 2>&1 || (echo "ERROR: npm not found" && exit 1)
endif
	$(Q)echo "  ✓ npm v$$(npm --version)"

.PHONY: check-go
check-go:
	$(Q)echo "Checking Go..."
ifdef WINDOWS_NATIVE
	$(Q)where go >nul 2>&1 || (echo "ERROR: Go not found. Install from https://golang.org/dl/" && exit 1)
else
	$(Q)which go > /dev/null 2>&1 || (echo "ERROR: Go not found. Install from https://golang.org/dl/" && exit 1)
endif
	$(Q)echo "  ✓ $$(go version | head -c 30)"

.PHONY: check-git
check-git:
	$(Q)echo "Checking Git..."
ifdef WINDOWS_NATIVE
	$(Q)where git >nul 2>&1 || (echo "ERROR: Git not found. Install from https://git-scm.com/" && exit 1)
else
	$(Q)which git > /dev/null 2>&1 || (echo "ERROR: Git not found. Install from https://git-scm.com/" && exit 1)
endif
	$(Q)echo "  ✓ $$(git --version)"

.PHONY: check-compiler
check-compiler:
	$(Q)echo "Checking C compiler..."
ifdef WINDOWS_NATIVE
	$(Q)(where cl >nul 2>&1 || where gcc >nul 2>&1) || (echo "ERROR: No C compiler found. Install Visual Studio Build Tools" && exit 1)
else
	$(Q)(which cc > /dev/null 2>&1 || which gcc > /dev/null 2>&1 || which clang > /dev/null 2>&1) || (echo "ERROR: No C compiler found" && exit 1)
endif
	$(Q)echo "  ✓ C compiler available"

.PHONY: check-python
check-python:
	$(Q)echo "Checking Python (for node-gyp)..."
ifdef WINDOWS_NATIVE
	$(Q)(where python >nul 2>&1 || where python3 >nul 2>&1) || (echo "ERROR: Python not found" && exit 1)
else
	$(Q)(which python3 > /dev/null 2>&1 || which python > /dev/null 2>&1) || (echo "ERROR: Python not found" && exit 1)
endif
	$(Q)echo "  ✓ Python available"

.PHONY: check-curl
check-curl:
	$(Q)echo "Checking curl..."
ifdef WINDOWS_NATIVE
	$(Q)where curl >nul 2>&1 || (echo "ERROR: curl not found" && exit 1)
else
	$(Q)which curl > /dev/null 2>&1 || (echo "ERROR: curl not found" && exit 1)
endif
	$(Q)echo "  ✓ curl available"

# Combined prerequisite checks
.PHONY: check-source-prereqs
check-source-prereqs: check-node check-npm check-go check-git check-compiler check-python
	$(Q)echo ""
	$(Q)echo "✓ All prerequisites for source build satisfied"

.PHONY: check-hybrid-prereqs
check-hybrid-prereqs: check-node check-npm check-compiler check-python check-curl
	$(Q)echo ""
	$(Q)echo "✓ All prerequisites for hybrid build satisfied"

.PHONY: check-prebuilt-prereqs
check-prebuilt-prereqs: check-node check-curl
	$(Q)echo ""
	$(Q)echo "✓ All prerequisites for prebuilt download satisfied"

# ------------------------------------------------------------------------------
# Download Targets
# ------------------------------------------------------------------------------

$(DOWNLOAD_DIR):
	$(call MKDIR,$(DOWNLOAD_DIR))

# Download uplink-c library from GitHub releases
.PHONY: download-lib
download-lib: $(PLATFORM_DIR) $(DOWNLOAD_DIR) check-curl
	$(Q)echo "══════════════════════════════════════════════════════════════"
	$(Q)echo "  Downloading uplink-c $(UPLINK_C_VERSION) for $(PLATFORM)"
	$(Q)echo "══════════════════════════════════════════════════════════════"
ifeq ($(FORCE_DOWNLOAD),0)
ifdef WINDOWS_NATIVE
	$(Q)if exist "$(subst /,\,$(PLATFORM_DIR)/$(LIB_NAME))" ( \
		echo "Library already exists: $(PLATFORM_DIR)/$(LIB_NAME)" && \
		echo "Use FORCE_DOWNLOAD=1 to re-download" && \
		exit 0 \
	)
else
	$(Q)if [ -f "$(PLATFORM_DIR)/$(LIB_NAME)" ]; then \
		echo "Library already exists: $(PLATFORM_DIR)/$(LIB_NAME)"; \
		echo "Use FORCE_DOWNLOAD=1 to re-download"; \
		exit 0; \
	fi
endif
endif
	$(Q)echo "URL: $(UPLINK_C_RELEASE_URL)/$(UPLINK_C_VERSION)/$(UPLINK_C_ARCHIVE)"
	$(Q)curl $(CURL_QUIET) -L -f \
		"$(UPLINK_C_RELEASE_URL)/$(UPLINK_C_VERSION)/$(UPLINK_C_ARCHIVE)" \
		-o "$(DOWNLOAD_DIR)/$(UPLINK_C_ARCHIVE)" || \
		(echo "ERROR: Download failed. Prebuilt may not be available for $(PLATFORM)" && \
		 echo "Try: make install-source" && exit 1)
	$(Q)echo "Extracting to $(PLATFORM_DIR)..."
	$(Q)tar -xzf "$(DOWNLOAD_DIR)/$(UPLINK_C_ARCHIVE)" -C "$(PLATFORM_DIR)"
	$(Q)rm -f "$(DOWNLOAD_DIR)/$(UPLINK_C_ARCHIVE)"
	$(Q)echo "✓ Downloaded $(LIB_NAME)"

# Download prebuilt Node addon from GitHub releases
.PHONY: download-addon
download-addon: $(PLATFORM_DIR) $(DOWNLOAD_DIR) check-curl
	$(Q)echo "══════════════════════════════════════════════════════════════"
	$(Q)echo "  Downloading Node addon v$(ADDON_VERSION) for $(PLATFORM)"
	$(Q)echo "══════════════════════════════════════════════════════════════"
ifeq ($(FORCE_DOWNLOAD),0)
ifdef WINDOWS_NATIVE
	$(Q)if exist "$(subst /,\,$(PLATFORM_DIR)/$(NODE_ADDON))" ( \
		echo "Addon already exists: $(PLATFORM_DIR)/$(NODE_ADDON)" && \
		echo "Use FORCE_DOWNLOAD=1 to re-download" && \
		exit 0 \
	)
else
	$(Q)if [ -f "$(PLATFORM_DIR)/$(NODE_ADDON)" ]; then \
		echo "Addon already exists: $(PLATFORM_DIR)/$(NODE_ADDON)"; \
		echo "Use FORCE_DOWNLOAD=1 to re-download"; \
		exit 0; \
	fi
endif
endif
	$(Q)echo "URL: $(ADDON_RELEASE_URL)/v$(ADDON_VERSION)/$(ADDON_ARCHIVE)"
	$(Q)curl $(CURL_QUIET) -L -f \
		"$(ADDON_RELEASE_URL)/v$(ADDON_VERSION)/$(ADDON_ARCHIVE)" \
		-o "$(DOWNLOAD_DIR)/$(ADDON_ARCHIVE)" || \
		(echo "ERROR: Download failed. Prebuilt may not be available for $(PLATFORM)" && \
		 echo "Try: make install-hybrid" && exit 1)
	$(Q)echo "Extracting to $(PLATFORM_DIR)..."
	$(Q)tar -xzf "$(DOWNLOAD_DIR)/$(ADDON_ARCHIVE)" -C "$(PLATFORM_DIR)"
	$(Q)rm -f "$(DOWNLOAD_DIR)/$(ADDON_ARCHIVE)"
	$(Q)echo "✓ Downloaded $(NODE_ADDON)"

# ------------------------------------------------------------------------------
# Clone uplink-c from GitHub
# ------------------------------------------------------------------------------

.PHONY: clone-uplink
clone-uplink: check-git
	$(Q)echo "══════════════════════════════════════════════════════════════"
	$(Q)echo "  Cloning uplink-c $(UPLINK_C_VERSION)"
	$(Q)echo "══════════════════════════════════════════════════════════════"
ifdef WINDOWS_NATIVE
	$(Q)if exist "$(subst /,\,$(UPLINK_C_BUILD_DIR))" ( \
		echo "uplink-c source exists at $(UPLINK_C_BUILD_DIR)" && \
		cd "$(UPLINK_C_BUILD_DIR)" && git pull origin main 2>nul || echo "Using existing source" \
	) else ( \
		echo "Cloning from https://github.com/storj/uplink-c.git..." && \
		git clone --depth 1 https://github.com/storj/uplink-c.git "$(UPLINK_C_BUILD_DIR)" \
	)
else
	$(Q)if [ -d "$(UPLINK_C_BUILD_DIR)" ]; then \
		echo "uplink-c source exists at $(UPLINK_C_BUILD_DIR)"; \
		cd "$(UPLINK_C_BUILD_DIR)" && git pull origin main 2>/dev/null || echo "Using existing source"; \
	else \
		echo "Cloning from https://github.com/storj/uplink-c.git..."; \
		git clone --depth 1 https://github.com/storj/uplink-c.git "$(UPLINK_C_BUILD_DIR)"; \
	fi
endif
	$(Q)echo "✓ uplink-c source ready at $(UPLINK_C_BUILD_DIR)"

# Build from cloned uplink-c source
.PHONY: build-uplink-cloned
build-uplink-cloned: clone-uplink $(PLATFORM_DIR) check-go
	$(Q)echo "══════════════════════════════════════════════════════════════"
	$(Q)echo "  Building uplink-c for $(PLATFORM)"
	$(Q)echo "══════════════════════════════════════════════════════════════"
	$(Q)echo "Source: $(UPLINK_C_BUILD_DIR)"
	$(Q)echo "Target: $(PLATFORM_DIR)/$(LIB_NAME)"
ifdef WINDOWS_NATIVE
	$(Q)cd "$(UPLINK_C_BUILD_DIR)" && set CGO_ENABLED=1 && set GOOS=$(GO_OS) && set GOARCH=$(GO_ARCH) && go build -buildmode=c-shared -o $(LIB_NAME)
else
	$(Q)cd "$(UPLINK_C_BUILD_DIR)" && CGO_ENABLED=1 GOOS=$(GO_OS) GOARCH=$(GO_ARCH) go build -buildmode=c-shared -o $(LIB_NAME)
endif
	$(Q)$(call CP,$(UPLINK_C_BUILD_DIR)/$(LIB_NAME),$(PLATFORM_DIR)/$(LIB_NAME))
ifdef WINDOWS_NATIVE
	$(Q)if exist "$(subst /,\,$(UPLINK_C_BUILD_DIR)/libuplink.h)" $(call CP,$(UPLINK_C_BUILD_DIR)/libuplink.h,$(INCLUDE_DIR)/uplink.h)
else
	$(Q)if [ -f "$(UPLINK_C_BUILD_DIR)/libuplink.h" ]; then $(call CP,$(UPLINK_C_BUILD_DIR)/libuplink.h,$(INCLUDE_DIR)/uplink.h); fi
endif
	$(Q)echo "✓ Built $(LIB_NAME)"
	$(Q)ls -lh "$(PLATFORM_DIR)/$(LIB_NAME)" 2>/dev/null || dir "$(PLATFORM_DIR)\$(LIB_NAME)"
	@$(MAKE) generate-import-lib

# Copy built addon to prebuilds
.PHONY: copy-addon-to-prebuilds
copy-addon-to-prebuilds: $(PLATFORM_DIR)
	$(Q)echo "Copying addon to prebuilds..."
ifdef WINDOWS_NATIVE
	$(Q)if exist "$(subst /,\,$(BUILD_DIR)/Release/uplink_native.node)" ( \
		$(call CP,$(BUILD_DIR)/Release/uplink_native.node,$(PLATFORM_DIR)/$(NODE_ADDON)) && \
		echo "✓ Copied $(NODE_ADDON) to $(PLATFORM_DIR)" \
	) else ( \
		echo "ERROR: Addon not found at $(BUILD_DIR)/Release/uplink_native.node" && exit 1 \
	)
else
	$(Q)if [ -f "$(BUILD_DIR)/Release/uplink_native.node" ]; then \
		$(call CP,$(BUILD_DIR)/Release/uplink_native.node,$(PLATFORM_DIR)/$(NODE_ADDON)); \
		echo "✓ Copied $(NODE_ADDON) to $(PLATFORM_DIR)"; \
	else \
		echo "ERROR: Addon not found at $(BUILD_DIR)/Release/uplink_native.node"; \
		exit 1; \
	fi
endif

# ------------------------------------------------------------------------------
# Installation Options
# ------------------------------------------------------------------------------

# OPTION 1: Full Source Build
# Builds both uplink-c library and Node addon from source
# Requirements: Go, Git, C compiler, Python, Node.js
.PHONY: install-source
install-source:
	$(Q)echo ""
	$(Q)echo "╔══════════════════════════════════════════════════════════════╗"
	$(Q)echo "║  OPTION 1: Full Source Build                                 ║"
	$(Q)echo "║  Building uplink-c + Node addon from source                  ║"
	$(Q)echo "╚══════════════════════════════════════════════════════════════╝"
	$(Q)echo ""
	$(Q)echo "Platform: $(PLATFORM)"
	$(Q)echo "Output:   $(PLATFORM_DIR)"
	$(Q)echo ""
	@$(MAKE) check-source-prereqs
	$(Q)echo ""
	$(Q)echo "[1/4] Preparing uplink-c source..."
ifdef WINDOWS_NATIVE
	@if exist "$(subst /,\,$(UPLINK_C_DIR)/go.mod)" ( \
		echo "Using existing uplink-c at $(UPLINK_C_DIR)" && \
		$(MAKE) build-uplink \
	) else ( \
		$(MAKE) build-uplink-cloned \
	)
else
	@if [ -f "$(UPLINK_C_DIR)/go.mod" ]; then \
		echo "Using existing uplink-c at $(UPLINK_C_DIR)"; \
		$(MAKE) build-uplink; \
	else \
		$(MAKE) build-uplink-cloned; \
	fi
endif
	$(Q)echo ""
	$(Q)echo "[2/4] Building Node addon..."
	@$(MAKE) build-native
	$(Q)echo ""
	$(Q)echo "[3/4] Copying addon to prebuilds..."
	@$(MAKE) copy-addon-to-prebuilds
	$(Q)echo ""
	$(Q)echo "[4/4] Verifying installation..."
ifneq ($(SKIP_VERIFY),1)
	@$(MAKE) verify-full
endif
	$(Q)echo ""
	$(Q)echo "╔══════════════════════════════════════════════════════════════╗"
	$(Q)echo "║  ✓ Installation complete (Source Build)                      ║"
	$(Q)echo "╚══════════════════════════════════════════════════════════════╝"
	$(Q)echo ""

# OPTION 2: Hybrid Build (Default)
# Downloads prebuilt uplink-c library, builds Node addon locally
# Requirements: C compiler, Python, Node.js (NO Go needed)
.PHONY: install-hybrid
install-hybrid:
	$(Q)echo ""
	$(Q)echo "╔══════════════════════════════════════════════════════════════╗"
	$(Q)echo "║  OPTION 2: Hybrid Build                                      ║"
	$(Q)echo "║  Download uplink-c library, build Node addon locally         ║"
	$(Q)echo "╚══════════════════════════════════════════════════════════════╝"
	$(Q)echo ""
	$(Q)echo "Platform: $(PLATFORM)"
	$(Q)echo "Output:   $(PLATFORM_DIR)"
	$(Q)echo ""
	@$(MAKE) check-hybrid-prereqs
	$(Q)echo ""
	$(Q)echo "[1/3] Downloading uplink-c library..."
	@$(MAKE) download-lib
	$(Q)echo ""
	$(Q)echo "[2/3] Building Node addon..."
	@$(MAKE) build-native
	@$(MAKE) copy-addon-to-prebuilds
	$(Q)echo ""
	$(Q)echo "[3/3] Verifying installation..."
ifneq ($(SKIP_VERIFY),1)
	@$(MAKE) verify-full
endif
	$(Q)echo ""
	$(Q)echo "╔══════════════════════════════════════════════════════════════╗"
	$(Q)echo "║  ✓ Installation complete (Hybrid Build)                      ║"
	$(Q)echo "╚══════════════════════════════════════════════════════════════╝"
	$(Q)echo ""

# OPTION 3: Full Prebuilt
# Downloads both prebuilt uplink-c library AND Node addon
# Requirements: Node.js only (NO compilation)
.PHONY: install-prebuilt
install-prebuilt:
	$(Q)echo ""
	$(Q)echo "╔══════════════════════════════════════════════════════════════╗"
	$(Q)echo "║  OPTION 3: Full Prebuilt                                     ║"
	$(Q)echo "║  Download everything - no compilation required               ║"
	$(Q)echo "╚══════════════════════════════════════════════════════════════╝"
	$(Q)echo ""
	$(Q)echo "Platform: $(PLATFORM)"
	$(Q)echo "Output:   $(PLATFORM_DIR)"
	$(Q)echo ""
	@$(MAKE) check-prebuilt-prereqs
	$(Q)echo ""
	$(Q)echo "[1/3] Downloading uplink-c library..."
	@$(MAKE) download-lib
	$(Q)echo ""
	$(Q)echo "[2/3] Downloading Node addon..."
	@$(MAKE) download-addon
	$(Q)echo ""
	$(Q)echo "[3/3] Verifying installation..."
ifneq ($(SKIP_VERIFY),1)
	@$(MAKE) verify-full
endif
	$(Q)echo ""
	$(Q)echo "╔══════════════════════════════════════════════════════════════╗"
	$(Q)echo "║  ✓ Installation complete (Prebuilt - No compilation!)        ║"
	$(Q)echo "╚══════════════════════════════════════════════════════════════╝"
	$(Q)echo ""

# Check if shipped prebuilt binaries exist and work (no download needed)
.PHONY: install-prebuilt-shipped
install-prebuilt-shipped:
	$(Q)echo ""
	$(Q)echo "╔══════════════════════════════════════════════════════════════╗"
	$(Q)echo "║  Prebuilt: Using shipped binaries (no compilation)           ║"
	$(Q)echo "╚══════════════════════════════════════════════════════════════╝"
	$(Q)echo ""
	$(Q)echo "Platform: $(PLATFORM)"
	$(Q)echo "Checking: $(PLATFORM_DIR)"
	$(Q)echo ""
ifdef WINDOWS_NATIVE
	$(Q)if exist "$(subst /,\,$(PLATFORM_DIR)/$(LIB_NAME))" if exist "$(subst /,\,$(PLATFORM_DIR)/$(NODE_ADDON))" ( \
		echo "✓ $(LIB_NAME) found" && \
		echo "✓ $(NODE_ADDON) found" && \
		echo "" && \
		echo "╔══════════════════════════════════════════════════════════════╗" && \
		echo "║  ✓ Prebuilt binaries ready — no compilation needed!          ║" && \
		echo "╚══════════════════════════════════════════════════════════════╝" \
	) else ( \
		echo "✗ Prebuilt binaries not found for $(PLATFORM)" && exit 1 \
	)
else
	@if [ -f "$(PLATFORM_DIR)/$(LIB_NAME)" ] && [ -f "$(PLATFORM_DIR)/$(NODE_ADDON)" ]; then \
		echo "✓ $(LIB_NAME) — $$(ls -lh $(PLATFORM_DIR)/$(LIB_NAME) | awk '{print $$5}')"; \
		echo "✓ $(NODE_ADDON) — $$(ls -lh $(PLATFORM_DIR)/$(NODE_ADDON) | awk '{print $$5}')"; \
		echo ""; \
		echo "╔══════════════════════════════════════════════════════════════╗"; \
		echo "║  ✓ Prebuilt binaries ready — no compilation needed!          ║"; \
		echo "╚══════════════════════════════════════════════════════════════╝"; \
	else \
		echo "✗ Prebuilt binaries not found for $(PLATFORM)"; \
		exit 1; \
	fi
endif

# Default install - auto-detect best method
# Order: prebuilt-shipped → hybrid → source
.PHONY: install-auto
install-auto:
	@echo ""
	@echo "╔══════════════════════════════════════════════════════════════╗"
	@echo "║  Auto-detecting best installation method...                  ║"
	@echo "╚══════════════════════════════════════════════════════════════╝"
	@echo ""
	@echo "[uplink-nodejs] Step 1: Trying prebuilt shipped binaries..."
	@$(MAKE) install-prebuilt-shipped 2>/dev/null \
		&& echo "" \
		&& echo "[uplink-nodejs] ✓ Prebuilt shipped binaries — success" \
		|| ( \
			echo "[uplink-nodejs] ✗ Prebuilt shipped binaries not available"; \
			echo ""; \
			echo "[uplink-nodejs] Step 2: Trying hybrid build (download lib + compile addon)..."; \
			$(MAKE) install-hybrid 2>&1 \
			&& echo "" \
			&& echo "[uplink-nodejs] ✓ Hybrid build — success" \
			|| ( \
				echo "[uplink-nodejs] ✗ Hybrid build failed"; \
				echo ""; \
				echo "[uplink-nodejs] Step 3: Trying full source build..."; \
				$(MAKE) install-source \
			) \
		)

# ------------------------------------------------------------------------------
# Verification
# ------------------------------------------------------------------------------

.PHONY: verify-full
verify-full:
	$(Q)echo "Verifying installation..."
	$(Q)echo ""
	$(Q)echo "Files in $(PLATFORM_DIR):"
ifdef WINDOWS_NATIVE
	$(Q)dir "$(subst /,\,$(PLATFORM_DIR))" 2>nul || echo "  (directory not found)"
else
	$(Q)ls -lh "$(PLATFORM_DIR)" 2>/dev/null || echo "  (directory not found)"
endif
	$(Q)echo ""
	@# Check library
ifdef WINDOWS_NATIVE
	$(Q)if exist "$(subst /,\,$(PLATFORM_DIR)/$(LIB_NAME))" ( \
		echo "✓ $(LIB_NAME)" \
	) else ( \
		echo "✗ $(LIB_NAME) - NOT FOUND" && exit 1 \
	)
	$(Q)if exist "$(subst /,\,$(PLATFORM_DIR)/$(NODE_ADDON))" ( \
		echo "✓ $(NODE_ADDON)" \
	) else ( \
		echo "✗ $(NODE_ADDON) - NOT FOUND" && exit 1 \
	)
else
	$(Q)if [ -f "$(PLATFORM_DIR)/$(LIB_NAME)" ]; then \
		echo "✓ $(LIB_NAME) - $$(ls -lh $(PLATFORM_DIR)/$(LIB_NAME) | awk '{print $$5}')"; \
	else \
		echo "✗ $(LIB_NAME) - NOT FOUND" && exit 1; \
	fi
	$(Q)if [ -f "$(PLATFORM_DIR)/$(NODE_ADDON)" ]; then \
		echo "✓ $(NODE_ADDON) - $$(ls -lh $(PLATFORM_DIR)/$(NODE_ADDON) | awk '{print $$5}')"; \
	else \
		echo "✗ $(NODE_ADDON) - NOT FOUND" && exit 1; \
	fi
endif
	$(Q)echo ""
	$(Q)echo "Verification passed!"

# ------------------------------------------------------------------------------
# Clean Targets
# ------------------------------------------------------------------------------

.PHONY: clean-downloads
clean-downloads:
	$(Q)echo "Cleaning downloads..."
	$(call RMDIR,$(DOWNLOAD_DIR))
	$(Q)echo "✓ Downloads cleaned"

.PHONY: clean-prebuilds
clean-prebuilds:
	$(Q)echo "Cleaning prebuilds for $(PLATFORM)..."
	$(call RMDIR,$(PLATFORM_DIR))
	$(Q)echo "✓ Prebuilds cleaned"

.PHONY: clean-clone
clean-clone:
	$(Q)echo "Cleaning cloned uplink-c..."
	$(call RMDIR,$(UPLINK_C_BUILD_DIR))
	$(Q)echo "✓ Cloned source cleaned"

.PHONY: clean-all-dist
clean-all-dist: clean clean-downloads clean-prebuilds clean-clone
	$(Q)echo "✓ All distribution artifacts cleaned"

# ------------------------------------------------------------------------------
# Help (Updated)
# ------------------------------------------------------------------------------

.PHONY: help
help:
	@echo ""
	@echo "╔══════════════════════════════════════════════════════════════════╗"
	@echo "║                    uplink-nodejs Makefile                        ║"
	@echo "╚══════════════════════════════════════════════════════════════════╝"
	@echo ""
	@echo "  Platform: $(PLATFORM)"
	@echo "  Library:  $(LIB_NAME)"
	@echo ""
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@echo "  INSTALLATION OPTIONS"
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@echo ""
	@echo "  make install            Auto-detect (prebuilt → hybrid → source)"
	@echo "  make install-source     Build everything from source (needs Go)"
	@echo "  make install-hybrid     Download lib, build addon (needs compiler)"
	@echo "  make install-prebuilt   Download everything (Node.js only)"
	@echo ""
	@echo "  Via npm (one command):"
	@echo "    npm install <pkg>                           # auto-detect"
	@echo "    UPLINK_INSTALL=prebuilt npm install <pkg>   # use shipped binaries"
	@echo "    UPLINK_INSTALL=hybrid  npm install <pkg>    # compile addon only"
	@echo "    UPLINK_INSTALL=source  npm install <pkg>    # full source build"
	@echo "    UPLINK_INSTALL=skip    npm install <pkg>    # skip native build"
	@echo ""
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@echo "  BUILD TARGETS"
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@echo ""
	@echo "  make build              Build TypeScript + native addon"
	@echo "  make build-ts           Build TypeScript only"
	@echo "  make build-native       Build native addon only (node-gyp)"
	@echo "  make build-uplink       Build uplink-c from ../uplink-c"
	@echo "  make clone-uplink       Clone uplink-c from GitHub"
	@echo ""
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@echo "  DOWNLOAD TARGETS"
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@echo ""
	@echo "  make download-lib       Download prebuilt uplink-c library"
	@echo "  make download-addon     Download prebuilt Node addon"
	@echo ""
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@echo "  TEST & VERIFY"
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@echo ""
	@echo "  make test               Run all tests"
	@echo "  make verify             Verify uplink-c library"
	@echo "  make verify-full        Verify library + addon"
	@echo "  make check              Check all prerequisites"
	@echo ""
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@echo "  CLEAN"
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@echo ""
	@echo "  make clean              Clean build artifacts"
	@echo "  make clean-downloads    Clean downloaded files"
	@echo "  make clean-prebuilds    Clean prebuilt binaries"
	@echo "  make clean-all          Clean everything"
	@echo ""
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@echo "  VARIABLES"
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@echo ""
	@echo "  UPLINK_INSTALL     Install method: auto|prebuilt|hybrid|source|skip"
	@echo "  UPLINK_C_DIR       Path to uplink-c source (default: ../uplink-c)"
	@echo "  UPLINK_C_VERSION   Version to download/clone (default: $(UPLINK_C_VERSION))"
	@echo "  GITHUB_OWNER       GitHub owner for prebuilt downloads (default: $(GITHUB_OWNER))"
	@echo "  GITHUB_REPO        GitHub repo for prebuilt downloads (default: $(GITHUB_REPO))"
	@echo "  PLATFORM           Target platform (default: $(PLATFORM))"
	@echo "  FORCE_DOWNLOAD=1   Force re-download even if exists"
	@echo "  VERBOSE=1          Enable verbose output"
	@echo ""
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@echo "  EXAMPLES"
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@echo ""
	@echo "  npm install @shivam995364/uplink-nodejs-test          # auto"
	@echo "  UPLINK_INSTALL=prebuilt npm install @shivam995364/uplink-nodejs-test"
	@echo "  UPLINK_INSTALL=source UPLINK_C_DIR=~/uplink-c npm install ..."
	@echo "  make install-hybrid VERBOSE=1          # Direct Make"
	@echo "  make download-lib UPLINK_C_VERSION=v1.13.0"
	@echo ""
