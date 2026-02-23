# Examples

The `example/` directory contains **8 ready-to-run TypeScript scripts** covering the most common Storj operations. Each script is self-contained and reads credentials from environment variables or a `.env.test` file.

---

## Prerequisites

### 1. Install the package

```sh
npm install storj-uplink-nodejs
```

### 2. Install ts-node (to run TypeScript examples directly)

```sh
npm install -g ts-node
# OR use npx (no global install needed)
npx ts-node example/<script-name>.ts
```

### 3. Set credentials

Create a `.env.test` file at the project root:

```
TEST_SATELLITE=us1.storj.io:7777
TEST_API_KEY=your-api-key-from-storj-console
TEST_PASSPHRASE=your-secret-passphrase
```

Or export them in your shell:

```sh
export TEST_SATELLITE="us1.storj.io:7777"
export TEST_API_KEY="your-api-key"
export TEST_PASSPHRASE="your-passphrase"
```

> NOTE: All examples load credentials from `.env.test` automatically using `dotenv`.

---

## Running Examples

### General syntax

```sh
# Using npx ts-node (recommended — no global install required)
npx ts-node example/<script-name>.ts

# Using globally installed ts-node
ts-node example/<script-name>.ts

# After building (compile TypeScript first)
npm run build:ts
node dist/example/<script-name>.js
```

### With optional environment variables

Most examples accept additional env vars for bucket name, file paths, and object keys:

```sh
TEST_BUCKET=my-bucket \
UPLOAD_FILE_PATH=./my-file.txt \
UPLOAD_OBJECT_KEY=uploads/my-file.txt \
npx ts-node example/upload-chunks.ts
```

---

## Example Scripts

---

### 1. `create-list-buckets.ts`

**What it does:**\
Creates 4 uniquely named buckets (with `Date.now()` suffix to avoid conflicts), then lists all buckets in the project and prints their names.

**Demonstrates:** `ensureBucket()`, `listBuckets()`

**Environment variables:**

| Variable | Required | Default | Description |
| --- | --- | --- | --- |
| `TEST_SATELLITE` | Yes | — | Satellite address |
| `TEST_API_KEY` | Yes | — | Storj API key |
| `TEST_PASSPHRASE` | Yes | — | Encryption passphrase |

**Run:**

```sh
npx ts-node example/create-list-buckets.ts
```

**Expected output:**

```
Created bucket: example-bucket-1-1700000001
Created bucket: example-bucket-2-1700000001
Created bucket: example-bucket-3-1700000001
Created bucket: example-bucket-4-1700000001
Buckets:
- example-bucket-1-1700000001
- example-bucket-2-1700000001
- example-bucket-3-1700000001
- example-bucket-4-1700000001
```

---

### 2. `delete-buckets.ts`

**What it does:**\
Lists all buckets in the project and deletes up to 4 buckets whose names start with `example-bucket-`. Useful for cleaning up after `create-list-buckets.ts`.

**Demonstrates:** `listBuckets()`, `deleteBucket()`

**Environment variables:**

| Variable | Required | Default | Description |
| --- | --- | --- | --- |
| `TEST_SATELLITE` | Yes | — | Satellite address |
| `TEST_API_KEY` | Yes | — | Storj API key |
| `TEST_PASSPHRASE` | Yes | — | Encryption passphrase |

**Run:**

```sh
npx ts-node example/delete-buckets.ts
```

**Expected output:**

```
Deleted bucket: example-bucket-1-1700000001
Deleted bucket: example-bucket-2-1700000001
Deleted bucket: example-bucket-3-1700000001
Deleted bucket: example-bucket-4-1700000001
```

> NOTE: The bucket must be empty before deletion. If the bucket contains objects, this will throw `BucketNotEmptyError`.

---

### 3. `upload-chunks.ts`

**What it does:**\
Reads a local file in **1 MB chunks** using a Node.js `ReadStream` and uploads it to Storj object by object using `write()` in a streaming loop. This is the recommended approach for large files.

**Demonstrates:** `ensureBucket()`, `uploadObject()`, `upload.write()` (streaming), `upload.commit()`

**Environment variables:**

| Variable | Required | Default | Description |
| --- | --- | --- | --- |
| `TEST_SATELLITE` | Yes | — | Satellite address |
| `TEST_API_KEY` | Yes | — | Storj API key |
| `TEST_PASSPHRASE` | Yes | — | Encryption passphrase |
| `TEST_BUCKET` | No | `example-chunk-bucket` | Destination bucket |
| `UPLOAD_FILE_PATH` | No | `example/sample.txt` | Local file to upload |
| `UPLOAD_OBJECT_KEY` | No | `chunked-upload.txt` | Object key on Storj |

**Run:**

```sh
# With defaults (uploads example/sample.txt)
npx ts-node example/upload-chunks.ts

# With custom file and bucket
TEST_BUCKET=my-bucket \
UPLOAD_FILE_PATH=./data/report.pdf \
UPLOAD_OBJECT_KEY=reports/report.pdf \
npx ts-node example/upload-chunks.ts
```

**Expected output:**

```
Uploaded chunk: 1048576 bytes
Uploaded chunk: 1048576 bytes
Uploaded chunk: 524288 bytes
Upload complete: chunked-upload.txt (2621440 bytes)
```

---

### 4. `download-file.ts`

**What it does:**\
Downloads an object from Storj into memory using `download.info()` to get the file size, then `download.read()` to read all bytes into a buffer, and writes the result to a local file.

**Demonstrates:** `downloadObject()`, `download.info()`, `download.read()`, `download.close()`

**Environment variables:**

| Variable | Required | Default | Description |
| --- | --- | --- | --- |
| `TEST_SATELLITE` | Yes | — | Satellite address |
| `TEST_API_KEY` | Yes | — | Storj API key |
| `TEST_PASSPHRASE` | Yes | — | Encryption passphrase |
| `TEST_BUCKET` | No | `example-chunk-bucket` | Source bucket |
| `UPLOAD_OBJECT_KEY` | No | `chunked-upload.txt` | Object key to download |
| `DOWNLOAD_FILE_PATH` | No | `example/downloaded.txt` | Local destination path |

**Run:**

```sh
# Download the file uploaded by upload-chunks.ts
npx ts-node example/download-file.ts

# With custom paths
TEST_BUCKET=my-bucket \
UPLOAD_OBJECT_KEY=reports/report.pdf \
DOWNLOAD_FILE_PATH=./local-report.pdf \
npx ts-node example/download-file.ts
```

**Expected output:**

```
Downloaded chunked-upload.txt to /path/to/example/downloaded.txt (2621440 bytes)
```

> NOTE: Run `upload-chunks.ts` first to create an object to download.

---

### 5. `list-objects.ts`

**What it does:**\
Lists all objects in a bucket recursively and prints each object's key and size in bytes.

**Demonstrates:** `listObjects()` with `{ recursive: true }`

**Environment variables:**

| Variable | Required | Default | Description |
| --- | --- | --- | --- |
| `TEST_SATELLITE` | Yes | — | Satellite address |
| `TEST_API_KEY` | Yes | — | Storj API key |
| `TEST_PASSPHRASE` | Yes | — | Encryption passphrase |
| `TEST_BUCKET` | No | `example-chunk-bucket` | Bucket to list |

**Run:**

```sh
npx ts-node example/list-objects.ts

# With a different bucket
TEST_BUCKET=my-bucket npx ts-node example/list-objects.ts
```

**Expected output:**

```
Objects in bucket 'example-chunk-bucket':
- chunked-upload.txt (2621440 bytes)
- another-file.txt (1024 bytes)
```

---

### 6. `delete-object.ts`

**What it does:**\
Deletes a single object from a Storj bucket by key.

**Demonstrates:** `deleteObject()`

**Environment variables:**

| Variable | Required | Default | Description |
| --- | --- | --- | --- |
| `TEST_SATELLITE` | Yes | — | Satellite address |
| `TEST_API_KEY` | Yes | — | Storj API key |
| `TEST_PASSPHRASE` | Yes | — | Encryption passphrase |
| `TEST_BUCKET` | No | `example-chunk-bucket` | Bucket containing the object |
| `UPLOAD_OBJECT_KEY` | No | `chunked-upload.txt` | Object key to delete |

**Run:**

```sh
npx ts-node example/delete-object.ts

# Delete a specific object
TEST_BUCKET=my-bucket \
UPLOAD_OBJECT_KEY=reports/report.pdf \
npx ts-node example/delete-object.ts
```

**Expected output:**

```
Deleted object: chunked-upload.txt from bucket: example-chunk-bucket
```

---

### 7. `upload-multipart.ts`

**What it does:**\
Uploads a large file using **multipart upload** in **5 MB parts**. Uses `beginMultipartUpload()` to start the upload, streams the file in chunks using a `ReadStream`, uploads each chunk as a separate part, then calls `mp.commit()` to finalize all parts into a single object.

**Demonstrates:** `beginMultipartUpload()`, `mp.uploadPart()`, `part.write()`, `part.commit()`, `mp.commit()`

**Environment variables:**

| Variable | Required | Default | Description |
| --- | --- | --- | --- |
| `TEST_SATELLITE` | Yes | — | Satellite address |
| `TEST_API_KEY` | Yes | — | Storj API key |
| `TEST_PASSPHRASE` | Yes | — | Encryption passphrase |
| `TEST_BUCKET` | No | `example-multipart-bucket` | Destination bucket |
| `UPLOAD_FILE_PATH` | No | `example/large.txt` | Local file to upload |
| `UPLOAD_OBJECT_KEY` | No | `multipart-upload.txt` | Object key on Storj |

**Run:**

```sh
# Create a test large file first (if needed)
dd if=/dev/urandom of=example/large.txt bs=1M count=20

# Run the multipart upload
npx ts-node example/upload-multipart.ts

# With custom file
TEST_BUCKET=my-bucket \
UPLOAD_FILE_PATH=./videos/clip.mp4 \
UPLOAD_OBJECT_KEY=videos/clip.mp4 \
npx ts-node example/upload-multipart.ts
```

**Expected output:**

```
Uploaded part 1: 5242880 bytes
Uploaded part 2: 5242880 bytes
Uploaded part 3: 5242880 bytes
Uploaded part 4: 4974592 bytes
Multipart upload complete: multipart-upload.txt (20971008 bytes)
```

> NOTE: This example is designed for large files. For files smaller than 5 MB, use `upload-chunks.ts` instead.

---

### 8. `download-multipart.ts`

**What it does:**\
Downloads an object from Storj into memory using `download.info()` to determine the full size, reads all bytes at once into a buffer, and saves it to a local file. Functionally identical to `download-file.ts` but pointed at the multipart-uploaded object.

**Demonstrates:** `downloadObject()`, `download.info()`, `download.read()`, `download.close()`

**Environment variables:**

| Variable | Required | Default | Description |
| --- | --- | --- | --- |
| `TEST_SATELLITE` | Yes | — | Satellite address |
| `TEST_API_KEY` | Yes | — | Storj API key |
| `TEST_PASSPHRASE` | Yes | — | Encryption passphrase |
| `TEST_BUCKET` | No | `example-multipart-bucket` | Source bucket |
| `UPLOAD_OBJECT_KEY` | No | `multipart-upload.txt` | Object key to download |
| `DOWNLOAD_FILE_PATH` | No | `example/downloaded-multipart.txt` | Local destination path |

**Run:**

```sh
# Download the object uploaded by upload-multipart.ts
npx ts-node example/download-multipart.ts

# With custom paths
TEST_BUCKET=my-bucket \
UPLOAD_OBJECT_KEY=videos/clip.mp4 \
DOWNLOAD_FILE_PATH=./local-clip.mp4 \
npx ts-node example/download-multipart.ts
```

**Expected output:**

```
Downloaded multipart multipart-upload.txt to /path/to/example/downloaded-multipart.txt (20971008 bytes)
```

> NOTE: Run `upload-multipart.ts` first to create the object to download.

---

## Suggested Workflow

Run the examples in this order to see a full end-to-end flow:

```sh
# 1. Create buckets
npx ts-node example/create-list-buckets.ts

# 2. Upload a file in chunks (create sample.txt first if needed)
echo "Hello Storj" > example/sample.txt
npx ts-node example/upload-chunks.ts

# 3. List what's in the bucket
npx ts-node example/list-objects.ts

# 4. Download it back
npx ts-node example/download-file.ts

# 5. Multipart upload (create a large file first)
dd if=/dev/urandom of=example/large.txt bs=1M count=20
npx ts-node example/upload-multipart.ts

# 6. Download the multipart object
npx ts-node example/download-multipart.ts

# 7. Delete the uploaded object
npx ts-node example/delete-object.ts

# 8. Delete the buckets
npx ts-node example/delete-buckets.ts
```

---

## Testing Examples

You can run a quick sanity test of all examples using the integration test suite (which covers the same operations in a structured way):

```sh
# Run all integration tests (covers all example operations)
npm run test:integration

# Run specific integration test groups
npx jest test/integration/bucket.test.ts
npx jest test/integration/upload.test.ts
npx jest test/integration/download.test.ts
npx jest test/integration/multipart.test.ts
npx jest test/integration/object.test.ts
```

Or run the full E2E suite which mirrors the workflow above:

```sh
# Full E2E workflow
npm run test:e2e

# Individual E2E steps
npm run test:e2e:bucket          # bucket create/list/delete
npm run test:e2e:upload          # upload
npm run test:e2e:download        # download
npm run test:e2e:multipart-upload    # multipart upload
npm run test:e2e:multipart-download  # multipart download
npm run test:e2e:list            # list objects
npm run test:e2e:object-ops      # copy/move/delete
npm run test:e2e:cleanup         # cleanup
```

> NOTE: Integration and E2E tests require `TEST_SATELLITE`, `TEST_API_KEY`, and `TEST_PASSPHRASE` to be set in `.env.test`.

---

## Troubleshooting

### `Missing Storj credentials in environment`

```
Error: Missing Storj credentials in environment
```

Create or check your `.env.test` file at the project root. All three variables — `TEST_SATELLITE`, `TEST_API_KEY`, `TEST_PASSPHRASE` — must be set.

---

### `Cannot find module 'storj-uplink-nodejs'`

```
Error: Cannot find module 'storj-uplink-nodejs'
```

Run `npm install` first. If running from source, also run `npm run build:ts` and `make install-source`.

---

### `ts-node: command not found`

Use `npx ts-node` instead of `ts-node`:

```sh
npx ts-node example/create-list-buckets.ts
```

---

### `BucketNotEmptyError` when deleting a bucket

Delete all objects in the bucket first:

```sh
# List to see what's there
npx ts-node example/list-objects.ts

# Delete each object, then delete the bucket
npx ts-node example/delete-object.ts
npx ts-node example/delete-buckets.ts
```

---

### `ObjectNotFoundError` when downloading

Make sure you uploaded the object first. The `UPLOAD_OBJECT_KEY` in your download command must match what was uploaded:

```sh
# Upload first
UPLOAD_OBJECT_KEY=my-file.txt npx ts-node example/upload-chunks.ts

# Then download with the same key
UPLOAD_OBJECT_KEY=my-file.txt npx ts-node example/download-file.ts
```

---

> NOTE: For full API documentation, see the [Binding Functions](/library.md) reference.

> NOTE: For all type definitions and error classes, see [Types, Errors and Constants](/types.md).
