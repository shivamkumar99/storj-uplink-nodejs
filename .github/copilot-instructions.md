# GitHub Copilot Instructions for uplink-nodejs-v1

## Project Context

This is a Node.js native binding for Storj's uplink-c library. The binding uses **pure C** (not C++) with Node-API (N-API).

## Critical Rules

### 1. Language Requirements

**Native Code (native/ folder):**
- Use ONLY pure C, never C++
- Use `node_api.h`, never `napi.h`
- File extensions: `.c` and `.h` only

**TypeScript Code (src/ folder):**
- Strict TypeScript, no `any`
- All async operations return Promises
- Follow existing uplink-nodejs class names exactly

### 2. API Class Structure

```typescript
// Entry point
class Uplink {
    requestAccessWithPassphrase(satellite, apiKey, passphrase): Promise<AccessResultStruct>
    parseAccess(accessGrant): Promise<AccessResultStruct>
    configRequestAccessWithPassphrase(config, satellite, apiKey, passphrase): Promise<AccessResultStruct>
    uplinkDeriveEncryptionKey(passphrase, salt, length): Promise<EncryptionKey>
}

// Access operations
class AccessResultStruct {
    openProject(): Promise<ProjectResultStruct>
    configOpenProject(config): Promise<ProjectResultStruct>
    share(permission, prefixes): Promise<AccessResultStruct>
    serialize(): Promise<string>
    overrideEncryptionKey(bucket, prefix, key): Promise<void>
}

// Project operations
class ProjectResultStruct {
    close(): Promise<void>
    createBucket(name): Promise<BucketInfo>
    ensureBucket(name): Promise<BucketInfo>
    deleteBucket(name): Promise<void>
    statBucket(name): Promise<BucketInfo>
    listBuckets(options?): Promise<BucketInfo[]>
    uploadObject(bucket, key, options?): Promise<UploadResultStruct>
    downloadObject(bucket, key, options?): Promise<DownloadResultStruct>
    statObject(bucket, key): Promise<ObjectInfo>
    listObjects(bucket, options?): Promise<ObjectInfo[]>
    deleteObject(bucket, key): Promise<void>
    copyObject(srcBucket, srcKey, dstBucket, dstKey): Promise<ObjectInfo>
    moveObject(srcBucket, srcKey, dstBucket, dstKey): Promise<void>
}

// Upload operations
class UploadResultStruct {
    write(buffer, length): Promise<number>
    setCustomMetadata(metadata): Promise<void>
    commit(): Promise<void>
    abort(): Promise<void>
    info(): Promise<ObjectInfo>
}

// Download operations
class DownloadResultStruct {
    read(buffer, length): Promise<number>
    info(): Promise<ObjectInfo>
    close(): Promise<void>
}
```

### 3. Naming Conventions

| Context | Convention | Example |
|---------|------------|---------|
| C functions | snake_case | `parse_access`, `create_bucket` |
| C macros | UPPER_SNAKE | `LOG_ERROR`, `HANDLE_TYPE_ACCESS` |
| C structs | PascalCase | `ParseAccessData`, `HandleEntry` |
| TS classes | PascalCase | `AccessResultStruct`, `ProjectResultStruct` |
| TS methods | camelCase | `parseAccess`, `openProject` |
| TS interfaces | IPascalCase | `IBucketInfo`, `IObjectInfo` |

### 4. C Code Pattern

Always use this async pattern for uplink-c calls:

```c
#include <node_api.h>
#include "uplink.h"

typedef struct {
    char* param;
    UplinkXxxResult result;
    napi_deferred deferred;
    napi_async_work work;
} XxxData;

static void xxx_execute(napi_env env, void* data) {
    (void)env;
    XxxData* d = (XxxData*)data;
    d->result = uplink_xxx(d->param);
}

static void xxx_complete(napi_env env, napi_status status, void* data) {
    XxxData* d = (XxxData*)data;
    if (d->result.error) {
        napi_value err = error_to_js(env, d->result.error);
        napi_reject_deferred(env, d->deferred, err);
        uplink_free_error(d->result.error);
    } else {
        napi_value val = result_to_js(env, d->result);
        napi_resolve_deferred(env, d->deferred, val);
    }
    free(d->param);
    napi_delete_async_work(env, d->work);
    free(d);
}

napi_value xxx(napi_env env, napi_callback_info info) {
    // 1. Extract arguments
    // 2. Validate inputs
    // 3. Allocate work data
    // 4. Create promise
    // 5. Queue async work
    // 6. Return promise
}
```

### 5. File Organization

```
native/src/common/     → logger.c, handle_helpers.c, string_helpers.c
native/src/access/     → access_ops.c
native/src/project/    → project_ops.c
native/src/bucket/     → bucket_ops.c
native/src/upload/     → upload_ops.c
native/src/download/   → download_ops.c

src/uplink.ts          → Uplink class
src/access.ts          → AccessResultStruct class
src/project.ts         → ProjectResultStruct class
src/upload.ts          → UploadResultStruct class
src/download.ts        → DownloadResultStruct class
src/types/index.ts     → All interfaces and types
```

### 6. Required Patterns

**Always include logging:**
```c
LOG_DEBUG("Function called with: %s", param);
LOG_ERROR("Failed: %s", error->message);
LOG_INFO("Success: handle=%zu", handle);
```

**Always validate inputs:**
```typescript
if (!bucketName || bucketName.length < 3) {
    throw new TypeError('Bucket name must be at least 3 characters');
}
```

**Always clean up memory:**
```c
uplink_free_access_result(result);
uplink_free_error(error);
free(allocated_string);
```

### 7. Do NOT Generate

- C++ code in native/ folder
- Code using `napi.h` instead of `node_api.h`
- Synchronous blocking calls to uplink-c
- Callback-based APIs (use Promises)
- Code with `any` type in TypeScript
- Functions without logging
- Code without input validation

### 8. Test Status Reporting

After **every implementation task** (file edit, feature, fix, refactor), you **must** run the test suite and report the status to the user in the following format:

```
## 🧪 Test Status
- **Command**: `npx jest --no-coverage --testPathIgnorePatterns='/test/memory/' --testPathIgnorePatterns='/test/benchmarks/' --testPathIgnorePatterns='/test/install/'`
- **Result**: ✅ PASS / ❌ FAIL
- **Test Suites**: X passed, Y failed, Z total
- **Tests**: X passed, Y failed, Z skipped, W total
- **Failures** (if any):
  - `test/path/file.test.ts` > test name — error summary
```

**Rules:**
- Always run tests after modifying any `.c`, `.h`, `.ts` file
- If the prebuilt binary is stale, copy `build/Release/uplink_native.node` to `native/prebuilds/darwin-arm64/uplink_native.node` before running tests
- Exclude memory tests (`/test/memory/`), benchmark tests (`/test/benchmarks/`), and install tests (`/test/install/`) from routine runs — they are slow
- If tests fail, fix the issue before moving to the next task
- Report the status even when all tests pass — the user needs to see confirmation

### 9. Iterator Architecture (Sprint 17+)

**All list operations** use the iterator pattern — **never** loop in native C:

```
Native exposes 5 thin wrappers per iterator type:
  - *Create  → one uplink-c call → returns iterator handle
  - *Next    → one uplink-c call → returns boolean
  - *Item    → one uplink-c call → returns one item
  - *Err     → one uplink-c call → returns error or null (resolves, not rejects)
  - *Free    → one uplink-c call → frees iterator

JavaScript drives the loop:
  const iter = await native.listXxxCreate(handle, options);
  try {
    while (await native.xxxIteratorNext(iter)) {
      items.push(await native.xxxIteratorItem(iter));
    }
    const err = await native.xxxIteratorErr(iter);
    if (err) throw err;
  } finally {
    await native.freeXxxIterator(iter);
  }
```

**Iterator types** (defined in `handle_helpers.h`):
- `HANDLE_TYPE_BUCKET_ITERATOR` — bucket listing
- `HANDLE_TYPE_OBJECT_ITERATOR` — object listing
- `HANDLE_TYPE_PART_ITERATOR` — multipart part listing
- `HANDLE_TYPE_UPLOAD_ITERATOR` — pending upload listing
