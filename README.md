# <b>storj-uplink-nodejs binding</b>

#### *Node.js native bindings for Storj's uplink-c library — v0.1.0-beta.13*

A modern, TypeScript-first binding using pure C + Node-API (N-API) for connecting to the Storj decentralized cloud storage network. Prebuilt binaries are available for macOS, Linux, and Windows — no Go toolchain required for most installs.

---

## <b>Documentation</b>

| Document | Description |
| --- | --- |
| [API Reference](docs/API.md) | Quick-reference tables for all classes, methods, and standalone functions |
| [Binding Functions](docs/library.md) | Full reference for every binding function with arguments and examples |
| [Types, Errors & Constants](docs/types.md) | All TypeScript types, error class hierarchy, and error codes |
| [Tutorial](docs/tutorial.md) | Step-by-step guide: connect → upload → download → cleanup |
| [Examples](docs/EXAMPLES.md) | How to run the 8 example scripts in the `example/` directory |
| [CI / Continuous Integration](docs/CI.md) | All GitHub Actions workflows, test commands, and build matrix |

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
