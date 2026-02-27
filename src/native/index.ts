/**
 * @file native/index.ts
 * @brief Centralized native module loader
 *
 * This module loads the native addon once and exports it for use
 * throughout the application. This is more efficient than requiring
 * the native module multiple times in different files.
 */

import * as path from 'path';
import * as os from 'os';

/**
 * Instance type for errors created by native error classes.
 * These match the shape of the classes defined in error_registry.c.
 */
export interface StorjErrorInstance extends Error {
  readonly code: number;
  readonly details?: string;
}

/**
 * Constructor type for the base StorjError class (defined in native).
 */
export interface StorjErrorConstructor {
  new (message: string, code: number, details?: string): StorjErrorInstance;
  readonly prototype: StorjErrorInstance;
}

/**
 * Constructor type for StorjError subclasses (defined in native).
 * Each subclass takes a single `details` argument.
 */
export interface StorjErrorSubclassConstructor {
  new (details?: string): StorjErrorInstance;
  readonly prototype: StorjErrorInstance;
}

/**
 * Complete interface for all native module functions
 */
export interface NativeModule {
  // Access operations
  parseAccess(accessGrant: string): Promise<unknown>;
  requestAccessWithPassphrase(
    satellite: string,
    apiKey: string,
    passphrase: string
  ): Promise<unknown>;
  configRequestAccessWithPassphrase(
    config: unknown,
    satellite: string,
    apiKey: string,
    passphrase: string
  ): Promise<unknown>;
  accessSatelliteAddress(access: unknown): Promise<string>;
  accessSerialize(access: unknown): Promise<string>;
  accessShare(access: unknown, permission: unknown, prefixes: unknown[]): Promise<unknown>;
  accessOverrideEncryptionKey(
    access: unknown,
    bucket: string,
    prefix: string,
    key: unknown
  ): Promise<void>;

  // Project operations
  openProject(access: unknown): Promise<unknown>;
  configOpenProject(config: unknown, access: unknown): Promise<unknown>;
  closeProject(project: unknown): Promise<void>;
  revokeAccess(project: unknown, access: unknown): Promise<void>;

  // Bucket operations
  createBucket(project: unknown, bucketName: string): Promise<unknown>;
  ensureBucket(project: unknown, bucketName: string): Promise<unknown>;
  statBucket(project: unknown, bucketName: string): Promise<unknown>;
  deleteBucket(project: unknown, bucketName: string): Promise<void>;
  deleteBucketWithObjects(project: unknown, bucketName: string): Promise<void>;
  // Bucket iterator operations
  listBucketsCreate(project: unknown, options?: unknown): Promise<unknown>;
  bucketIteratorNext(iterator: unknown): Promise<boolean>;
  bucketIteratorItem(iterator: unknown): Promise<unknown>;
  bucketIteratorErr(iterator: unknown): Promise<unknown>;
  freeBucketIterator(iterator: unknown): Promise<void>;

  // Object operations
  statObject(project: unknown, bucket: string, key: string): Promise<unknown>;
  deleteObject(project: unknown, bucket: string, key: string): Promise<void>;
  // Object iterator operations
  listObjectsCreate(project: unknown, bucket: string, options?: unknown): Promise<unknown>;
  objectIteratorNext(iterator: unknown): Promise<boolean>;
  objectIteratorItem(iterator: unknown): Promise<unknown>;
  objectIteratorErr(iterator: unknown): Promise<unknown>;
  freeObjectIterator(iterator: unknown): Promise<void>;
  copyObject(
    project: unknown,
    srcBucket: string,
    srcKey: string,
    dstBucket: string,
    dstKey: string,
    options?: unknown
  ): Promise<unknown>;
  moveObject(
    project: unknown,
    srcBucket: string,
    srcKey: string,
    dstBucket: string,
    dstKey: string,
    options?: unknown
  ): Promise<void>;
  updateObjectMetadata(
    project: unknown,
    bucket: string,
    key: string,
    metadata: Record<string, string>
  ): Promise<void>;

  // Upload operations
  uploadObject(project: unknown, bucket: string, key: string, options?: unknown): Promise<unknown>;
  uploadWrite(upload: unknown, buffer: Buffer, length: number): Promise<number>;
  uploadCommit(upload: unknown): Promise<void>;
  uploadAbort(upload: unknown): Promise<void>;
  uploadSetCustomMetadata(upload: unknown, metadata: Record<string, string>): Promise<void>;
  uploadInfo(upload: unknown): Promise<unknown>;

  // Download operations
  downloadObject(
    project: unknown,
    bucket: string,
    key: string,
    options?: unknown
  ): Promise<unknown>;
  downloadRead(download: unknown, buffer: Buffer, length: number): Promise<{ bytesRead: number }>;
  downloadInfo(download: unknown): Promise<unknown>;
  closeDownload(download: unknown): Promise<void>;

  // Encryption operations
  deriveEncryptionKey(passphrase: string, salt: Buffer): Promise<unknown>;

  // Multipart operations
  beginUpload(project: unknown, bucket: string, key: string, options?: unknown): Promise<unknown>;
  commitUpload(
    project: unknown,
    bucket: string,
    key: string,
    uploadId: string,
    options?: unknown
  ): Promise<unknown>;
  abortUpload(project: unknown, bucket: string, key: string, uploadId: string): Promise<void>;
  uploadPart(
    project: unknown,
    bucket: string,
    key: string,
    uploadId: string,
    partNumber: number
  ): Promise<unknown>;
  partUploadWrite(partUpload: unknown, buffer: Buffer, length: number): Promise<number>;
  partUploadCommit(partUpload: unknown): Promise<void>;
  partUploadAbort(partUpload: unknown): Promise<void>;
  partUploadSetEtag(partUpload: unknown, etag: string): Promise<void>;
  partUploadInfo(partUpload: unknown): Promise<unknown>;
  // Part iterator operations
  listUploadPartsCreate(
    project: unknown,
    bucket: string,
    key: string,
    uploadId: string,
    options?: unknown
  ): Promise<unknown>;
  partIteratorNext(iterator: unknown): Promise<boolean>;
  partIteratorItem(iterator: unknown): Promise<unknown>;
  partIteratorErr(iterator: unknown): Promise<unknown>;
  freePartIterator(iterator: unknown): Promise<void>;

  // Upload iterator operations
  listUploadsCreate(project: unknown, bucket: string, options?: unknown): Promise<unknown>;
  uploadIteratorNext(iterator: unknown): Promise<boolean>;
  uploadIteratorItem(iterator: unknown): Promise<unknown>;
  uploadIteratorErr(iterator: unknown): Promise<unknown>;
  freeUploadIterator(iterator: unknown): Promise<void>;

  // Edge operations
  edgeRegisterAccess(config: unknown, access: unknown, options?: unknown): Promise<unknown>;
  edgeJoinShareUrl(
    baseUrl: string,
    accessKeyId: string,
    bucket: string,
    key: string,
    options?: unknown
  ): Promise<string>;

  // Debug operations
  internalUniverseIsEmpty(): Promise<boolean>;
  testThrowTypedError(code: number, message: string): Promise<never>;

  // Error class initialization (defined entirely in native via embedded JS).
  // Call once after module load. Optionally pass the caller's Error constructor
  // to ensure instanceof Error works in VM sandboxes (e.g. Jest).
  // Returns an object of constructor functions.
  initErrorClasses(errorBase?: ErrorConstructor): ErrorClassesMap;
}

/**
 * Map of error class names to their constructors,
 * returned by native.initErrorClasses().
 */
export interface ErrorClassesMap {
  StorjError: StorjErrorConstructor;
  InternalError: StorjErrorSubclassConstructor;
  CanceledError: StorjErrorSubclassConstructor;
  InvalidHandleError: StorjErrorSubclassConstructor;
  TooManyRequestsError: StorjErrorSubclassConstructor;
  BandwidthLimitExceededError: StorjErrorSubclassConstructor;
  StorageLimitExceededError: StorjErrorSubclassConstructor;
  SegmentsLimitExceededError: StorjErrorSubclassConstructor;
  PermissionDeniedError: StorjErrorSubclassConstructor;
  BucketNameInvalidError: StorjErrorSubclassConstructor;
  BucketAlreadyExistsError: StorjErrorSubclassConstructor;
  BucketNotEmptyError: StorjErrorSubclassConstructor;
  BucketNotFoundError: StorjErrorSubclassConstructor;
  ObjectKeyInvalidError: StorjErrorSubclassConstructor;
  ObjectNotFoundError: StorjErrorSubclassConstructor;
  UploadDoneError: StorjErrorSubclassConstructor;
  EdgeAuthDialFailedError: StorjErrorSubclassConstructor;
  EdgeRegisterAccessFailedError: StorjErrorSubclassConstructor;
}

/**
 * Load the native module once
 *
 * Search order:
 *   1. Prebuilt: native/prebuilds/<platform>/uplink_native.node  (Option 3: install-prebuilt)
 *   2. Local build: build/Release/uplink_native.node             (Option 1/2: install-source/hybrid)
 */
function loadNativeModule(): NativeModule {
  // Native .node addons must be loaded via require() with dynamic paths.
  // TypeScript's import() cannot load .node files, and this is CommonJS.
  // eslint-disable-next-line @typescript-eslint/no-require-imports, @typescript-eslint/no-var-requires, security/detect-non-literal-require
  const loadAddon = (p: string): NativeModule => require(p) as NativeModule;

  // Determine platform string: darwin-arm64, linux-x64, win32-x64, etc.
  const platform = `${process.platform}-${os.arch()}`;
  const prebuiltPath = path.join(
    __dirname,
    '..',
    '..',
    'native',
    'prebuilds',
    platform,
    'uplink_native.node'
  );
  const buildPath = path.join(__dirname, '..', '..', 'build', 'Release', 'uplink_native.node');

  // Try prebuilt first (Option 3: no compilation needed)
  try {
    return loadAddon(prebuiltPath);
  } catch {
    // Prebuilt not available — fall through to local build
  }

  // Try local build (Option 1/2: compiled via node-gyp)
  // Capture the error here — this is the last resort and its message is meaningful
  let buildError: unknown;
  try {
    return loadAddon(buildPath);
  } catch (err: unknown) {
    buildError = err;
  }

  throw new Error(
    'Failed to load uplink native module. No prebuilt binary found for ' +
      `${platform}, and no local build at build/Release/. ` +
      'Run "make install-prebuilt" (no compiler) or "make install-hybrid" (requires C compiler) ' +
      `or "make install-source" (requires Go + C compiler). Last error: ${buildError}`
  );
}

/**
 * Singleton instance of the native module
 */
export const native: NativeModule = loadNativeModule();

/**
 * Initialize error classes from native embedded JS.
 *
 * Passes the current realm's `Error` constructor so that the native-defined
 * error classes extend it correctly. This ensures `instanceof Error` works
 * in all environments, including Jest VM sandboxes where each test file
 * has its own global scope.
 *
 * The returned object contains all constructor functions:
 *   { StorjError, InternalError, BucketNotFoundError, ... }
 */
export const errorClasses: ErrorClassesMap = native.initErrorClasses(Error);

/**
 * Default export for convenience
 */
export default native;
