# Tutorial

> Welcome to the tutorial for creating your own project with `storj-uplink-nodejs`. Let's start!

> Note: All functions are asynchronous and return Promises.

---

## Step 1: Storj Configurations

First you need your Storj credentials. You can store them in a `.env.test` file or initialize variables directly:

```js
const storjConfig = {
  satellite:   "us1.storj.io:7777",
  apiKey:      "change-me-to-your-api-key",
  passphrase:  "change-me-to-your-encryption-passphrase",
  bucketName:  "change-me-to-desired-bucket-name",
  uploadPath:  "optional/path/filename.txt",
};
```

Or load from environment variables:

```js
require("dotenv").config({ path: ".env.test" });

const storjConfig = {
  satellite:  process.env.TEST_SATELLITE,
  apiKey:     process.env.TEST_API_KEY,
  passphrase: process.env.TEST_PASSPHRASE,
  bucketName: process.env.TEST_BUCKET || "my-bucket",
};
```

---

## Step 2: File Paths

If working with files, define source and destination paths:

```js
const localFiles = {
  src:  "change-me-to-source-file-path",
  dest: "change-me-to-destination-file-path",
};
```

---

## Step 3: Create Uplink Object

Create an instance of the `Uplink` class:

```js
// CommonJS
const { Uplink } = require("storj-uplink-nodejs");
const uplink = new Uplink();
```

```ts
// TypeScript
import { Uplink } from "storj-uplink-nodejs";
const uplink = new Uplink();
```

---

## Step 4: Create Access Handle

Obtain an access grant to the Storj V3 network using one of these methods:

### requestAccessWithPassphrase

```js
// Promise .then() style
uplink.requestAccessWithPassphrase(
  storjConfig.satellite,
  storjConfig.apiKey,
  storjConfig.passphrase
).then(access => {
  // ...use access...
}).catch((err) => {
  console.error("Access error:", err);
});

// OR async/await style
const access = await uplink.requestAccessWithPassphrase(
  storjConfig.satellite,
  storjConfig.apiKey,
  storjConfig.passphrase
);
```

The function accepts 3 arguments — Satellite address, API Key, and Passphrase — and returns an `AccessResultStruct` on success.

> NOTE: We will use async/await style for the remainder of this tutorial.

### configRequestAccessWithPassphrase

```js
const config = {
  userAgent: "MyApp/1.0",
  dialTimeoutMilliseconds: 10000
};

const access = await uplink.configRequestAccessWithPassphrase(
  config,
  storjConfig.satellite,
  storjConfig.apiKey,
  storjConfig.passphrase
);
```

Accepts 4 arguments — Config object, Satellite address, API Key, and Passphrase.

### parseAccess

```js
// For re-using a previously serialized access grant:
const serializedGrant = "1J5F2Kxxxxxxxxxxxxxxxxxxxxxxxx";
const access = await uplink.parseAccess(serializedGrant);
```

> NOTE: `parseAccess` is used when you have a serialized access string (e.g. from `access.serialize()`).

---

## Step 5: Open Project

Once you have an access grant, open a project:

### openProject

```js
const project = await access.openProject();
```

Returns a `ProjectResultStruct`. The `project` handle is used in all subsequent steps.

### configOpenProject

```js
const config = { userAgent: "MyApp/1.0" };
const project = await access.configOpenProject(config);
```

---

## Step 6: Create / Ensure Bucket

Before uploading, make sure the target bucket exists:

### ensureBucket (Recommended)

```js
const bucketInfo = await project.ensureBucket(storjConfig.bucketName);
console.log("Bucket ready:", bucketInfo.name);
```

Creates the bucket if it doesn't exist; succeeds silently if it already does.

### statBucket

```js
const bucketInfo = await project.statBucket(storjConfig.bucketName);
console.log("Bucket exists:", bucketInfo.name);
```

Returns bucket information. Throws `BucketNotFoundError` if the bucket does not exist.

### createBucket

```js
const bucketInfo = await project.createBucket(storjConfig.bucketName);
console.log("Created:", bucketInfo.name);
```

Creates a new bucket. Throws `BucketAlreadyExistsError` if it already exists.

---

## Step 7: List Buckets (Optional)

Once a bucket exists, you can list all buckets in the project:

```js
const buckets = await project.listBuckets();
console.log("All buckets:");
for (const bucket of buckets) {
  console.log(" -", bucket.name, "created:", new Date(bucket.created * 1000).toISOString());
}
```

With pagination:

```js
const buckets = await project.listBuckets({ cursor: "my-last-bucket" });
```

---

## Step 8: Upload

Uploading a file involves these sub-steps:

### Get Upload Handle

```js
const upload = await project.uploadObject(storjConfig.bucketName, storjConfig.uploadPath);
```

With optional expiry:

```js
const upload = await project.uploadObject(storjConfig.bucketName, storjConfig.uploadPath, {
  expires: new Date(Date.now() + 7 * 24 * 60 * 60 * 1000) // 7 days
});
```

### Read and Write File Data

```js
const fs = require("fs");
const BUFFER_SIZE = 256 * 1024; // 256 KB chunks

const fileHandle = fs.openSync(localFiles.src, "r");
const fileSize = fs.statSync(localFiles.src).size;

const buffer = Buffer.alloc(BUFFER_SIZE);
let totalWritten = 0;

while (totalWritten < fileSize) {
  const toRead = Math.min(BUFFER_SIZE, fileSize - totalWritten);
  const bytesRead = fs.readSync(fileHandle, buffer, 0, toRead, totalWritten);
  if (bytesRead === 0) break;

  const written = await upload.write(buffer.slice(0, bytesRead), bytesRead);
  totalWritten += written;
}

fs.closeSync(fileHandle);
```

For small data, you can write directly:

```js
const data = Buffer.from("Hello, Storj!");
await upload.write(data, data.length);
```

### (Optional) Set Custom Metadata

```js
await upload.setCustomMetadata({
  "app:filename": "report.pdf",
  "app:author": "Jane Doe"
});
```

### Commit Upload

```js
await upload.commit();
console.log("Upload complete!");
```

### Abort Upload (on error)

```js
await upload.abort(); // discard uploaded data if something went wrong
```

---

## Step 9: Download

Downloading involves these sub-steps:

### Get Download Handle

```js
const download = await project.downloadObject(storjConfig.bucketName, storjConfig.uploadPath);
```

With byte-range (partial download):

```js
const download = await project.downloadObject(storjConfig.bucketName, storjConfig.uploadPath, {
  offset: 0,
  length: 1024 // first 1KB only
});
```

### Get Object Info (Size)

```js
const objectInfo = await download.info();
const objectSize = objectInfo.system.contentLength;
console.log("Object size:", objectSize, "bytes");
```

### Read and Write to File

```js
const fs = require("fs");
const BUFFER_SIZE = 256 * 1024;

const fileHandle = fs.openSync(localFiles.dest, "w");
let totalRead = 0;

while (totalRead < objectSize) {
  const toRead = Math.min(BUFFER_SIZE, objectSize - totalRead);
  const buffer = Buffer.alloc(toRead);
  const result = await download.read(buffer, toRead);
  if (result.bytesRead === 0) break;

  fs.writeSync(fileHandle, buffer, 0, result.bytesRead);
  totalRead += result.bytesRead;
}

fs.closeSync(fileHandle);
```

For small objects, read all at once:

```js
const buffer = Buffer.alloc(objectSize);
const result = await download.read(buffer, objectSize);
console.log("Downloaded:", result.bytesRead, "bytes");
```

### Close Download Stream

```js
await download.close();
console.log("Download complete!");
```

> NOTE: Always close the download stream when done, even on error:
> ```js
> try { ... } finally { await download.close(); }
> ```

---

## Step 10: Error Handling

All Storj errors extend `StorjError` and have a numeric `code` property:

```js
const {
  StorjError,
  BucketNotFoundError,
  BucketNotEmptyError,
  ObjectNotFoundError,
  PermissionDeniedError
} = require("storj-uplink-nodejs");

try {
  await project.statBucket("nonexistent-bucket");
} catch (err) {
  if (err instanceof BucketNotFoundError) {
    console.log("Bucket does not exist, creating it...");
    await project.createBucket("nonexistent-bucket");
  } else if (err instanceof PermissionDeniedError) {
    console.error("Access denied — check your access grant");
  } else if (err instanceof StorjError) {
    console.error("Storj error:", err.code, err.message);
  } else {
    throw err; // re-throw unexpected errors
  }
}
```

Use `finally` to always clean up resources:

```js
const project = await access.openProject();
try {
  // ...work with project...
} finally {
  await project.close(); // always called
}
```

---

## Step 11: Shared Access (Optional)

To share access with restricted permissions:

```js
// Define permissions
const permission = {
  allowDownload: true,
  allowUpload: false,
  allowList: true,
  allowDelete: false,
  notAfter: new Date(Date.now() + 24 * 60 * 60 * 1000) // expires in 24h
};

// Define what to share
const prefixes = [
  { bucket: storjConfig.bucketName, prefix: "public/" }
];

// Create restricted access
const sharedAccess = await access.share(permission, prefixes);

// Serialize to share with others
const serialized = await sharedAccess.serialize();
console.log("Share this grant:", serialized);

// Other user can use it with:
// const access = await uplink.parseAccess(serialized);
```

---

## Step 12: List Objects (Optional)

```js
const objects = await project.listObjects(storjConfig.bucketName, {
  prefix: "photos/",
  recursive: true,
  system: true
});

for (const obj of objects) {
  const sizeKB = (obj.system.contentLength / 1024).toFixed(1);
  console.log(`${obj.key} (${sizeKB} KB)`);
}
```

---

## Step 13: Copy / Move Objects (Optional)

```js
// Copy object
await project.copyObject("src-bucket", "original.txt", "dst-bucket", "copy.txt");

// Move (rename) object
await project.moveObject("my-bucket", "old-name.txt", "my-bucket", "new-name.txt");
```

---

## Step 14: Delete Object

```js
await project.deleteObject(storjConfig.bucketName, storjConfig.uploadPath);
console.log("Object deleted");
```

---

## Step 15: Delete Bucket

```js
// Bucket must be empty before deletion
await project.deleteBucket(storjConfig.bucketName);
console.log("Bucket deleted");
```

---

## Step 16: Close Project

After all operations, always close the project:

```js
await project.close();
console.log("Project closed");
```

---

## Complete Example

```js
const { Uplink } = require("storj-uplink-nodejs");
const fs = require("fs");

async function main() {
  const uplink = new Uplink();

  const access = await uplink.requestAccessWithPassphrase(
    process.env.TEST_SATELLITE,
    process.env.TEST_API_KEY,
    process.env.TEST_PASSPHRASE
  );

  const project = await access.openProject();

  try {
    const bucket = "tutorial-bucket";
    await project.ensureBucket(bucket);

    // Upload
    const upload = await project.uploadObject(bucket, "hello.txt");
    const data = Buffer.from("Hello, Storj!");
    await upload.write(data, data.length);
    await upload.commit();
    console.log("Uploaded hello.txt");

    // List
    const objects = await project.listObjects(bucket, { system: true });
    for (const obj of objects) {
      console.log(`- ${obj.key} (${obj.system.contentLength} bytes)`);
    }

    // Download
    const download = await project.downloadObject(bucket, "hello.txt");
    const buf = Buffer.alloc(data.length);
    await download.read(buf, buf.length);
    await download.close();
    console.log("Downloaded:", buf.toString());

    // Clean up
    await project.deleteObject(bucket, "hello.txt");
    await project.deleteBucket(bucket);
    console.log("Cleaned up");
  } finally {
    await project.close();
  }
}

main().catch(err => {
  console.error("Fatal error:", err);
  process.exit(1);
});
```

> NOTE: For more binding functions refer to the [storj-uplink-nodejs Binding Functions](/library.md) and the [Types, Errors and Constants](/types.md) documentation.

> NOTE: Perform error handling as per your implementation requirements.
