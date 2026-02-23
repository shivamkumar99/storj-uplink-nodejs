# Binding Functions

## Include uplink in library

```js
// CommonJS
const { Uplink } = require("storj-uplink-nodejs");
const uplink = new Uplink();
```

```ts
// TypeScript / ESM
import { Uplink } from "storj-uplink-nodejs";
const uplink = new Uplink();
```

* Create an instance of the `Uplink` class to call all uplink functions.

> NOTE: All binding functions are **asynchronous** and return **Promises**.

---

## Uplink Class Functions

---

## requestAccessWithPassphrase(satellite, apiKey, passphrase)

### Description:

Requests a new access grant from a Storj satellite using a passphrase.\
No pre-requisites required. This is the most common way to connect to Storj.\
Returns an `AccessResultStruct` that can be used to open a project or share access.

An access grant is an internally serialized structure comprising an API key, a set of encryption key information, and the satellite address responsible for the metadata.

### Arguments:

| arguments | Description | Type |
| --- | --- | --- |
| `satellite` | Storj V3 satellite address (e.g. `us1.storj.io:7777`) | `string` |
| `apiKey` | Storj API key from the console | `string` |
| `passphrase` | Encryption passphrase — keep this secret! | `string` |

### Usage Example:

```js
const satellite = "us1.storj.io:7777";
const apiKey = "change-me-to-desired-api-key";
const passphrase = "change-me-to-desired-passphrase";

// Using .then()
uplink.requestAccessWithPassphrase(satellite, apiKey, passphrase).then(access => {
  // ...use access...
}).catch((err) => {
  console.error(err);
});

// OR using async/await
const access = await uplink.requestAccessWithPassphrase(satellite, apiKey, passphrase);
```

---

## configRequestAccessWithPassphrase(config, satellite, apiKey, passphrase)

### Description:

Requests a new access grant with a custom config object.\
Useful when you need to set a custom user agent, dial timeout, or temp directory.\
Returns an `AccessResultStruct` on success.

### Arguments:

| arguments | Description | Type |
| --- | --- | --- |
| `config` | Uplink configuration object | `object` |
| `satellite` | Storj V3 satellite address | `string` |
| `apiKey` | Storj API key | `string` |
| `passphrase` | Encryption passphrase | `string` |

### Usage Example:

```js
const config = {
  userAgent: "MyApp/1.0",
  dialTimeoutMilliseconds: 10000,
  tempDirectory: "/tmp"
};
const satellite = "us1.storj.io:7777";
const apiKey = "change-me-to-desired-api-key";
const passphrase = "change-me-to-desired-passphrase";

uplink.configRequestAccessWithPassphrase(config, satellite, apiKey, passphrase).then(access => {
  // ...use access...
}).catch((err) => {
  console.error(err);
});
```

---

## parseAccess(accessGrant)

### Description:

Parses a serialized access grant string.\
This is the main way to instantiate an access grant that was previously serialized (e.g. shared with another user).\
No pre-requisites required.\
Returns an `AccessResultStruct` on success.

### Arguments:

| arguments | Description | Type |
| --- | --- | --- |
| `accessGrant` | Serialized access grant string (returned by `access.serialize()`) | `string` |

### Usage Example:

```js
const accessGrant = "1J5F2Kxxxxxxxxxxxxxxxxxxxxxxxx"; // serialized access string

uplink.parseAccess(accessGrant).then(async (access) => {
  // ...use access...
}).catch((err) => {
  console.error(err);
});
```

---

## uplinkDeriveEncryptionKey(passphrase, salt, length)

### Description:

Derives a salted encryption key from a passphrase using the given salt.\
Useful for multi-tenancy: derive per-user encryption keys in a single shared bucket.\
Returns an `EncryptionKey` object.

### Arguments:

| arguments | Description | Type |
| --- | --- | --- |
| `passphrase` | Any passphrase string | `string` |
| `salt` | Salt as a byte array | `number[]` |
| `length` | Length of key to derive | `number` |

### Usage Example:

```js
const encryptionKey = await uplink.uplinkDeriveEncryptionKey("my-passphrase", [4, 5, 6, 7], 32)
  .catch((err) => {
    console.error(err);
  });
```

---

> NOTE: Following functions require an `AccessResultStruct` object (returned by `requestAccessWithPassphrase`, `parseAccess`, or `configRequestAccessWithPassphrase`).

---

## share(permission, prefixes)

### Description:

Creates a new access grant with restricted permissions.\
`parseAccess` or `requestAccessWithPassphrase` is required as a pre-requisite.\
Permissions will be applied to the specified prefixes.\
Returns a new `AccessResultStruct` with the restricted access.

### Arguments:

| arguments | Description | Type |
| --- | --- | --- |
| `permission` | Permission object defining allowed actions | `object` |
| `prefixes` | Array of `SharePrefix` objects defining what to share | `SharePrefix[]` |

### Usage Example:

```js
const permission = {
  allowDownload: true,
  allowUpload: false,
  allowList: true,
  allowDelete: false
};

const prefixes = [
  { bucket: "change-me-to-desired-bucket-name", prefix: "photos/" }
];

await access.share(permission, prefixes).then(async (sharedAccess) => {
  // serialize and share
  const serialized = await sharedAccess.serialize();
  console.log("Share this grant:", serialized);
}).catch((err) => {
  console.error(err);
});
```

---

## serialize()

### Description:

Serializes the access grant into a string that can be shared or stored.\
`requestAccessWithPassphrase`, `parseAccess`, or `share` is required as a pre-requisite.\
Returns a serialized access grant string.

### Usage Example:

```js
await access.serialize().then((serializedString) => {
  console.log("Serialized access:", serializedString);
}).catch((err) => {
  console.error(err);
});
```

---

## openProject()

### Description:

Opens a Storj project using the access grant.\
`requestAccessWithPassphrase` or `configRequestAccessWithPassphrase` is required as a pre-requisite.\
Returns a `ProjectResultStruct` that allows managing buckets and objects.

### Usage Example:

```js
access.openProject().then(async (project) => {
  // ...use project...
}).catch((err) => {
  console.error(err);
});
```

---

## configOpenProject(config)

### Description:

Opens a project with a custom configuration.\
`requestAccessWithPassphrase` or similar is required as a pre-requisite.\
Returns a `ProjectResultStruct`.

### Arguments:

| arguments | Description | Type |
| --- | --- | --- |
| `config` | Uplink configuration object | `object` |

### Usage Example:

```js
const config = { userAgent: "MyApp/1.0" };
access.configOpenProject(config).then(async (project) => {
  // ...use project...
}).catch((err) => {
  console.error(err);
});
```

---

## overrideEncryptionKey(bucket, prefix, encryptionKey)

### Description:

Overrides the root encryption key for the specified prefix in a bucket.\
Useful for multi-tenancy: allows per-user encryption in a single shared bucket.\
`parseAccess` is required as a pre-requisite.

### Arguments:

| arguments | Description | Type |
| --- | --- | --- |
| `bucket` | Storj bucket name | `string` |
| `prefix` | Object key prefix | `string` |
| `encryptionKey` | EncryptionKey object from `uplinkDeriveEncryptionKey` | `object` |

### Usage Example:

```js
const encryptionKey = await uplink.uplinkDeriveEncryptionKey("user-passphrase", [1, 2, 3], 32);
await access.overrideEncryptionKey("my-bucket", "user-123/", encryptionKey)
  .catch((err) => {
    console.error(err);
  });
```

---

> NOTE: Following functions require a `ProjectResultStruct` object (returned by `openProject` or `configOpenProject`).

---

## close()

### Description:

Closes the project and releases resources.\
`openProject` or `configOpenProject` is required as a pre-requisite.\
Always call this when done, preferably in a `finally` block.

### Usage Example:

```js
await project.close().then(() => {
  console.log("Project closed");
}).catch((err) => {
  console.error(err);
});
```

---

## statBucket(bucketName)

### Description:

Returns information about a bucket.\
`openProject` is required as a pre-requisite.\
Returns a `BucketInfo` object with the bucket's name and creation time.

### Arguments:

| arguments | Description | Type |
| --- | --- | --- |
| `bucketName` | Storj bucket name | `string` |

### Usage Example:

```js
const bucketName = "change-me-to-desired-bucket-name";
await project.statBucket(bucketName).then((bucketInfo) => {
  console.log("Bucket:", bucketInfo.name, "created:", bucketInfo.created);
}).catch((err) => {
  console.error(err);
});
```

---

## ensureBucket(bucketName)

### Description:

Creates a new bucket, or silently succeeds if the bucket already exists.\
`openProject` is required as a pre-requisite.\
Returns a `BucketInfo` object.

### Arguments:

| arguments | Description | Type |
| --- | --- | --- |
| `bucketName` | Bucket name on Storj V3 network | `string` |

### Usage Example:

```js
const bucketName = "change-me-to-desired-bucket-name";
await project.ensureBucket(bucketName).then((bucketInfo) => {
  console.log("Bucket ready:", bucketInfo.name);
}).catch((err) => {
  console.error(err);
});
```

---

## createBucket(bucketName)

### Description:

Creates a new bucket.\
When the bucket already exists, returns the existing bucket info and throws `BucketAlreadyExistsError`.\
`openProject` is required as a pre-requisite.\
Returns a `BucketInfo` object.

### Arguments:

| arguments | Description | Type |
| --- | --- | --- |
| `bucketName` | Bucket name to create | `string` |

### Usage Example:

```js
const bucketName = "change-me-to-desired-bucket-name";
await project.createBucket(bucketName).then((bucketInfo) => {
  console.log("Created bucket:", bucketInfo.name);
}).catch((err) => {
  console.error(err);
});
```

---

## deleteBucket(bucketName)

### Description:

Deletes a bucket.\
When the bucket is not empty, throws `BucketNotEmptyError`.\
`openProject` is required as a pre-requisite.

### Arguments:

| arguments | Description | Type |
| --- | --- | --- |
| `bucketName` | Bucket name to delete | `string` |

### Usage Example:

```js
const bucketName = "change-me-to-desired-bucket-name";
await project.deleteBucket(bucketName).then(() => {
  console.log("Bucket deleted");
}).catch((err) => {
  console.error(err);
});
```

---

## listBuckets(options?)

### Description:

Lists all buckets in the project.\
`openProject` is required as a pre-requisite.\
Returns an array of `BucketInfo` objects.

### Arguments:

| arguments | Description | Type |
| --- | --- | --- |
| `options` | Optional listing options (cursor for pagination) | `ListBucketsOptions` (optional) |

### Usage Example:

```js
// List all buckets (no options)
const buckets = await project.listBuckets();
for (const bucket of buckets) {
  console.log(bucket.name);
}

// With pagination cursor
const buckets = await project.listBuckets({ cursor: "my-last-bucket" });
```

---

## statObject(bucketName, objectKey)

### Description:

Returns information about an object at the specified key.\
`openProject` is required as a pre-requisite.\
Returns an `ObjectInfo` object.

### Arguments:

| arguments | Description | Type |
| --- | --- | --- |
| `bucketName` | Bucket name on Storj V3 network | `string` |
| `objectKey` | Object key (path) on Storj V3 network | `string` |

### Usage Example:

```js
const bucketName = "change-me-to-desired-bucket-name";
const objectKey = "path/to/my-file.txt";
await project.statObject(bucketName, objectKey).then((objectInfo) => {
  console.log("Object key:", objectInfo.key);
  console.log("Size:", objectInfo.system.contentLength);
}).catch((err) => {
  console.error(err);
});
```

---

## listObjects(bucketName, options?)

### Description:

Lists objects in a bucket.\
`openProject` is required as a pre-requisite.\
Returns an array of `ObjectInfo` objects.

### Arguments:

| arguments | Description | Type |
| --- | --- | --- |
| `bucketName` | Bucket name on Storj V3 network | `string` |
| `options` | Optional listing options | `ListObjectsOptions` (optional) |

### Usage Example:

```js
const bucketName = "change-me-to-desired-bucket-name";

// List all objects
const objects = await project.listObjects(bucketName);
for (const obj of objects) {
  console.log(obj.key);
}

// With options
const objects = await project.listObjects(bucketName, {
  prefix: "photos/",
  recursive: true,
  system: true,
  custom: true
}).catch((err) => {
  console.error(err);
});
```

---

## uploadObject(bucketName, objectKey, options?)

### Description:

Starts an upload to the specified key.\
`openProject` is required as a pre-requisite.\
Returns an `UploadResultStruct` that must be written to and then committed (or aborted).

### Arguments:

| arguments | Description | Type |
| --- | --- | --- |
| `bucketName` | Bucket name on Storj V3 network | `string` |
| `objectKey` | Object key (path) to upload to | `string` |
| `options` | Optional upload options (e.g. expiry) | `UploadOptions` (optional) |

### Usage Example:

```js
const bucketName = "change-me-to-desired-bucket-name";
const objectKey = "path/to/my-file.txt";

await project.uploadObject(bucketName, objectKey).then(async (upload) => {
  const data = Buffer.from("Hello, Storj!");
  await upload.write(data, data.length);
  await upload.commit();
}).catch((err) => {
  console.error(err);
});

// With expiry
const upload = await project.uploadObject(bucketName, objectKey, {
  expires: new Date(Date.now() + 7 * 24 * 60 * 60 * 1000) // 7 days
});
```

---

## downloadObject(bucketName, objectKey, options?)

### Description:

Starts a download from the specified key.\
`openProject` is required as a pre-requisite.\
Returns a `DownloadResultStruct` that must be read from and then closed.

### Arguments:

| arguments | Description | Type |
| --- | --- | --- |
| `bucketName` | Bucket name on Storj V3 network | `string` |
| `objectKey` | Object key (path) to download | `string` |
| `options` | Optional download options (offset, length) | `DownloadOptions` (optional) |

### Usage Example:

```js
const bucketName = "change-me-to-desired-bucket-name";
const objectKey = "path/to/my-file.txt";

await project.downloadObject(bucketName, objectKey).then(async (download) => {
  const info = await download.info();
  const size = info.system.contentLength;
  const buffer = Buffer.alloc(size);
  const result = await download.read(buffer, size);
  await download.close();
  console.log("Downloaded bytes:", result.bytesRead);
}).catch((err) => {
  console.error(err);
});

// With byte-range (partial download)
const download = await project.downloadObject(bucketName, objectKey, {
  offset: 0,
  length: 1024 // first 1KB only
});
```

---

## deleteObject(bucketName, objectKey)

### Description:

Deletes an object at the specified key.\
`openProject` is required as a pre-requisite.\
Returns an `ObjectInfo` of the deleted object.

### Arguments:

| arguments | Description | Type |
| --- | --- | --- |
| `bucketName` | Bucket name on Storj V3 network | `string` |
| `objectKey` | Object key (path) to delete | `string` |

### Usage Example:

```js
const bucketName = "change-me-to-desired-bucket-name";
const objectKey = "path/to/my-file.txt";
await project.deleteObject(bucketName, objectKey).then((objectInfo) => {
  console.log("Deleted:", objectInfo.key);
}).catch((err) => {
  console.error(err);
});
```

---

## copyObject(srcBucket, srcKey, dstBucket, dstKey, options?)

### Description:

Copies an object from one location to another.\
`openProject` is required as a pre-requisite.\
Returns an `ObjectInfo` of the new object.

### Arguments:

| arguments | Description | Type |
| --- | --- | --- |
| `srcBucket` | Source bucket name | `string` |
| `srcKey` | Source object key | `string` |
| `dstBucket` | Destination bucket name | `string` |
| `dstKey` | Destination object key | `string` |
| `options` | Optional copy options (expiry) | `CopyObjectOptions` (optional) |

### Usage Example:

```js
await project.copyObject("src-bucket", "original.txt", "dst-bucket", "copy.txt")
  .then((objectInfo) => {
    console.log("Copied to:", objectInfo.key);
  }).catch((err) => {
    console.error(err);
  });
```

---

## moveObject(srcBucket, srcKey, dstBucket, dstKey, options?)

### Description:

Moves (renames) an object from one location to another.\
`openProject` is required as a pre-requisite.

### Arguments:

| arguments | Description | Type |
| --- | --- | --- |
| `srcBucket` | Source bucket name | `string` |
| `srcKey` | Source object key | `string` |
| `dstBucket` | Destination bucket name | `string` |
| `dstKey` | Destination object key | `string` |
| `options` | Optional move options | `MoveObjectOptions` (optional) |

### Usage Example:

```js
await project.moveObject("my-bucket", "old-name.txt", "my-bucket", "new-name.txt")
  .catch((err) => {
    console.error(err);
  });
```

---

> NOTE: Following functions require an `UploadResultStruct` object (returned by `uploadObject`).

---

## setCustomMetadata(metadata)

### Description:

Sets custom metadata on an object being uploaded.\
`uploadObject` is required as a pre-requisite.\
Must be called before `commit()`.

### Arguments:

| arguments | Description | Type |
| --- | --- | --- |
| `metadata` | Custom key-value metadata object | `CustomMetadata` |

### Usage Example:

```js
const upload = await project.uploadObject("my-bucket", "photo.jpg");
const data = Buffer.from(/* file data */);
await upload.write(data, data.length);

await upload.setCustomMetadata({
  "app:title": "Vacation Photo",
  "app:author": "Jane Doe"
}).catch((err) => {
  console.error(err);
});

await upload.commit();
```

---

## write(buffer, length)

### Description:

Uploads bytes from the buffer to the object's data stream.\
`uploadObject` is required as a pre-requisite.\
Returns the number of bytes written.

### Arguments:

| arguments | Description | Type |
| --- | --- | --- |
| `buffer` | Buffer containing data to write | `Buffer` |
| `length` | Number of bytes to write from the buffer | `number` |

### Usage Example:

```js
const BUFFER_SIZE = 256 * 1024; // 256 KB
const buffer = Buffer.alloc(BUFFER_SIZE);
// ... fill buffer with data ...

await upload.write(buffer, buffer.length).then((bytesWritten) => {
  console.log("Bytes written:", bytesWritten);
}).catch((err) => {
  console.error(err);
});
```

---

## info() (on Upload)

### Description:

Returns information about the object being uploaded.\
`uploadObject` is required as a pre-requisite.\
Returns an `ObjectInfo` object.

### Usage Example:

```js
await upload.info().then((objectInfo) => {
  console.log("Upload key:", objectInfo.key);
}).catch((err) => {
  console.error(err);
});
```

---

## commit()

### Description:

Commits the upload — finalizes the data on Storj.\
`uploadObject` is required as a pre-requisite.\
Must be called after all `write()` calls.

### Usage Example:

```js
await upload.commit().then(() => {
  console.log("Upload committed successfully");
}).catch((err) => {
  console.error(err);
});
```

---

## abort()

### Description:

Aborts an in-progress upload, discarding all uploaded data.\
`uploadObject` is required as a pre-requisite.

### Usage Example:

```js
await upload.abort().then(() => {
  console.log("Upload aborted");
}).catch((err) => {
  console.error(err);
});
```

---

> NOTE: Following functions require a `DownloadResultStruct` object (returned by `downloadObject`).

---

## read(buffer, length)

### Description:

Downloads data from the object's data stream into the buffer.\
`downloadObject` is required as a pre-requisite.\
Returns a `ReadResult` with the number of bytes read.

### Arguments:

| arguments | Description | Type |
| --- | --- | --- |
| `buffer` | Buffer to read data into | `Buffer` |
| `length` | Maximum number of bytes to read | `number` |

### Usage Example:

```js
const buffer = Buffer.alloc(4096);
await download.read(buffer, buffer.length).then(async (result) => {
  console.log("Bytes read:", result.bytesRead);
}).catch((err) => {
  console.error(err);
});
```

---

## info() (on Download)

### Description:

Returns information about the object being downloaded.\
`downloadObject` is required as a pre-requisite.\
Returns an `ObjectInfo` object (including `system.contentLength` for the total size).

### Usage Example:

```js
await download.info().then((objectInfo) => {
  console.log("Object key:", objectInfo.key);
  console.log("Size (bytes):", objectInfo.system.contentLength);
}).catch((err) => {
  console.error(err);
});
```

---

## close() (on Download)

### Description:

Closes the download stream and releases resources.\
`downloadObject` is required as a pre-requisite.\
Always call this when done, preferably in a `finally` block.

### Usage Example:

```js
await download.close().then(() => {
  console.log("Download stream closed");
}).catch((err) => {
  console.error(err);
});
```

---

> NOTE: Following are **standalone functions** for multipart uploads. They require a native project handle (`project._nativeHandle`).

---

## beginMultipartUpload(projectHandle, bucket, key, options?)

### Description:

Begins a new multipart upload.\
Multipart uploads allow uploading large files in parts — useful for resumable or parallel uploads.\
Returns a `MultipartUpload` helper object.

### Arguments:

| arguments | Description | Type |
| --- | --- | --- |
| `projectHandle` | Native project handle (`project._nativeHandle`) | `unknown` |
| `bucket` | Bucket name | `string` |
| `key` | Object key | `string` |
| `options` | Optional upload options (expiry) | `BeginUploadOptions` (optional) |

### Usage Example:

```js
const { beginMultipartUpload } = require("storj-uplink-nodejs");

const mp = await beginMultipartUpload(project._nativeHandle, "my-bucket", "large-file.zip");

// Upload parts
const part1 = await mp.uploadPart(1);
await part1.write(chunk1, chunk1.length);
await part1.commit();

// Finalize
const objectInfo = await mp.commit();
console.log("Multipart upload complete:", objectInfo.key);
```

---

## listMultipartUploads(projectHandle, bucket, options?)

### Description:

Lists all pending (uncommitted) multipart uploads in a bucket.\
Returns an array of `UploadInfo` objects.

### Arguments:

| arguments | Description | Type |
| --- | --- | --- |
| `projectHandle` | Native project handle (`project._nativeHandle`) | `unknown` |
| `bucket` | Bucket name | `string` |
| `options` | Optional list options | `ListUploadsOptions` (optional) |

### Usage Example:

```js
const { listMultipartUploads } = require("storj-uplink-nodejs");

const uploads = await listMultipartUploads(project._nativeHandle, "my-bucket");
for (const upload of uploads) {
  console.log("Pending upload:", upload.uploadId, upload.key);
}
```

---

> NOTE: Following are **standalone functions** for Storj edge/linkshare services.

---

## edgeRegisterAccess(config, accessHandle, options?)

### Description:

Registers an access grant with Storj edge services to obtain S3-compatible credentials.\
Returns an `EdgeCredentials` object with `accessKeyId`, `secretKey`, and `endpoint`.

### Arguments:

| arguments | Description | Type |
| --- | --- | --- |
| `config` | Edge configuration (auth service address) | `EdgeConfig` |
| `accessHandle` | Native access handle (`access._nativeHandle`) | `unknown` |
| `options` | Optional options (`isPublic: true` for linkshare) | `EdgeRegisterAccessOptions` (optional) |

### Usage Example:

```js
const { edgeRegisterAccess, EdgeRegions } = require("storj-uplink-nodejs");

const credentials = await edgeRegisterAccess(
  { authServiceAddress: EdgeRegions.US1.authService },
  access._nativeHandle,
  { isPublic: true }
);

console.log("Access Key:", credentials.accessKeyId);
console.log("Endpoint:", credentials.endpoint);
```

---

## edgeJoinShareUrl(baseUrl, accessKeyId, bucket, key, options?)

### Description:

Creates a shareable linkshare URL for an object or bucket.\
Requires an access key registered with `isPublic: true`.\
Returns a URL string.

### Arguments:

| arguments | Description | Type |
| --- | --- | --- |
| `baseUrl` | Linkshare base URL (e.g. `https://link.us1.storjshare.io`) | `string` |
| `accessKeyId` | Access key ID from `edgeRegisterAccess` | `string` |
| `bucket` | Bucket name (empty string to share entire project) | `string` |
| `key` | Object key or prefix (empty string to share entire bucket) | `string` |
| `options` | Optional options (`raw: true` to serve file directly) | `EdgeShareURLOptions` (optional) |

### Usage Example:

```js
const { edgeJoinShareUrl, EdgeRegions } = require("storj-uplink-nodejs");

const url = await edgeJoinShareUrl(
  EdgeRegions.US1.linkshare,
  credentials.accessKeyId,
  "my-bucket",
  "photos/vacation.jpg",
  { raw: true }
);

console.log("Share URL:", url);
```

---

> NOTE: All binding functions are asynchronous and return Promises.

> NOTE: You can view the uplink-c documentation [here](https://pkg.go.dev/storj.io/uplink).
