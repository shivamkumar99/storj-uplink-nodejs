# <b>storj-uplink-nodejs binding</b>

#### *Node.js native bindings for Storj's uplink-c library — v0.1.0-beta.21*

A modern, TypeScript-first binding using pure C + Node-API (N-API) for connecting to the Storj decentralized cloud storage network. Prebuilt binaries are available for macOS, Linux, and Windows — no Go toolchain required for most installs.

---

## <b>Documentation</b>

| Document | Description |
| --- | --- |
| [API Reference](API.md) | Quick-reference tables for all classes, methods, and standalone functions |
| [Binding Functions](library.md) | Full reference for every binding function with arguments and examples |
| [Types, Errors & Constants](types.md) | All TypeScript types, error class hierarchy, and error codes |
| [Tutorial](tutorial.md) | Step-by-step guide: connect → upload → download → cleanup |
| [Examples](EXAMPLES.md) | How to run the 8 example scripts in the `example/` directory |
| [CI / Continuous Integration](CI.md) | All GitHub Actions workflows, test commands, and build matrix |

---

## <b>Why This Package? — Differences from the Original Storj Binding</b>

This package (`storj-uplink-nodejs`) is a full rewrite of the original [`uplink-nodejs`](https://github.com/storj-thirdparty/uplink-nodejs) binding published by Storj. Every layer — build system, native C code, error handling, library loading, and TypeScript API — has been redesigned. Below is a summary of the five major differences.

---

### 1. Build Process — Prebuilt Binaries, No Go Required by Default

**Original:** Installation always compiled everything from source. `preinstall` ran `make` which cloned and built `uplink-c` with Go, then `node-gyp` compiled the C++ addon. Go (>= 1.17), a C++ compiler, and `make` were all mandatory on every developer machine.

```
# Original — always runs on every npm install:
preinstall → make (clones + builds uplink-c with Go)
install    → node-gyp configure && node-gyp rebuild
```

**This package:** Four install modes controlled by the `UPLINK_INSTALL` environment variable, with auto-detection:

| Mode | What runs | Requires |
| --- | --- | --- |
| `prebuilt` *(default)* | Downloads prebuilt `libuplink` + `.node` addon from GitHub Releases | Node.js only |
| `hybrid` | Downloads prebuilt `libuplink`, compiles addon locally | C compiler |
| `source` | Builds `uplink-c` from source + compiles addon | Go + C compiler |
| `skip` | No native build | Nothing |

The auto-detect fallback tries `prebuilt → hybrid → source` in order, so most users never need Go installed. The chosen method persists in a `.uplinkrc` file so future `npm install` runs reuse it automatically.

---

### 2. Native Logging — Built-in Diagnostics via Environment Variables

**Original:** No logging at any layer. When something failed during install or at runtime, there was no way to see what the native addon was doing internally.

**This package:** A purpose-built logger lives in the native C layer (`native/src/common/logger.c`). It is fully controlled by environment variables — no code changes needed.

```sh
UPLINK_LOG_LEVEL=debug node app.js      # console output (stderr, colour-coded)
UPLINK_LOG_FILE=./uplink.log node app.js # write to file (appended, no colours)
```

Log levels: `none` · `error` · `warn` · `info` (default) · `debug` · `trace`

Every native source file (`library_loader.c`, `error_registry.c`, `handle_helpers.c`, etc.) emits structured log lines with timestamp, level, source file, line number, and function name:

```
[2024-01-15 10:23:45] DEBUG [library_loader.c:87 load_uplink_library()] Loaded libuplink from native/prebuilds/darwin-arm64/libuplink.dylib
[2024-01-15 10:23:46] ERROR [upload_ops.c:112 upload_write()] Write failed: connection timeout
```

---

### 3. Unified Codebase — No Duplicate Windows / Linux Files

**Original:** The native C++ layer had two completely separate copies of the source tree — one for Linux/macOS (`functions/`) and one for Windows (`functions_win/`), each containing 18 near-identical files:

```
functions/          ← Linux / macOS
  libUplink.cc
  libUplink.h
  promises_execute.cc
  promises_complete.cc
  ...18 files

functions_win/      ← Windows — duplicated code
  libUplink_win.cc
  libUplink_win.h
  promises_execute.cc
  promises_complete.cc
  ...18 files
```

`binding.gyp` selected the entire directory based on the target OS, meaning any bug fix or feature had to be applied twice.

**This package:** A single unified source tree (`native/src/`) compiles on all platforms. Platform differences are handled with narrow `#ifdef` guards inside shared helper files. For example, the entire platform difference for loading the shared library is isolated in one abstraction in `library_loader.c`:

```c
#ifdef _WIN32
    #define LOAD_LIBRARY(path) LoadLibraryA(path)
    #define GET_SYMBOL(handle, name) GetProcAddress((HMODULE)handle, name)
#else
    #define LOAD_LIBRARY(path) dlopen(path, RTLD_NOW | RTLD_LOCAL)
    #define GET_SYMBOL(handle, name) dlsym(handle, name)
#endif
```

Every other source file is identical across all platforms.

---

### 4. Error Handling in Native C — No JS Wrapper Mapping

**Original:** Error codes returned from the C++ addon were plain integers. A JavaScript file (`error.js`) contained a large `switch` statement that mapped codes to `class` definitions:

```js
// error.js — original approach
function storjException(code, details) {
    switch (code) {
        case 0x02: throw new InternalError(details);
        case 0x13: throw new BucketNotFoundError(details);
        case 0x21: throw new ObjectNotFoundError(details);
        // ...
    }
}
```

This meant `instanceof` checks could fail across module boundaries, and error classes were defined entirely in JavaScript with no native awareness.

**This package:** Error classification happens entirely inside the native C layer (`native/src/common/error_registry.c`). The C code evaluates the `uplink_error` code, selects the correct typed error constructor, and calls `napi_new_instance()` to create a properly typed JS error before it ever reaches TypeScript. The error classes themselves are defined via an embedded JS snippet that is evaluated in the caller's realm — meaning `instanceof` works correctly even inside Jest sandboxes or multiple module contexts:

```c
// error_registry.c — native C creates the right typed error
napi_value create_typed_error(napi_env env, int32_t code, const char* message) {
    // look up constructor for this error code
    // call napi_new_instance() → returns new BucketNotFoundError(...)
}
```

The TypeScript layer re-exports these classes with full type annotations but does not duplicate the class definitions or mapping logic.

---

### 5. Dynamic Library Loading — Runtime Resolution Instead of Link-Time

**Original:** `libuplink` was linked at compile time via `binding.gyp` linker flags (e.g. `-luplinkcv1.2.4`). The library had to be in a hard-coded location at build time. There was no mechanism to locate the library at runtime or swap it without a recompile.

**This package:** The native addon does not link `libuplink` at compile time. Instead, `library_loader.c` loads it at runtime using `dlopen` (Linux/macOS) or `LoadLibraryA` (Windows) and resolves each function symbol with `dlsym` / `GetProcAddress`. The loader searches a priority chain of paths:

1. `UPLINK_LIBRARY_PATH` environment variable (user override)
2. `native/prebuilds/<platform>/libuplink.{dylib,so,dll}` (shipped prebuilt)
3. `prebuilds/<platform>/libuplink.{dylib,so,dll}` (alternate relative path)
4. System library directories

This decouples the compiled addon from the library binary, which enables the prebuilt distribution model — the `.node` addon and `libuplink` shared library can be distributed and updated independently without recompiling from source.

---

### Quick Comparison

| | Original `uplink-nodejs` | This package `storj-uplink-nodejs` |
| --- | --- | --- |
| **Install** | Always builds from source (Go + C++ required) | Prebuilt download by default (Node.js only) |
| **Logging** | None | `UPLINK_LOG_LEVEL` + `UPLINK_LOG_FILE` env vars |
| **Platform code** | Duplicate `functions/` and `functions_win/` directories | Single `native/src/` with `#ifdef` guards |
| **Error mapping** | JavaScript `switch` in `error.js` | Native C `error_registry.c`, typed before reaching JS |
| **Library loading** | Linked at compile time | Runtime `dlopen`/`LoadLibraryA` with path search |
| **Language** | C++ (N-API with STL) | Pure C (Node-API, no C++ dependencies) |

---

## <b>Initial Set-up</b>

Node.js **v18 or higher** is required. [Download Node.js](https://nodejs.org/en/download/)

### Check Node.js Version

```sh
node -v
```

### Install TypeScript Types

```sh
npm install --save-dev @types/node
```

### Source Build Prerequisites

Required only if building from source (`UPLINK_INSTALL=source` or `hybrid`):

- **Go** >= 1.21
- **make** and a C compiler (`gcc` / `clang`)
- `python3` (required by node-gyp)
- macOS: Xcode Command Line Tools → `xcode-select --install`
- Windows: [Visual Studio Build Tools](https://visualstudio.microsoft.com/visual-cpp-build-tools/)

---

## <b>Installation Methods</b>

Supported prebuilt platforms:

| Platform | Architecture |
| --- | --- |
| Linux | x64 |
| macOS | arm64 (Apple Silicon) |
| macOS | x64 (Intel) |
| Windows | x64 |

---

### Prebuilt Install (Recommended)

Downloads a prebuilt binary — no compiler or Go toolchain needed.

**Linux / macOS**
```sh
npm install storj-uplink-nodejs
```

**Windows (PowerShell)**
```powershell
npm install storj-uplink-nodejs
```

**Windows (Command Prompt)**
```cmd
npm install storj-uplink-nodejs
```

---

### Install from Source

Compiles the native addon from source. Requires Go >= 1.21 and a C compiler.

**Linux / macOS**
```sh
UPLINK_INSTALL=source npm install storj-uplink-nodejs
```

**Windows (PowerShell)**
```powershell
$env:UPLINK_INSTALL="source"; npm install storj-uplink-nodejs
```

**Windows (Command Prompt)**
```cmd
set UPLINK_INSTALL=source && npm install storj-uplink-nodejs
```

---

### Hybrid Install

Tries prebuilt first, falls back to building from source if the prebuilt is unavailable.

**Linux / macOS**
```sh
UPLINK_INSTALL=hybrid npm install storj-uplink-nodejs
```

**Windows (PowerShell)**
```powershell
$env:UPLINK_INSTALL="hybrid"; npm install storj-uplink-nodejs
```

**Windows (Command Prompt)**
```cmd
set UPLINK_INSTALL=hybrid && npm install storj-uplink-nodejs
```

---

### Skip Native Build

Installs without any native compilation. Useful in CI environments where the binary is provided separately.

**Linux / macOS**
```sh
UPLINK_INSTALL=skip npm install storj-uplink-nodejs
```

**Windows (PowerShell)**
```powershell
$env:UPLINK_INSTALL="skip"; npm install storj-uplink-nodejs
```

**Windows (Command Prompt)**
```cmd
set UPLINK_INSTALL=skip && npm install storj-uplink-nodejs
```

---

### Verify Install

```sh
make verify-full
```

---

## <b>Installation Environment Variables</b>

| Variable | Values | Description |
| --- | --- | --- |
| `UPLINK_INSTALL` | `prebuilt` \| `hybrid` \| `source` \| `skip` | Force a specific install method |
| `UPLINK_C_DIR` | path | Path to a local `uplink-c` source directory (used with `source` or `hybrid`) |
| `VERBOSE` | `1` | Enable verbose build output (shows full compiler output) |

---

### Force Prebuilt Download

**Linux / macOS**
```sh
UPLINK_INSTALL=prebuilt npm install storj-uplink-nodejs
```

**Windows (PowerShell)**
```powershell
$env:UPLINK_INSTALL="prebuilt"; npm install storj-uplink-nodejs
```

**Windows (Command Prompt)**
```cmd
set UPLINK_INSTALL=prebuilt && npm install storj-uplink-nodejs
```

---

### Build from Source with Local uplink-c

**Linux / macOS**
```sh
UPLINK_INSTALL=source UPLINK_C_DIR=../uplink-c npm install storj-uplink-nodejs
```

**Windows (PowerShell)**
```powershell
$env:UPLINK_INSTALL="source"; $env:UPLINK_C_DIR="../uplink-c"; npm install storj-uplink-nodejs
```

**Windows (Command Prompt)**
```cmd
set UPLINK_INSTALL=source && set UPLINK_C_DIR=../uplink-c && npm install storj-uplink-nodejs
```

---

### Enable Verbose Build Output

**Linux / macOS**
```sh
VERBOSE=1 npm install storj-uplink-nodejs
```

**Windows (PowerShell)**
```powershell
$env:VERBOSE="1"; npm install storj-uplink-nodejs
```

**Windows (Command Prompt)**
```cmd
set VERBOSE=1 && npm install storj-uplink-nodejs
```

---

### Persist Install Method via `.uplinkrc`

Create a `.uplinkrc` file in your project root to avoid passing `UPLINK_INSTALL` on every command. The file should contain just the method name on one line.

Valid values: `prebuilt`, `hybrid`, `source`, `skip`

Priority order (highest to lowest):
1. `UPLINK_INSTALL` environment variable
2. `.uplinkrc` file in the project root
3. Auto-detect (defaults to `prebuilt`)

**Linux / macOS**
```sh
echo "source" > .uplinkrc
```

**Windows (PowerShell)**
```powershell
"source" | Out-File -FilePath .uplinkrc -Encoding ascii
```

**Windows (Command Prompt)**
```cmd
echo source > .uplinkrc
```

---

## <b>Storj Credential Environment Variables</b>

These variables are required for running integration tests and examples.

| Variable | Description |
| --- | --- |
| `TEST_SATELLITE` | Satellite address (e.g. `us1.storj.io:7777`) |
| `TEST_API_KEY` | API key from the Storj console |
| `TEST_PASSPHRASE` | Encryption passphrase |
| `TEST_BUCKET` | Bucket name to use in tests/examples |

---

### Set Credentials via `.env.test` File (Recommended)

Create a `.env.test` file in your project root:

```
TEST_SATELLITE=us1.storj.io:7777
TEST_API_KEY=your-api-key-from-storj-console
TEST_PASSPHRASE=your-secret-passphrase
TEST_BUCKET=my-test-bucket
```

---

### Set Credentials in Shell

**Linux / macOS**
```sh
export TEST_SATELLITE="us1.storj.io:7777"
export TEST_API_KEY="your-api-key"
export TEST_PASSPHRASE="your-passphrase"
export TEST_BUCKET="my-test-bucket"
```

**Windows (PowerShell)**
```powershell
$env:TEST_SATELLITE="us1.storj.io:7777"
$env:TEST_API_KEY="your-api-key"
$env:TEST_PASSPHRASE="your-passphrase"
$env:TEST_BUCKET="my-test-bucket"
```

**Windows (Command Prompt)**
```cmd
set TEST_SATELLITE=us1.storj.io:7777
set TEST_API_KEY=your-api-key
set TEST_PASSPHRASE=your-passphrase
set TEST_BUCKET=my-test-bucket
```

---

## <b>Logging</b>

The native addon has a built-in logger controlled entirely via environment variables. No code changes are needed — just set the variable before running your app or tests.

| Variable | Description |
| --- | --- |
| `UPLINK_LOG_LEVEL` | Controls log verbosity (`none` / `error` / `warn` / `info` / `debug` / `trace`) |
| `UPLINK_LOG_FILE` | Path to a file where logs are written (appended, no colors) |

---

### Log Levels

| Level | Numeric | Description |
| --- | --- | --- |
| `none` | 0 | Disable all logging |
| `error` | 1 | Errors only |
| `warn` | 2 | Warnings and errors |
| `info` | 3 | Informational messages *(default)* |
| `debug` | 4 | Verbose debug output |
| `trace` | 5 | Highly detailed trace output |

---

### Log Output Format

Each log line follows this format:

```
[YYYY-MM-DD HH:MM:SS] LEVEL [filename:line function()] message
```

Example:
```
[2024-01-15 10:23:45] INFO  [uplink.c:42 uplink_init()] Uplink initialized
[2024-01-15 10:23:45] DEBUG [project.c:118 open_project()] Opening project connection
[2024-01-15 10:23:46] ERROR [upload.c:87 upload_write()] Write failed: connection timeout
```

Console output goes to **stderr** and is color-coded:

| Level | Color |
| --- | --- |
| `ERROR` | Red |
| `WARN` | Yellow |
| `INFO` | Green |
| `DEBUG` | Cyan |
| `TRACE` | Gray |

---

### Disable All Logs

**Linux / macOS**
```sh
UPLINK_LOG_LEVEL=none node app.js
```

**Windows (PowerShell)**
```powershell
$env:UPLINK_LOG_LEVEL="none"; node app.js
```

**Windows (Command Prompt)**
```cmd
set UPLINK_LOG_LEVEL=none && node app.js
```

---

### Show Errors Only

**Linux / macOS**
```sh
UPLINK_LOG_LEVEL=error node app.js
```

**Windows (PowerShell)**
```powershell
$env:UPLINK_LOG_LEVEL="error"; node app.js
```

**Windows (Command Prompt)**
```cmd
set UPLINK_LOG_LEVEL=error && node app.js
```

---

### Show Warnings and Errors

**Linux / macOS**
```sh
UPLINK_LOG_LEVEL=warn node app.js
```

**Windows (PowerShell)**
```powershell
$env:UPLINK_LOG_LEVEL="warn"; node app.js
```

**Windows (Command Prompt)**
```cmd
set UPLINK_LOG_LEVEL=warn && node app.js
```

---

### Enable Debug Logging

**Linux / macOS**
```sh
UPLINK_LOG_LEVEL=debug node app.js
```

**Windows (PowerShell)**
```powershell
$env:UPLINK_LOG_LEVEL="debug"; node app.js
```

**Windows (Command Prompt)**
```cmd
set UPLINK_LOG_LEVEL=debug && node app.js
```

---

### Enable Trace Logging (Most Verbose)

**Linux / macOS**
```sh
UPLINK_LOG_LEVEL=trace node app.js
```

**Windows (PowerShell)**
```powershell
$env:UPLINK_LOG_LEVEL="trace"; node app.js
```

**Windows (Command Prompt)**
```cmd
set UPLINK_LOG_LEVEL=trace && node app.js
```

---

### Write Logs to a File

Logs are **appended** to the file. File output has no ANSI color codes. Console (stderr) output still appears alongside the file.

**Linux / macOS**
```sh
UPLINK_LOG_FILE=./uplink.log node app.js
```

**Windows (PowerShell)**
```powershell
$env:UPLINK_LOG_FILE=".\uplink.log"; node app.js
```

**Windows (Command Prompt)**
```cmd
set UPLINK_LOG_FILE=.\uplink.log && node app.js
```

---

### Write Debug Logs to a File

**Linux / macOS**
```sh
UPLINK_LOG_LEVEL=debug UPLINK_LOG_FILE=./debug.log node app.js
```

**Windows (PowerShell)**
```powershell
$env:UPLINK_LOG_LEVEL="debug"; $env:UPLINK_LOG_FILE=".\debug.log"; node app.js
```

**Windows (Command Prompt)**
```cmd
set UPLINK_LOG_LEVEL=debug && set UPLINK_LOG_FILE=.\debug.log && node app.js
```

---

### Suppress Console Logs (Redirect stderr)

**Linux / macOS**
```sh
node app.js 2>/dev/null
```

**Windows (PowerShell)**
```powershell
node app.js 2>$null
```

**Windows (Command Prompt)**
```cmd
node app.js 2>nul
```

---

### Save Console Logs to a File

**Linux / macOS**
```sh
node app.js 2>uplink-stderr.log
```

**Windows (PowerShell)**
```powershell
node app.js 2>uplink-stderr.log
```

**Windows (Command Prompt)**
```cmd
node app.js 2>uplink-stderr.log
```

---

### Save stdout and stderr to Separate Files

**Linux / macOS**
```sh
node app.js >app.log 2>uplink.log
```

**Windows (PowerShell)**
```powershell
node app.js >app.log 2>uplink.log
```

**Windows (Command Prompt)**
```cmd
node app.js >app.log 2>uplink.log
```

---

### Run Integration Tests with Debug Logging

**Linux / macOS**
```sh
UPLINK_LOG_LEVEL=debug npm run test:integration
```

**Windows (PowerShell)**
```powershell
$env:UPLINK_LOG_LEVEL="debug"; npm run test:integration
```

**Windows (Command Prompt)**
```cmd
set UPLINK_LOG_LEVEL=debug && npm run test:integration
```

---

### Run Tests with Logging Disabled

**Linux / macOS**
```sh
UPLINK_LOG_LEVEL=none npm test
```

**Windows (PowerShell)**
```powershell
$env:UPLINK_LOG_LEVEL="none"; npm test
```

**Windows (Command Prompt)**
```cmd
set UPLINK_LOG_LEVEL=none && npm test
```

---

## <b>Architecture / Flow Diagram</b>

```
Your Application (JS / TypeScript)
        ↓
storj-uplink-nodejs  (TypeScript classes — Uplink, AccessResultStruct, ProjectResultStruct, ...)
        ↓
native addon  (.node binary — pure C, Node-API)
        ↓
uplink-c  (Storj Go library, compiled as shared library)
        ↓
Storj Decentralized Storage Network
```

Class hierarchy:

```
Uplink
  ├─ requestAccessWithPassphrase(satellite, apiKey, passphrase) → AccessResultStruct
  ├─ parseAccess(accessGrant) → AccessResultStruct
  ├─ configRequestAccessWithPassphrase(config, satellite, apiKey, passphrase) → AccessResultStruct
  └─ uplinkDeriveEncryptionKey(passphrase, salt, length) → EncryptionKey

AccessResultStruct
  ├─ openProject() → ProjectResultStruct
  ├─ configOpenProject(config) → ProjectResultStruct
  ├─ share(permission, prefixes) → AccessResultStruct
  ├─ serialize() → string
  └─ overrideEncryptionKey(bucket, prefix, key)

ProjectResultStruct
  ├─ createBucket(name) → BucketInfo
  ├─ ensureBucket(name) → BucketInfo
  ├─ statBucket(name) → BucketInfo
  ├─ listBuckets(options?) → BucketInfo[]
  ├─ deleteBucket(name)
  ├─ uploadObject(bucket, key, options?) → UploadResultStruct
  ├─ downloadObject(bucket, key, options?) → DownloadResultStruct
  ├─ statObject(bucket, key) → ObjectInfo
  ├─ listObjects(bucket, options?) → ObjectInfo[]
  ├─ deleteObject(bucket, key) → ObjectInfo
  ├─ copyObject(srcBucket, srcKey, dstBucket, dstKey, options?) → ObjectInfo
  ├─ moveObject(srcBucket, srcKey, dstBucket, dstKey, options?)
  └─ close()

UploadResultStruct
  ├─ write(buffer, length) → number
  ├─ setCustomMetadata(metadata)
  ├─ commit()
  ├─ abort()
  └─ info() → ObjectInfo

DownloadResultStruct
  ├─ read(buffer, length) → ReadResult
  ├─ info() → ObjectInfo
  └─ close()

(standalone) beginMultipartUpload(projectHandle, bucket, key, options?) → MultipartUpload
  └─ MultipartUpload
       ├─ uploadPart(partNumber) → PartUploadResultStruct
       ├─ commit(options?) → ObjectInfo
       ├─ abort()
       └─ listParts(options?) → PartInfo[]

(standalone) listMultipartUploads(projectHandle, bucket, options?) → UploadInfo[]
(standalone) edgeRegisterAccess(config, accessHandle, options?) → EdgeCredentials
(standalone) edgeJoinShareUrl(baseUrl, accessKeyId, bucket, key, options?) → string
```

---

## <b>Testing</b>

Create a `.env.test` file at the module root with your Storj credentials before running integration tests.

---

### Run All Tests

```sh
npm test
```

---

### Run Unit Tests Only

No credentials needed.

```sh
npm run test:unit
```

---

### Run Integration Tests

Requires credentials in `.env.test`.

```sh
npm run test:integration
```

---

### Run C Native Tests

```sh
npm run test:c
```

---

> NOTE: Please ensure `npm install` has been run before testing.

### Tested Platforms

```
Linux (Ubuntu 22.04 LTS)
  Architecture: x64
  Node versions: 18, 20, 22

macOS (Sequoia / Sonoma)
  Architecture: arm64 (Apple Silicon M1/M2/M3)
  Node versions: 18, 20, 22

Windows 10/11
  Architecture: x64
  Node versions: 18, 20, 22
```
