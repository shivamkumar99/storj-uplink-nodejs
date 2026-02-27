# Examples

The `example/` directory contains **8 ready-to-run TypeScript scripts** covering the most common Storj operations.

There are two ways to run them:

| Approach | When to use |
|----------|-------------|
| [Inside this repo](#option-a-run-inside-this-repo) | You cloned `storj-uplink-nodejs` and want to test against the local build |
| [Your own project](#option-b-run-in-your-own-project) | You installed `storj-uplink-nodejs` as an npm dependency — **start here if you are copying examples** |

---

## Option A — Run inside this repo

### 1. Build the native module

```sh
npm install
make install          # auto-detects: prebuilt → hybrid → source
npm run build:ts
```

### 2. Set credentials

Create `.env.test` at the repo root:

```sh
TEST_SATELLITE=us1.storj.io:7777
TEST_API_KEY=your-api-key-from-storj-console
TEST_PASSPHRASE=your-secret-passphrase
```

Or export in your shell:

```sh
export TEST_SATELLITE="us1.storj.io:7777"
export TEST_API_KEY="your-api-key"
export TEST_PASSPHRASE="your-passphrase"
```

### 3. Run an example

```sh
npx ts-node example/create-list-buckets.ts
```

> `ts-node` is already a devDependency — `npx ts-node` picks it up from `node_modules/.bin/` automatically.

---

## Option B — Run in your own project

This is the path to follow when you **copy an example into a fresh project** and run it with `npx ts-node example/<file>.ts`.

### Why a plain `npm init` + `npx ts-node` shows no output

When you create a new project with `npm init`, three things are missing that cause `npx ts-node` to silently exit with code 0 and print nothing:

1. **No `tsconfig.json`** — without it, `ts-node` cannot determine the module system. Under `"type": "commonjs"` (the npm default), `import` statements hit the Node.js CommonJS parser, fail immediately, and the process exits `0` with zero output and zero error message.
2. **`ts-node` not installed locally** — `npx ts-node` pulls a random cached version from the global npx cache, not a pinned local one, and without a `tsconfig.json` it uses broken defaults.
3. **`dotenv` version mismatch** — copy-pasting `"dotenv": "^17.x"` from some sources installs a non-existent version, causing credentials to silently not load.

### Correct project setup (step by step)

#### 1. Create the project

```sh
mkdir my-storj-app && cd my-storj-app
npm init -y
```

#### 2. Install dependencies

```sh
# Runtime dependencies
npm install storj-uplink-nodejs dotenv

# Dev dependencies — ts-node + typescript are required to run .ts files directly
npm install --save-dev ts-node typescript @types/node
```

> **Why install `ts-node` locally?**
> `npx ts-node` resolves in this order: PATH → global npx cache → `node_modules/.bin/`.
> Without a local install, you get a random globally cached version that ignores your `tsconfig.json`.
> A local install guarantees the right version is always used and picks up your config automatically.

#### 3. Create `tsconfig.json`

This is the **most important step** — without it, `import` statements silently fail.

Create `tsconfig.json` at the project root:

```json
{
  "compilerOptions": {
    "target": "ES2020",
    "module": "CommonJS",
    "moduleResolution": "node",
    "esModuleInterop": true,
    "strict": true,
    "outDir": "dist",
    "rootDir": ".",
    "resolveJsonModule": true,
    "skipLibCheck": true
  },
  "include": ["example/**/*"],
  "exclude": ["node_modules", "dist"]
}
```

> `"module": "CommonJS"` tells `ts-node` to compile `import` statements to `require()` calls,
> which is what Node.js expects in a `"type": "commonjs"` package.

#### 4. Set credentials

Create `.env.test` at the project root (same directory as `package.json`):

```sh
TEST_SATELLITE=us1.storj.io:7777
TEST_API_KEY=your-api-key-from-storj-console
TEST_PASSPHRASE=your-secret-passphrase
```

> The examples call `dotenv.config({ path: path.resolve(__dirname, '../.env.test') })`.
> `__dirname` is the `example/` folder, so `../` resolves to the project root — exactly where `package.json` lives.

#### 5. Add npm scripts (recommended)

Add convenience scripts to `package.json` so you never need to type file paths:

```json
{
  "scripts": {
    "example:buckets":            "ts-node example/create-list-buckets.ts",
    "example:upload":             "ts-node example/upload-chunks.ts",
    "example:download":           "ts-node example/download-file.ts",
    "example:list-objects":       "ts-node example/list-objects.ts",
    "example:upload-multipart":   "ts-node example/upload-multipart.ts",
    "example:download-multipart": "ts-node example/download-multipart.ts",
    "example:delete-object":      "ts-node example/delete-object.ts",
    "example:delete-buckets":     "ts-node example/delete-buckets.ts"
  }
}
```

npm automatically adds `node_modules/.bin/` to `PATH` when running scripts, so `ts-node` resolves correctly.

#### 6. Copy the examples

```sh
mkdir example
# copy the .ts files from the storj-uplink-nodejs example/ directory
```

#### 7. Run

```sh
# Via npm script (recommended)
npm run example:buckets

# Or directly
npx ts-node example/create-list-buckets.ts
```

### Complete `package.json` reference

```json
{
  "name": "my-storj-app",
  "version": "1.0.0",
  "type": "commonjs",
  "scripts": {
    "example:buckets":            "ts-node example/create-list-buckets.ts",
    "example:upload":             "ts-node example/upload-chunks.ts",
    "example:download":           "ts-node example/download-file.ts",
    "example:list-objects":       "ts-node example/list-objects.ts",
    "example:upload-multipart":   "ts-node example/upload-multipart.ts",
    "example:download-multipart": "ts-node example/download-multipart.ts",
    "example:delete-object":      "ts-node example/delete-object.ts",
    "example:delete-buckets":     "ts-node example/delete-buckets.ts"
  },
  "dependencies": {
    "dotenv": "^16.4.5",
    "storj-uplink-nodejs": "^0.1.0-beta.22"
  },
  "devDependencies": {
    "@types/node": "^20.14.0",
    "ts-node": "^10.9.2",
    "typescript": "^5.4.5"
  }
}
```

---

## Example Scripts

---

### 1. `create-list-buckets.ts`

**What it does:**
Creates 4 uniquely named buckets (with `Date.now()` suffix to avoid conflicts), then lists all buckets in the project and prints their names.

**Demonstrates:** `ensureBucket()`, `listBuckets()`

**Environment variables:**

| Variable | Required | Default | Description |
|----------|----------|---------|-------------|
| `TEST_SATELLITE` | Yes | — | Satellite address |
| `TEST_API_KEY` | Yes | — | Storj API key |
| `TEST_PASSPHRASE` | Yes | — | Encryption passphrase |

**Run:**

```sh
npx ts-node example/create-list-buckets.ts
# or
npm run example:buckets
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

**What it does:**
Lists all buckets in the project and deletes up to 4 buckets whose names start with `example-bucket-`. Useful for cleaning up after `create-list-buckets.ts`.

**Demonstrates:** `listBuckets()`, `deleteBucket()`

**Environment variables:**

| Variable | Required | Default | Description |
|----------|----------|---------|-------------|
| `TEST_SATELLITE` | Yes | — | Satellite address |
| `TEST_API_KEY` | Yes | — | Storj API key |
| `TEST_PASSPHRASE` | Yes | — | Encryption passphrase |

**Run:**

```sh
npx ts-node example/delete-buckets.ts
# or
npm run example:delete-buckets
```

**Expected output:**

```
Deleted bucket: example-bucket-1-1700000001
Deleted bucket: example-bucket-2-1700000001
Deleted bucket: example-bucket-3-1700000001
Deleted bucket: example-bucket-4-1700000001
```

> **Note:** The bucket must be empty before deletion. If it contains objects, `BucketNotEmptyError` is thrown.

---

### 3. `upload-chunks.ts`

**What it does:**
Reads a local file in **1 MB chunks** using a Node.js `ReadStream` and uploads it to Storj using `write()` in a streaming loop. Recommended approach for large files.

**Demonstrates:** `ensureBucket()`, `uploadObject()`, `upload.write()` (streaming), `upload.commit()`

**Environment variables:**

| Variable | Required | Default | Description |
|----------|----------|---------|-------------|
| `TEST_SATELLITE` | Yes | — | Satellite address |
| `TEST_API_KEY` | Yes | — | Storj API key |
| `TEST_PASSPHRASE` | Yes | — | Encryption passphrase |
| `TEST_BUCKET` | No | `example-chunk-bucket` | Destination bucket |
| `UPLOAD_FILE_PATH` | No | `example/sample.txt` | Local file to upload |
| `UPLOAD_OBJECT_KEY` | No | `chunked-upload.txt` | Object key on Storj |

**Run:**

```sh
# With defaults (create a sample file first if needed)
echo "Hello Storj" > example/sample.txt
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

**What it does:**
Downloads an object from Storj into memory using `download.info()` to get the file size, then `download.read()` to read all bytes into a buffer, and writes the result to a local file.

**Demonstrates:** `downloadObject()`, `download.info()`, `download.read()`, `download.close()`

**Environment variables:**

| Variable | Required | Default | Description |
|----------|----------|---------|-------------|
| `TEST_SATELLITE` | Yes | — | Satellite address |
| `TEST_API_KEY` | Yes | — | Storj API key |
| `TEST_PASSPHRASE` | Yes | — | Encryption passphrase |
| `TEST_BUCKET` | No | `example-chunk-bucket` | Source bucket |
| `UPLOAD_OBJECT_KEY` | No | `chunked-upload.txt` | Object key to download |
| `DOWNLOAD_FILE_PATH` | No | `example/downloaded.txt` | Local destination path |

**Run:**

```sh
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

> **Note:** Run `upload-chunks.ts` first to create an object to download.

---

### 5. `list-objects.ts`

**What it does:**
Lists all objects in a bucket recursively and prints each object's key and size in bytes.

**Demonstrates:** `listObjects()` with `{ recursive: true }`

**Environment variables:**

| Variable | Required | Default | Description |
|----------|----------|---------|-------------|
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

**What it does:**
Deletes a single object from a Storj bucket by key.

**Demonstrates:** `deleteObject()`

**Environment variables:**

| Variable | Required | Default | Description |
|----------|----------|---------|-------------|
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

**What it does:**
Uploads a large file using **multipart upload** in **5 MB parts**. Uses `beginMultipartUpload()` to start the session, streams the file in chunks, uploads each chunk as a separate numbered part, then calls `mp.commit()` to finalise all parts into a single object.

**Demonstrates:** `beginMultipartUpload()`, `mp.uploadPart()`, `part.write()`, `part.commit()`, `mp.commit()`

**Environment variables:**

| Variable | Required | Default | Description |
|----------|----------|---------|-------------|
| `TEST_SATELLITE` | Yes | — | Satellite address |
| `TEST_API_KEY` | Yes | — | Storj API key |
| `TEST_PASSPHRASE` | Yes | — | Encryption passphrase |
| `TEST_BUCKET` | No | `example-multipart-bucket` | Destination bucket |
| `UPLOAD_FILE_PATH` | No | `example/large.txt` | Local file to upload |
| `UPLOAD_OBJECT_KEY` | No | `multipart-upload.txt` | Object key on Storj |

**Run:**

```sh
# Create a test large file first
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

> **Note:** Designed for large files. For files smaller than 5 MB, use `upload-chunks.ts` instead.

---

### 8. `download-multipart.ts`

**What it does:**
Downloads a multipart-uploaded object from Storj into memory using `download.info()` to determine the full size, reads all bytes into a buffer, and saves to a local file.

**Demonstrates:** `downloadObject()`, `download.info()`, `download.read()`, `download.close()`

**Environment variables:**

| Variable | Required | Default | Description |
|----------|----------|---------|-------------|
| `TEST_SATELLITE` | Yes | — | Satellite address |
| `TEST_API_KEY` | Yes | — | Storj API key |
| `TEST_PASSPHRASE` | Yes | — | Encryption passphrase |
| `TEST_BUCKET` | No | `example-multipart-bucket` | Source bucket |
| `UPLOAD_OBJECT_KEY` | No | `multipart-upload.txt` | Object key to download |
| `DOWNLOAD_FILE_PATH` | No | `example/downloaded-multipart.txt` | Local destination path |

**Run:**

```sh
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

> **Note:** Run `upload-multipart.ts` first to create the object to download.

---

## Suggested Workflow

Run the examples in this order to see a full end-to-end flow:

```sh
# 1. Create buckets
npx ts-node example/create-list-buckets.ts

# 2. Upload a file in chunks
echo "Hello Storj" > example/sample.txt
npx ts-node example/upload-chunks.ts

# 3. List what's in the bucket
npx ts-node example/list-objects.ts

# 4. Download it back
npx ts-node example/download-file.ts

# 5. Multipart upload (for large files)
dd if=/dev/urandom of=example/large.txt bs=1M count=20
npx ts-node example/upload-multipart.ts

# 6. Download the multipart object
npx ts-node example/download-multipart.ts

# 7. Delete the uploaded object
npx ts-node example/delete-object.ts

# 8. Clean up buckets
npx ts-node example/delete-buckets.ts
```

---

## Troubleshooting

### Nothing prints — silent exit with code 0

**Symptom:** `npx ts-node example/create-list-buckets.ts` exits immediately with no output and no error.

**Cause:** Missing `tsconfig.json`. Without it, `ts-node` cannot resolve the `import` statements under `"type": "commonjs"`, so the script fails to parse and exits `0` silently.

**Fix:** Create `tsconfig.json` at the project root — see [Option B step 3](#3-create-tsconfigjson) above.

---

### `Missing Storj credentials in environment`

```
Error: Missing Storj credentials in environment
```

The `.env.test` file was not found or the variables inside it are not set. Check:

1. `.env.test` exists at the **project root** (same folder as `package.json`), not inside `example/`
2. All three required variables are present: `TEST_SATELLITE`, `TEST_API_KEY`, `TEST_PASSPHRASE`
3. The values are not the placeholder defaults (`your_access_grant_here`, `change-me-...`)

---

### `Cannot find module 'storj-uplink-nodejs'`

```
Error: Cannot find module 'storj-uplink-nodejs'
```

Run `npm install` first. If running from the source repo, also run:

```sh
make install
npm run build:ts
```

---

### `ts-node: command not found`

Use `npx ts-node` instead of bare `ts-node`, or install it locally:

```sh
npm install --save-dev ts-node typescript @types/node
npx ts-node example/create-list-buckets.ts
```

---

### `BucketNotEmptyError` when deleting a bucket

Delete all objects in the bucket first:

```sh
# List to see what's there
npx ts-node example/list-objects.ts

# Delete the object, then the bucket
npx ts-node example/delete-object.ts
npx ts-node example/delete-buckets.ts
```

---

### `ObjectNotFoundError` when downloading

Make sure you uploaded the object first. The `UPLOAD_OBJECT_KEY` in your download command must match what was uploaded:

```sh
# Upload first with a specific key
UPLOAD_OBJECT_KEY=my-file.txt npx ts-node example/upload-chunks.ts

# Then download with the same key
UPLOAD_OBJECT_KEY=my-file.txt npx ts-node example/download-file.ts
```

---

## Testing via integration suite

You can also exercise all example operations through the structured integration test suite (inside the library repo):

```sh
# Run all integration tests
npm run test:integration

# Run specific groups
npx jest test/integration/bucket.test.ts
npx jest test/integration/upload.test.ts
npx jest test/integration/download.test.ts
npx jest test/integration/multipart.test.ts
npx jest test/integration/object.test.ts
```

> Requires `TEST_SATELLITE`, `TEST_API_KEY`, and `TEST_PASSPHRASE` set in `.env.test`.

---

> For full API documentation, see the [Binding Functions](/library.md) reference.
> For all type definitions and error classes, see [Types, Errors and Constants](/types.md).
> For a full step-by-step coding walkthrough, see the [Tutorial](/tutorial.md).
