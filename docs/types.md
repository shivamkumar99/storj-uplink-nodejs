# Types, Errors and Constants

## Types

All types are exported from `storj-uplink-nodejs` and defined in `src/types/index.ts`.

---

### UplinkConfig

Configuration for the Uplink client. Pass to `configRequestAccessWithPassphrase` or `configOpenProject`.

#### Fields:

| Field | Description | Type |
| --- | --- | --- |
| `userAgent` | User agent string sent with requests | `string` (optional) |
| `dialTimeoutMilliseconds` | How long to wait for a connection (ms) | `number` (optional) |
| `tempDirectory` | Directory for temporary files during downloads | `string` (optional) |

#### Usage Example

```js
const config = {
  userAgent: "MyApp/1.0",
  dialTimeoutMilliseconds: 10000,
  tempDirectory: "/tmp"
};

const access = await uplink.configRequestAccessWithPassphrase(
  config, satellite, apiKey, passphrase
);
```

---

### Permission

Defines what actions are allowed on a shared access grant.

#### Fields:

| Field | Description | Type |
| --- | --- | --- |
| `allowDownload` | Allow downloading objects | `boolean` (optional) |
| `allowUpload` | Allow uploading new objects | `boolean` (optional) |
| `allowList` | Allow listing buckets and objects | `boolean` (optional) |
| `allowDelete` | Allow deleting objects and buckets | `boolean` (optional) |
| `notBefore` | Grant is not valid before this time | `Date` (optional) |
| `notAfter` | Grant expires at this time | `Date` (optional) |

#### Usage Example

```js
const permission = {
  allowDownload: true,
  allowUpload: false,
  allowList: true,
  allowDelete: false,
  notAfter: new Date(Date.now() + 24 * 60 * 60 * 1000) // expires in 24 hours
};

const sharedAccess = await access.share(permission, [
  { bucket: "my-bucket", prefix: "photos/" }
]);
```

---

### SharePrefix

Defines a bucket prefix to be shared via `access.share()`.

#### Fields:

| Field | Description | Type |
| --- | --- | --- |
| `bucket` | Bucket name | `string` |
| `prefix` | Object key prefix (optional — share entire bucket if omitted) | `string` (optional) |

> Note: Within a bucket, the hierarchical key derivation scheme is delineated by forward slashes (`/`). Encryption information will be included in the access grant for any key sharing the same prefix up to the last slash.

#### Usage Example

```js
const prefixes = [
  { bucket: "my-bucket", prefix: "photos/" },
  { bucket: "my-bucket", prefix: "docs/" }
];

const sharedAccess = await access.share(
  { allowDownload: true, allowList: true },
  prefixes
);
```

---

### BucketInfo

Returned by bucket operations (`createBucket`, `ensureBucket`, `statBucket`, `listBuckets`).

#### Fields:

| Field | Description | Type |
| --- | --- | --- |
| `name` | Bucket name | `string` |
| `created` | Creation time (Unix timestamp in seconds) | `number` |

#### Usage Example

```js
const bucketInfo = await project.statBucket("my-bucket");
console.log(bucketInfo.name);    // "my-bucket"
console.log(bucketInfo.created); // 1700000000
```

---

### ObjectInfo

Returned by object operations (`statObject`, `listObjects`, `deleteObject`, `copyObject`, `upload.info()`, `download.info()`).

#### Fields:

| Field | Description | Type |
| --- | --- | --- |
| `key` | Object key (path) | `string` |
| `isPrefix` | Whether this is a prefix (directory placeholder) | `boolean` |
| `system` | System-managed metadata | `SystemMetadata` |
| `custom` | Custom user-defined metadata | `CustomMetadata` |

#### Usage Example

```js
const objectInfo = await project.statObject("my-bucket", "photos/vacation.jpg");
console.log(objectInfo.key);                    // "photos/vacation.jpg"
console.log(objectInfo.system.contentLength);   // 4096000
console.log(objectInfo.custom["app:title"]);    // "Vacation Photo"
```

---

### SystemMetadata

System-managed metadata for objects. Found on `ObjectInfo.system`.

#### Fields:

| Field | Description | Type |
| --- | --- | --- |
| `created` | Creation time (Unix timestamp in seconds) | `number` |
| `expires` | Expiration time (Unix timestamp in seconds, `null` if no expiry) | `number \| null` |
| `contentLength` | Object size in bytes | `number` |

---

### CustomMetadata

User-defined key-value pairs attached to an object. Found on `ObjectInfo.custom`.\
Keys should follow the convention `"appname:key"` to avoid collisions.

#### Usage Example

```js
// Setting custom metadata on upload
await upload.setCustomMetadata({
  "app:title": "Vacation Photo",
  "app:author": "Jane Doe",
  "app:tags": "travel,beach"
});

// Reading custom metadata
const info = await download.info();
console.log(info.custom["app:title"]); // "Vacation Photo"
```

---

### ListBucketsOptions

Options for `project.listBuckets()`.

#### Fields:

| Field | Description | Type |
| --- | --- | --- |
| `cursor` | Pagination cursor — list buckets after this name | `string` (optional) |

#### Usage Example

```js
// Paginate through buckets
let cursor = "";
while (true) {
  const buckets = await project.listBuckets({ cursor });
  if (buckets.length === 0) break;
  for (const b of buckets) console.log(b.name);
  cursor = buckets[buckets.length - 1].name;
}
```

---

### ListObjectsOptions

Options for `project.listObjects()`.

#### Fields:

| Field | Description | Type |
| --- | --- | --- |
| `prefix` | Filter objects by key prefix | `string` (optional) |
| `cursor` | Pagination cursor — list objects after this key | `string` (optional) |
| `recursive` | List recursively (no directory collapsing) | `boolean` (optional) |
| `system` | Include system metadata in results | `boolean` (optional) |
| `custom` | Include custom metadata in results | `boolean` (optional) |

#### Usage Example

```js
const objects = await project.listObjects("my-bucket", {
  prefix: "photos/",
  recursive: true,
  system: true,
  custom: true
});
```

---

### UploadOptions

Options for `project.uploadObject()`.

#### Fields:

| Field | Description | Type |
| --- | --- | --- |
| `expires` | When the object should expire | `Date` (optional) |

#### Usage Example

```js
const upload = await project.uploadObject("my-bucket", "temp-file.txt", {
  expires: new Date(Date.now() + 7 * 24 * 60 * 60 * 1000) // 7 days
});
```

---

### DownloadOptions

Options for `project.downloadObject()`.

#### Fields:

| Field | Description | Type |
| --- | --- | --- |
| `offset` | Starting byte offset | `number` (optional) |
| `length` | Number of bytes to download (`-1` for all remaining bytes) | `number` (optional) |

#### Usage Example

```js
// Download first 1KB only
const download = await project.downloadObject("my-bucket", "large-file.bin", {
  offset: 0,
  length: 1024
});

// Download bytes 5000-9999
const download = await project.downloadObject("my-bucket", "large-file.bin", {
  offset: 5000,
  length: 5000
});
```

---

### WriteResult

Returned by `upload.write()`.

#### Fields:

| Field | Description | Type |
| --- | --- | --- |
| `bytesWritten` | Number of bytes actually written | `number` |

---

### ReadResult

Returned by `download.read()`.

#### Fields:

| Field | Description | Type |
| --- | --- | --- |
| `bytesRead` | Number of bytes actually read | `number` |

---

### EncryptionKey

An opaque encryption key returned by `uplink.uplinkDeriveEncryptionKey()`.\
Pass to `access.overrideEncryptionKey()` for multi-tenant encryption.

---

### UploadInfo

Information about a pending multipart upload. Returned by `listMultipartUploads()`.

#### Fields:

| Field | Description | Type |
| --- | --- | --- |
| `uploadId` | Unique identifier for this multipart upload | `string` |
| `key` | Object key being uploaded | `string` |
| `isPrefix` | Whether this is a prefix | `boolean` |
| `system` | System metadata | `SystemMetadata` |
| `custom` | Custom metadata | `CustomMetadata` |

---

### PartInfo

Information about an uploaded part. Returned by `multipartUpload.listParts()`.

#### Fields:

| Field | Description | Type |
| --- | --- | --- |
| `partNumber` | Part number (1-based) | `number` |
| `size` | Size of the part in bytes | `number` |
| `modified` | When the part was last modified | `Date` |
| `etag` | ETag for the part | `string` |

---

### EdgeConfig

Configuration for Storj edge services.

#### Fields:

| Field | Description | Type |
| --- | --- | --- |
| `authServiceAddress` | Auth service address (e.g. `auth.us1.storjshare.io:7777`) | `string` |
| `certificatePem` | TLS certificate PEM (optional) | `string` (optional) |
| `insecureUnencryptedConnection` | Use unencrypted connection (not recommended) | `boolean` (optional) |

---

### EdgeCredentials

S3-compatible credentials returned by `edgeRegisterAccess()`.

#### Fields:

| Field | Description | Type |
| --- | --- | --- |
| `accessKeyId` | S3 access key ID | `string` |
| `secretKey` | S3 secret key | `string` |
| `endpoint` | S3-compatible endpoint URL | `string` |

---

### EdgeShareURLOptions

Options for `edgeJoinShareUrl()`.

#### Fields:

| Field | Description | Type |
| --- | --- | --- |
| `raw` | Serve file directly without landing page | `boolean` (optional) |

---

### EdgeRegions

Pre-configured edge service regions exported as a constant:

```js
const { EdgeRegions } = require("storj-uplink-nodejs");

// US East
EdgeRegions.US1.authService  // "auth.us1.storjshare.io:7777"
EdgeRegions.US1.linkshare    // "https://link.us1.storjshare.io"

// EU
EdgeRegions.EU1.authService  // "auth.eu1.storjshare.io:7777"
EdgeRegions.EU1.linkshare    // "https://link.eu1.storjshare.io"

// Asia Pacific
EdgeRegions.AP1.authService  // "auth.ap1.storjshare.io:7777"
EdgeRegions.AP1.linkshare    // "https://link.ap1.storjshare.io"
```

---

## Errors

All async methods throw typed JavaScript `Error` objects when something goes wrong. All Storj errors extend the base `StorjError` class and have a numeric `code` property.

### StorjError (Base Class)

#### Description:

Base class for all Storj uplink errors. Use `instanceof StorjError` to catch any Storj error.

#### Properties:

| Property | Description | Type |
| --- | --- | --- |
| `code` | Numeric error code from the native library | `number` |
| `message` | Human-readable error message | `string` |
| `details` | Additional error details from the native layer | `string` (optional) |

#### Usage Example

```js
const { StorjError, BucketNotFoundError } = require("storj-uplink-nodejs");

try {
  await project.statBucket("non-existent-bucket");
} catch (err) {
  if (err instanceof BucketNotFoundError) {
    console.log("Bucket does not exist");
  } else if (err instanceof StorjError) {
    console.log("Storj error code:", err.code, "message:", err.message);
  } else {
    throw err; // unexpected error
  }
}
```

`StorjError` is sub-categorized into the following specific error classes:

---

### General Errors

| Error Class | Code | When Thrown |
| --- | --- | --- |
| `InternalError` | `0x02` | Internal issue resolving the request |
| `CanceledError` | `0x03` | Operation was canceled |
| `InvalidHandleError` | `0x04` | Invalid or already-freed handle passed |
| `TooManyRequestsError` | `0x05` | Rate limit exceeded |
| `BandwidthLimitExceededError` | `0x06` | Project bandwidth limit exceeded |
| `StorageLimitExceededError` | `0x07` | Project storage limit exceeded |
| `SegmentsLimitExceededError` | `0x08` | Project segments limit exceeded |
| `PermissionDeniedError` | `0x09` | Access grant does not permit this action |

### Bucket Errors

| Error Class | Code | When Thrown |
| --- | --- | --- |
| `BucketNameInvalidError` | `0x10` | Bucket name is invalid |
| `BucketAlreadyExistsError` | `0x11` | Bucket already exists (from `createBucket`) |
| `BucketNotEmptyError` | `0x12` | Bucket is not empty (from `deleteBucket`) |
| `BucketNotFoundError` | `0x13` | Bucket does not exist |

### Object Errors

| Error Class | Code | When Thrown |
| --- | --- | --- |
| `ObjectKeyInvalidError` | `0x20` | Object key is invalid |
| `ObjectNotFoundError` | `0x21` | Object does not exist |
| `UploadDoneError` | `0x22` | `commit()` or `abort()` was already called |

### Edge Errors

| Error Class | Code | When Thrown |
| --- | --- | --- |
| `EdgeAuthDialFailedError` | `0x30` | Failed to connect to edge auth service |
| `EdgeRegisterAccessFailedError` | `0x31` | Failed to register access with edge service |

---

### Error Handling Patterns

#### Pattern 1: catch specific errors

```js
const {
  BucketNotFoundError,
  BucketNotEmptyError,
  ObjectNotFoundError,
  StorjError
} = require("storj-uplink-nodejs");

try {
  await project.deleteBucket("my-bucket");
} catch (err) {
  if (err instanceof BucketNotFoundError) {
    console.log("Bucket does not exist — skipping");
  } else if (err instanceof BucketNotEmptyError) {
    console.error("Cannot delete: bucket is not empty");
  } else if (err instanceof StorjError) {
    console.error("Storj error:", err.code, err.message);
  } else {
    throw err;
  }
}
```

#### Pattern 2: catch with async/await

```js
const upload = await project.uploadObject("my-bucket", "file.txt").catch(err => {
  if (err instanceof StorjError) {
    console.error("Upload failed:", err.message);
    return null;
  }
  throw err;
});

if (upload) {
  await upload.write(data, data.length);
  await upload.commit();
}
```

#### Pattern 3: always close resources

```js
const project = await access.openProject();
try {
  // ... work with project ...
} finally {
  await project.close(); // always runs, even on error
}
```

---

## Constants

Error code constants are exported from `storj-uplink-nodejs` via the `ErrorCodes` object:

```js
const { ErrorCodes } = require("storj-uplink-nodejs");

// General
ErrorCodes.INTERNAL                  // 0x02
ErrorCodes.CANCELED                  // 0x03
ErrorCodes.INVALID_HANDLE            // 0x04
ErrorCodes.TOO_MANY_REQUESTS         // 0x05
ErrorCodes.BANDWIDTH_LIMIT_EXCEEDED  // 0x06
ErrorCodes.STORAGE_LIMIT_EXCEEDED    // 0x07
ErrorCodes.SEGMENTS_LIMIT_EXCEEDED   // 0x08
ErrorCodes.PERMISSION_DENIED         // 0x09

// Bucket
ErrorCodes.BUCKET_NAME_INVALID       // 0x10
ErrorCodes.BUCKET_ALREADY_EXISTS     // 0x11
ErrorCodes.BUCKET_NOT_EMPTY          // 0x12
ErrorCodes.BUCKET_NOT_FOUND          // 0x13

// Object
ErrorCodes.OBJECT_KEY_INVALID        // 0x20
ErrorCodes.OBJECT_NOT_FOUND          // 0x21
ErrorCodes.UPLOAD_DONE               // 0x22

// Edge
ErrorCodes.EDGE_AUTH_DIAL_FAILED          // 0x30
ErrorCodes.EDGE_REGISTER_ACCESS_FAILED    // 0x31
```

> Note: You can view the uplink-c documentation [here](https://pkg.go.dev/storj.io/uplink).
