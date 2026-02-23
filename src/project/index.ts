/**
 * @file project/index.ts
 * @brief ProjectResultStruct class for uplink-nodejs
 *
 * Provides TypeScript wrapper for project operations.
 */

import type {
  BucketInfo,
  ListBucketsOptions,
  ObjectInfo,
  ListObjectsOptions,
  CopyObjectOptions,
  MoveObjectOptions,
  UploadOptions,
  DownloadOptions,
} from '../types';
import { UploadResultStruct } from '../upload';
import { DownloadResultStruct } from '../download';
import { native } from '../native';

/** Native handle type */
type ProjectHandle = unknown;

/**
 * Represents an open project on Storj.
 *
 * A project is the main entry point for bucket and object operations.
 * Always call `close()` when done to free resources.
 */
export class ProjectResultStruct {
  private readonly _handle: ProjectHandle;
  private _isOpen: boolean = true;

  /**
   * Create a new ProjectResultStruct from a native handle
   * @internal
   */
  constructor(handle: ProjectHandle) {
    if (handle == null) {
      throw new TypeError('Invalid project handle');
    }
    this._handle = handle;
  }

  /**
   * Get the internal handle (for internal use only)
   * @internal
   */
  get _nativeHandle(): ProjectHandle {
    this.validateOpen();
    return this._handle;
  }

  /**
   * Check if the project is still open
   */
  get isOpen(): boolean {
    return this._isOpen;
  }

  /**
   * Close the project and release resources.
   *
   * After calling this method, no other methods should be called
   * on this project instance.
   *
   * @example
   * ```typescript
   * const project = await access.openProject();
   * try {
   *   // Use project...
   * } finally {
   *   await project.close();
   * }
   * ```
   */
  async close(): Promise<void> {
    if (!this._isOpen) {
      return; // Already closed
    }

    await native.closeProject(this._handle);
    this._isOpen = false;
  }

  /**
   * Revoke an access grant.
   *
   * Revokes the API key embedded in the provided access grant.
   * This is useful when you want to invalidate a previously shared access.
   *
   * @param access - The access grant to revoke (AccessResultStruct)
   * @returns Promise resolving when the access is revoked
   * @throws Error if revocation fails
   *
   * @example
   * ```typescript
   * // Create a shared access with limited permissions
   * const sharedAccess = await access.share(permission, prefixes);
   *
   * // Give the shared access to someone...
   *
   * // Later, revoke the access when no longer needed
   * await project.revokeAccess(sharedAccess);
   * ```
   */
  async revokeAccess(access: { _nativeHandle: unknown }): Promise<void> {
    this.validateOpen();

    if (access == null || access._nativeHandle == null) {
      throw new TypeError('Invalid access: must be an AccessResultStruct');
    }

    await native.revokeAccess(this._handle, access._nativeHandle);
  }

  /**
   * Validate that the project is still open
   * @throws Error if project is closed
   */
  private validateOpen(): void {
    if (!this._isOpen) {
      throw new Error('Project is closed');
    }
  }

  /**
   * Validate a bucket name
   * @param name - Bucket name to validate
   * @throws TypeError if bucket name is invalid
   */
  private validateBucketName(name: string): void {
    if (!name || typeof name !== 'string') {
      throw new TypeError('Bucket name must be a non-empty string');
    }

    if (name.length < 3 || name.length > 63) {
      throw new TypeError('Bucket name must be 3-63 characters');
    }

    // Must be lowercase alphanumeric or hyphens, starting and ending with alphanumeric
    // Length already validated above (3-63), so start+end checks are safe
    if (!/^[a-z0-9]/.test(name) || !/[a-z0-9]$/.test(name) || !/^[a-z0-9-]+$/.test(name)) {
      throw new TypeError(
        'Bucket name must be lowercase alphanumeric or hyphens, ' +
          'starting and ending with alphanumeric'
      );
    }
  }

  // ========== Bucket Operations ==========

  /**
   * Create a new bucket.
   *
   * @param name - Bucket name (3-63 lowercase alphanumeric chars or hyphens)
   * @returns Promise resolving to the created bucket info
   * @throws TypeError if bucket name is invalid
   * @throws Error if bucket already exists
   *
   * @example
   * ```typescript
   * const bucket = await project.createBucket('my-bucket');
   * console.log(`Created bucket: ${bucket.name}`);
   * ```
   */
  async createBucket(name: string): Promise<BucketInfo> {
    this.validateOpen();
    this.validateBucketName(name);
    return native.createBucket(this._handle, name) as Promise<BucketInfo>;
  }

  /**
   * Ensure a bucket exists, creating it if necessary.
   *
   * This is idempotent - calling it multiple times has the same effect
   * as calling it once.
   *
   * @param name - Bucket name (3-63 lowercase alphanumeric chars or hyphens)
   * @returns Promise resolving to the bucket info
   * @throws TypeError if bucket name is invalid
   *
   * @example
   * ```typescript
   * const bucket = await project.ensureBucket('my-bucket');
   * // Bucket now exists, whether it was just created or already existed
   * ```
   */
  async ensureBucket(name: string): Promise<BucketInfo> {
    this.validateOpen();
    this.validateBucketName(name);
    return native.ensureBucket(this._handle, name) as Promise<BucketInfo>;
  }

  /**
   * Get information about a bucket.
   *
   * @param name - Bucket name
   * @returns Promise resolving to the bucket info
   * @throws TypeError if bucket name is invalid
   * @throws Error if bucket does not exist
   *
   * @example
   * ```typescript
   * const bucket = await project.statBucket('my-bucket');
   * console.log(`Bucket created: ${bucket.created}`);
   * ```
   */
  async statBucket(name: string): Promise<BucketInfo> {
    this.validateOpen();
    this.validateBucketName(name);
    return native.statBucket(this._handle, name) as Promise<BucketInfo>;
  }

  /**
   * Delete an empty bucket.
   *
   * The bucket must be empty before it can be deleted.
   * Use `deleteBucketWithObjects` to delete a bucket and all its contents.
   *
   * @param name - Bucket name
   * @returns Promise resolving when the bucket is deleted
   * @throws TypeError if bucket name is invalid
   * @throws Error if bucket does not exist or is not empty
   *
   * @example
   * ```typescript
   * await project.deleteBucket('my-bucket');
   * ```
   */
  async deleteBucket(name: string): Promise<void> {
    this.validateOpen();
    this.validateBucketName(name);
    return native.deleteBucket(this._handle, name);
  }

  /**
   * Delete a bucket and all its objects.
   *
   * WARNING: This will permanently delete all objects in the bucket!
   *
   * @param name - Bucket name
   * @returns Promise resolving when the bucket and all objects are deleted
   * @throws TypeError if bucket name is invalid
   * @throws Error if bucket does not exist
   *
   * @example
   * ```typescript
   * // Delete bucket and all its contents
   * await project.deleteBucketWithObjects('my-bucket');
   * ```
   */
  async deleteBucketWithObjects(name: string): Promise<void> {
    this.validateOpen();
    this.validateBucketName(name);
    return native.deleteBucketWithObjects(this._handle, name);
  }

  /**
   * List all buckets in the project.
   *
   * @param options - Optional listing options
   * @returns Promise resolving to an array of bucket info
   *
   * @example
   * ```typescript
   * const buckets = await project.listBuckets();
   * for (const bucket of buckets) {
   *   console.log(`- ${bucket.name} (created: ${bucket.created})`);
   * }
   * ```
   */
  async listBuckets(options?: ListBucketsOptions): Promise<BucketInfo[]> {
    this.validateOpen();
    const iterator = await native.listBucketsCreate(this._handle, options);
    const buckets: BucketInfo[] = [];
    try {
      while (await native.bucketIteratorNext(iterator)) {
        const bucket = await native.bucketIteratorItem(iterator);
        buckets.push(bucket as BucketInfo);
      }
      const err = await native.bucketIteratorErr(iterator);
      if (err) {
        throw err;
      }
    } finally {
      await native.freeBucketIterator(iterator);
    }
    return buckets;
  }

  // ========== Object Operations ==========

  /**
   * Validate an object key
   * @param key - Object key to validate
   * @throws TypeError if object key is invalid
   */
  private validateObjectKey(key: string): void {
    if (!key || typeof key !== 'string') {
      throw new TypeError('Object key must be a non-empty string');
    }
  }

  /**
   * Get information about an object.
   *
   * @param bucketName - Name of the bucket containing the object
   * @param objectKey - Object key (path)
   * @returns Promise resolving to the object info
   * @throws TypeError if bucket name or object key is invalid
   * @throws Error if object does not exist
   *
   * @example
   * ```typescript
   * const info = await project.statObject('my-bucket', 'path/to/file.txt');
   * console.log(`Size: ${info.system.contentLength} bytes`);
   * console.log(`Created: ${info.system.created}`);
   * ```
   */
  async statObject(bucketName: string, objectKey: string): Promise<ObjectInfo> {
    this.validateOpen();
    this.validateBucketName(bucketName);
    this.validateObjectKey(objectKey);
    return native.statObject(this._handle, bucketName, objectKey) as Promise<ObjectInfo>;
  }

  /**
   * Delete an object.
   *
   * @param bucketName - Name of the bucket containing the object
   * @param objectKey - Object key (path)
   * @returns Promise resolving when the object is deleted
   * @throws TypeError if bucket name or object key is invalid
   * @throws Error if object does not exist
   *
   * @example
   * ```typescript
   * await project.deleteObject('my-bucket', 'path/to/file.txt');
   * ```
   */
  async deleteObject(bucketName: string, objectKey: string): Promise<void> {
    this.validateOpen();
    this.validateBucketName(bucketName);
    this.validateObjectKey(objectKey);
    return native.deleteObject(this._handle, bucketName, objectKey);
  }

  /**
   * List objects in a bucket.
   *
   * @param bucketName - Name of the bucket to list objects from
   * @param options - Optional listing options
   * @returns Promise resolving to an array of object info
   * @throws TypeError if bucket name is invalid
   *
   * @example
   * ```typescript
   * // List all objects
   * const objects = await project.listObjects('my-bucket');
   *
   * // List with prefix and pagination
   * const objects = await project.listObjects('my-bucket', {
   *   prefix: 'photos/',
   *   recursive: true,
   *   system: true,
   *   custom: true
   * });
   *
   * for (const obj of objects) {
   *   if (obj.isPrefix) {
   *     console.log(`[DIR] ${obj.key}`);
   *   } else {
   *     console.log(`${obj.key} (${obj.system.contentLength} bytes)`);
   *   }
   * }
   * ```
   */
  async listObjects(bucketName: string, options?: ListObjectsOptions): Promise<ObjectInfo[]> {
    this.validateOpen();
    this.validateBucketName(bucketName);
    const iterator = await native.listObjectsCreate(this._handle, bucketName, options);
    const objects: ObjectInfo[] = [];
    try {
      while (await native.objectIteratorNext(iterator)) {
        const obj = await native.objectIteratorItem(iterator);
        objects.push(obj as ObjectInfo);
      }
      const err = await native.objectIteratorErr(iterator);
      if (err) {
        throw err;
      }
    } finally {
      await native.freeObjectIterator(iterator);
    }
    return objects;
  }

  /**
   * Copy an object to a new location.
   *
   * @param oldBucket - Source bucket name
   * @param oldKey - Source object key
   * @param newBucket - Destination bucket name
   * @param newKey - Destination object key
   * @param options - Optional copy options
   * @returns Promise resolving to the new object info
   * @throws TypeError if bucket name or object key is invalid
   * @throws Error if source object does not exist
   *
   * @example
   * ```typescript
   * // Copy within same bucket
   * const copied = await project.copyObject(
   *   'my-bucket', 'original.txt',
   *   'my-bucket', 'copy.txt'
   * );
   *
   * // Copy to different bucket
   * const copied = await project.copyObject(
   *   'source-bucket', 'file.txt',
   *   'dest-bucket', 'backup/file.txt'
   * );
   * ```
   */
  async copyObject(
    oldBucket: string,
    oldKey: string,
    newBucket: string,
    newKey: string,
    options?: CopyObjectOptions
  ): Promise<ObjectInfo> {
    this.validateOpen();
    this.validateBucketName(oldBucket);
    this.validateObjectKey(oldKey);
    this.validateBucketName(newBucket);
    this.validateObjectKey(newKey);
    return native.copyObject(
      this._handle,
      oldBucket,
      oldKey,
      newBucket,
      newKey,
      options
    ) as Promise<ObjectInfo>;
  }

  /**
   * Move (rename) an object to a new location.
   *
   * @param oldBucket - Source bucket name
   * @param oldKey - Source object key
   * @param newBucket - Destination bucket name
   * @param newKey - Destination object key
   * @param options - Optional move options
   * @returns Promise resolving when the move is complete
   * @throws TypeError if bucket name or object key is invalid
   * @throws Error if source object does not exist
   *
   * @example
   * ```typescript
   * // Rename within same bucket
   * await project.moveObject(
   *   'my-bucket', 'old-name.txt',
   *   'my-bucket', 'new-name.txt'
   * );
   *
   * // Move to different bucket
   * await project.moveObject(
   *   'source-bucket', 'file.txt',
   *   'archive-bucket', 'archived/file.txt'
   * );
   * ```
   */
  async moveObject(
    oldBucket: string,
    oldKey: string,
    newBucket: string,
    newKey: string,
    options?: MoveObjectOptions
  ): Promise<void> {
    this.validateOpen();
    this.validateBucketName(oldBucket);
    this.validateObjectKey(oldKey);
    this.validateBucketName(newBucket);
    this.validateObjectKey(newKey);
    return native.moveObject(this._handle, oldBucket, oldKey, newBucket, newKey, options);
  }

  /**
   * Update object custom metadata.
   *
   * Replaces the existing custom metadata with new metadata.
   * System metadata (created, expires, contentLength) cannot be modified.
   *
   * @param bucketName - Bucket name containing the object
   * @param objectKey - Object key to update
   * @param metadata - New custom metadata (replaces existing)
   * @returns Promise resolving when metadata is updated
   * @throws TypeError if bucket name or object key is invalid
   * @throws Error if object does not exist
   *
   * @example
   * ```typescript
   * // Update custom metadata
   * await project.updateObjectMetadata('my-bucket', 'file.txt', {
   *   'Content-Type': 'text/plain',
   *   'X-Author': 'John Doe',
   *   'X-Version': '2'
   * });
   *
   * // Clear custom metadata by passing empty object
   * await project.updateObjectMetadata('my-bucket', 'file.txt', {});
   * ```
   */
  async updateObjectMetadata(
    bucketName: string,
    objectKey: string,
    metadata: Record<string, string>
  ): Promise<void> {
    this.validateOpen();
    this.validateBucketName(bucketName);
    this.validateObjectKey(objectKey);

    if (metadata == null || typeof metadata !== 'object') {
      throw new TypeError('metadata must be an object');
    }

    return native.updateObjectMetadata(this._handle, bucketName, objectKey, metadata);
  }

  // ========== Upload Operations ==========

  /**
   * Start uploading an object.
   *
   * Returns an UploadResultStruct for writing data and finalizing the upload.
   *
   * @param bucketName - Name of the bucket to upload to
   * @param objectKey - Object key (path) for the uploaded object
   * @param options - Optional upload options (e.g., expiration)
   * @returns Promise resolving to an UploadResultStruct
   * @throws TypeError if bucket name or object key is invalid
   *
   * @example
   * ```typescript
   * // Simple upload
   * const upload = await project.uploadObject('my-bucket', 'file.txt');
   * await upload.write(Buffer.from('Hello, World!'), 13);
   * await upload.commit();
   *
   * // Upload with expiration
   * const upload = await project.uploadObject('my-bucket', 'temp.txt', {
   *   expires: new Date(Date.now() + 24 * 60 * 60 * 1000) // 24 hours
   * });
   * await upload.write(data, data.length);
   * await upload.commit();
   *
   * // Upload with custom metadata
   * const upload = await project.uploadObject('my-bucket', 'photo.jpg');
   * await upload.setCustomMetadata({
   *   'Content-Type': 'image/jpeg',
   *   'X-Custom-Tag': 'vacation'
   * });
   * await upload.write(imageBuffer, imageBuffer.length);
   * await upload.commit();
   * ```
   */
  async uploadObject(
    bucketName: string,
    objectKey: string,
    options?: UploadOptions
  ): Promise<UploadResultStruct> {
    this.validateOpen();
    this.validateBucketName(bucketName);
    this.validateObjectKey(objectKey);

    const handle = await native.uploadObject(this._handle, bucketName, objectKey, options);
    return new UploadResultStruct(handle);
  }

  /**
   * Start a download from a bucket.
   *
   * Opens a download stream for the specified object. Use the returned
   * DownloadResultStruct to read data and close the download when done.
   *
   * @param bucketName - Name of the bucket to download from
   * @param objectKey - Object key (path) to download
   * @param options - Optional download options (offset, length)
   * @returns Promise resolving to a DownloadResultStruct
   * @throws TypeError if bucket name or object key is invalid
   *
   * @example
   * ```typescript
   * // Download entire object
   * const download = await project.downloadObject('my-bucket', 'file.txt');
   * const buffer = Buffer.alloc(1024);
   * let totalBytes = 0;
   * let result;
   * do {
   *   result = await download.read(buffer, buffer.length);
   *   totalBytes += result.bytesRead;
   * } while (result.bytesRead > 0);
   * await download.close();
   *
   * // Download with offset and length (partial download)
   * const download = await project.downloadObject('my-bucket', 'large-file.bin', {
   *   offset: 1000,
   *   length: 500
   * });
   * const chunk = Buffer.alloc(500);
   * await download.read(chunk, chunk.length);
   * await download.close();
   * ```
   */
  async downloadObject(
    bucketName: string,
    objectKey: string,
    options?: DownloadOptions
  ): Promise<DownloadResultStruct> {
    this.validateOpen();
    this.validateBucketName(bucketName);
    this.validateObjectKey(objectKey);

    const result = (await native.downloadObject(this._handle, bucketName, objectKey, options)) as {
      downloadHandle: unknown;
    };
    return new DownloadResultStruct(result.downloadHandle);
  }
}
