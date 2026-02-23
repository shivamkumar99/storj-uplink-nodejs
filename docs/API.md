# API Reference

Quick reference for all exported functions and classes in `storj-uplink-nodejs`.

For detailed usage, arguments, and examples see [Binding Functions](/library.md).

---

## Uplink (class)

Entry point. Import and instantiate once.

```js
const { Uplink } = require("storj-uplink-nodejs");
const uplink = new Uplink();
```

| Method | Returns | Description |
| --- | --- | --- |
| `requestAccessWithPassphrase(satellite, apiKey, passphrase)` | `Promise<AccessResultStruct>` | Request access using satellite, API key, and passphrase |
| `configRequestAccessWithPassphrase(config, satellite, apiKey, passphrase)` | `Promise<AccessResultStruct>` | Same, with a custom config object |
| `parseAccess(accessGrant)` | `Promise<AccessResultStruct>` | Parse a serialized access grant string |
| `uplinkDeriveEncryptionKey(passphrase, salt, length)` | `Promise<EncryptionKey>` | Derive a salted encryption key |

---

## AccessResultStruct (class)

Obtained from `Uplink` methods. Used to open projects or share access.

| Method | Returns | Description |
| --- | --- | --- |
| `openProject()` | `Promise<ProjectResultStruct>` | Open the Storj project |
| `configOpenProject(config)` | `Promise<ProjectResultStruct>` | Open the project with custom config |
| `share(permission, prefixes)` | `Promise<AccessResultStruct>` | Create a restricted access grant |
| `serialize()` | `Promise<string>` | Serialize the access grant to a string |
| `overrideEncryptionKey(bucket, prefix, key)` | `Promise<void>` | Override the encryption key for a prefix |

---

## ProjectResultStruct (class)

Obtained from `AccessResultStruct.openProject()`. Main interface for buckets and objects.

### Bucket Operations

| Method | Returns | Description |
| --- | --- | --- |
| `createBucket(name)` | `Promise<BucketInfo>` | Create a new bucket |
| `ensureBucket(name)` | `Promise<BucketInfo>` | Create bucket or succeed if exists |
| `statBucket(name)` | `Promise<BucketInfo>` | Get bucket information |
| `listBuckets(options?)` | `Promise<BucketInfo[]>` | List all buckets |
| `deleteBucket(name)` | `Promise<void>` | Delete a bucket (must be empty) |

### Object Operations

| Method | Returns | Description |
| --- | --- | --- |
| `uploadObject(bucket, key, options?)` | `Promise<UploadResultStruct>` | Start uploading an object |
| `downloadObject(bucket, key, options?)` | `Promise<DownloadResultStruct>` | Start downloading an object |
| `statObject(bucket, key)` | `Promise<ObjectInfo>` | Get object information |
| `listObjects(bucket, options?)` | `Promise<ObjectInfo[]>` | List objects in a bucket |
| `deleteObject(bucket, key)` | `Promise<ObjectInfo>` | Delete an object |
| `copyObject(srcBucket, srcKey, dstBucket, dstKey, options?)` | `Promise<ObjectInfo>` | Copy an object |
| `moveObject(srcBucket, srcKey, dstBucket, dstKey, options?)` | `Promise<void>` | Move / rename an object |

### Lifecycle

| Method | Returns | Description |
| --- | --- | --- |
| `close()` | `Promise<void>` | Close the project and release resources |

---

## UploadResultStruct (class)

Obtained from `ProjectResultStruct.uploadObject()`.

| Method | Returns | Description |
| --- | --- | --- |
| `write(buffer, length)` | `Promise<number>` | Write bytes to the upload stream |
| `setCustomMetadata(metadata)` | `Promise<void>` | Set custom metadata (before commit) |
| `info()` | `Promise<ObjectInfo>` | Get info about the in-progress upload |
| `commit()` | `Promise<void>` | Finalize the upload |
| `abort()` | `Promise<void>` | Abort the upload, discard data |

---

## DownloadResultStruct (class)

Obtained from `ProjectResultStruct.downloadObject()`.

| Method | Returns | Description |
| --- | --- | --- |
| `read(buffer, length)` | `Promise<ReadResult>` | Read bytes from the download stream |
| `info()` | `Promise<ObjectInfo>` | Get object info (includes content length) |
| `close()` | `Promise<void>` | Close the download stream |

---

## Multipart Upload

Standalone functions for multipart uploads. Require a native project handle (`project._nativeHandle`).

| Function | Returns | Description |
| --- | --- | --- |
| `beginMultipartUpload(projectHandle, bucket, key, options?)` | `Promise<MultipartUpload>` | Begin a new multipart upload |
| `listMultipartUploads(projectHandle, bucket, options?)` | `Promise<UploadInfo[]>` | List pending multipart uploads |

### MultipartUpload (class)

| Method | Returns | Description |
| --- | --- | --- |
| `uploadPart(partNumber)` | `Promise<PartUploadResultStruct>` | Start uploading a part (1-10000) |
| `commit(options?)` | `Promise<ObjectInfo>` | Finalize all parts into one object |
| `abort()` | `Promise<void>` | Abort and discard all parts |
| `listParts(options?)` | `Promise<PartInfo[]>` | List uploaded parts |

### PartUploadResultStruct (class)

| Method | Returns | Description |
| --- | --- | --- |
| `write(buffer, length)` | `Promise<number>` | Write bytes to the part |
| `commit()` | `Promise<void>` | Commit the part |
| `abort()` | `Promise<void>` | Abort the part |
| `setEtag(etag)` | `Promise<void>` | Set ETag for the part |
| `info()` | `Promise<PartInfo>` | Get part info |

---

## Edge Services

Standalone functions for Storj edge / linkshare services.

| Function | Returns | Description |
| --- | --- | --- |
| `edgeRegisterAccess(config, accessHandle, options?)` | `Promise<EdgeCredentials>` | Get S3-compatible credentials |
| `edgeJoinShareUrl(baseUrl, accessKeyId, bucket, key, options?)` | `Promise<string>` | Generate a linkshare URL |

### EdgeRegions (constant)

Pre-configured regions:

```js
const { EdgeRegions } = require("storj-uplink-nodejs");
EdgeRegions.US1  // { authService: "auth.us1.storjshare.io:7777", linkshare: "https://link.us1.storjshare.io" }
EdgeRegions.EU1  // { authService: "auth.eu1.storjshare.io:7777", linkshare: "https://link.eu1.storjshare.io" }
EdgeRegions.AP1  // { authService: "auth.ap1.storjshare.io:7777", linkshare: "https://link.ap1.storjshare.io" }
```

---

## Error Classes

All errors extend `StorjError`. Import and use with `instanceof`.

```js
const {
  StorjError,
  InternalError,
  CanceledError,
  PermissionDeniedError,
  BandwidthLimitExceededError,
  StorageLimitExceededError,
  BucketNameInvalidError,
  BucketAlreadyExistsError,
  BucketNotEmptyError,
  BucketNotFoundError,
  ObjectKeyInvalidError,
  ObjectNotFoundError,
  UploadDoneError,
  EdgeAuthDialFailedError,
  EdgeRegisterAccessFailedError
} = require("storj-uplink-nodejs");
```

See [Types, Errors and Constants](/types.md) for full details.

---

## Types

All TypeScript types/interfaces exported from `storj-uplink-nodejs`:

| Type | Description |
| --- | --- |
| `UplinkConfig` | Config for Uplink client |
| `Permission` | Permissions for `access.share()` |
| `SharePrefix` | Bucket prefix for sharing |
| `BucketInfo` | Bucket name and creation time |
| `ObjectInfo` | Object key, size, metadata |
| `SystemMetadata` | System-managed metadata (created, expires, contentLength) |
| `CustomMetadata` | User-defined key-value metadata |
| `ListBucketsOptions` | Options for `listBuckets()` |
| `ListObjectsOptions` | Options for `listObjects()` |
| `UploadOptions` | Options for `uploadObject()` (expires) |
| `DownloadOptions` | Options for `downloadObject()` (offset, length) |
| `ReadResult` | Result of `download.read()` |
| `EncryptionKey` | Opaque key from `uplinkDeriveEncryptionKey()` |
| `UploadInfo` | Pending multipart upload info |
| `PartInfo` | Multipart part info |
| `EdgeConfig` | Edge auth service config |
| `EdgeCredentials` | S3-compatible credentials |
| `EdgeShareURLOptions` | Options for `edgeJoinShareUrl()` |

See [Types, Errors and Constants](/types.md) for field definitions.
