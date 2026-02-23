/**
 * @file multipart/index.ts
 * @brief Multipart upload operations for uplink-nodejs
 *
 * Provides TypeScript wrappers for multipart upload operations.
 */

import type {
  UploadInfo,
  PartInfo,
  ObjectInfo,
  BeginUploadOptions,
  CommitUploadOptions,
  ListUploadPartsOptions,
  ListUploadsOptions,
} from '../types';
import { native } from '../native';

/** Native handle types */
type ProjectHandle = unknown;
type PartUploadHandle = unknown;

/** Error messages used in multiple places */
const ERR_PART_UPLOAD_CLOSED = 'Part upload is closed';
const ERR_MULTIPART_NOT_ACTIVE = 'Multipart upload is no longer active';

/**
 * Represents an active part upload within a multipart upload.
 *
 * Use this class to write data to individual parts of a multipart upload.
 */
export class PartUploadResultStruct {
  private readonly _handle: PartUploadHandle;
  private _isOpen: boolean = true;

  /**
   * Create a new PartUploadResultStruct
   * @internal
   */
  constructor(handle: PartUploadHandle) {
    if (handle == null) {
      throw new TypeError('Invalid part upload handle');
    }
    this._handle = handle;
  }

  /**
   * Check if the part upload is still open
   */
  get isOpen(): boolean {
    return this._isOpen;
  }

  /**
   * Write data to the part.
   *
   * @param buffer - Buffer containing data to write
   * @param length - Number of bytes to write from buffer
   * @returns Promise resolving to number of bytes written
   */
  async write(buffer: Buffer, length: number): Promise<number> {
    if (!this._isOpen) {
      throw new Error(ERR_PART_UPLOAD_CLOSED);
    }

    if (!Buffer.isBuffer(buffer)) {
      throw new TypeError('buffer must be a Buffer');
    }

    if (typeof length !== 'number' || length < 0) {
      throw new TypeError('length must be a non-negative number');
    }

    if (length > buffer.length) {
      throw new RangeError('length exceeds buffer size');
    }

    const bytesWritten = await native.partUploadWrite(this._handle, buffer, length);
    return bytesWritten as number;
  }

  /**
   * Commit the part upload.
   *
   * After committing, the part cannot be modified.
   */
  async commit(): Promise<void> {
    if (!this._isOpen) {
      throw new Error('Part upload is already closed');
    }

    await native.partUploadCommit(this._handle);
    this._isOpen = false;
  }

  /**
   * Abort the part upload.
   *
   * Discards any data written to this part.
   */
  async abort(): Promise<void> {
    if (!this._isOpen) {
      throw new Error('Part upload is already closed');
    }

    await native.partUploadAbort(this._handle);
    this._isOpen = false;
  }

  /**
   * Set the ETag for this part.
   *
   * @param etag - ETag string
   */
  async setEtag(etag: string): Promise<void> {
    if (!this._isOpen) {
      throw new Error(ERR_PART_UPLOAD_CLOSED);
    }

    if (typeof etag !== 'string') {
      throw new TypeError('etag must be a string');
    }

    return native.partUploadSetEtag(this._handle, etag);
  }

  /**
   * Get information about this part upload.
   *
   * @returns Promise resolving to part information
   */
  async info(): Promise<PartInfo> {
    if (!this._isOpen) {
      throw new Error(ERR_PART_UPLOAD_CLOSED);
    }

    return native.partUploadInfo(this._handle) as Promise<PartInfo>;
  }
}

/**
 * Helper class for multipart upload operations.
 *
 * Use this class to perform multipart uploads, which allow uploading
 * large files in parts that can be uploaded in parallel or resumed
 * after failure.
 */
export class MultipartUpload {
  private readonly _projectHandle: ProjectHandle;
  private readonly _bucket: string;
  private readonly _key: string;
  private readonly _uploadId: string;
  private _isAborted: boolean = false;
  private _isCommitted: boolean = false;

  /**
   * Create a new MultipartUpload
   * @internal Use ProjectResultStruct.beginMultipartUpload() instead
   */
  constructor(projectHandle: ProjectHandle, bucket: string, key: string, uploadId: string) {
    this._projectHandle = projectHandle;
    this._bucket = bucket;
    this._key = key;
    this._uploadId = uploadId;
  }

  /**
   * Get the upload ID
   */
  get uploadId(): string {
    return this._uploadId;
  }

  /**
   * Get the bucket name
   */
  get bucket(): string {
    return this._bucket;
  }

  /**
   * Get the object key
   */
  get key(): string {
    return this._key;
  }

  /**
   * Check if the upload is still active
   */
  get isActive(): boolean {
    return !this._isAborted && !this._isCommitted;
  }

  /**
   * Start uploading a part.
   *
   * @param partNumber - Part number (1-based, max 10000)
   * @returns Promise resolving to a PartUploadResultStruct
   */
  async uploadPart(partNumber: number): Promise<PartUploadResultStruct> {
    if (!this.isActive) {
      throw new Error(ERR_MULTIPART_NOT_ACTIVE);
    }

    if (typeof partNumber !== 'number' || partNumber < 1 || partNumber > 10000) {
      throw new RangeError('partNumber must be between 1 and 10000');
    }

    const handle = await native.uploadPart(
      this._projectHandle,
      this._bucket,
      this._key,
      this._uploadId,
      partNumber
    );

    return new PartUploadResultStruct(handle);
  }

  /**
   * Commit the multipart upload.
   *
   * Combines all uploaded parts into a single object.
   *
   * @param options - Optional commit options (custom metadata)
   * @returns Promise resolving to the final object info
   */
  async commit(options?: CommitUploadOptions): Promise<ObjectInfo> {
    if (!this.isActive) {
      throw new Error(ERR_MULTIPART_NOT_ACTIVE);
    }

    const result = (await native.commitUpload(
      this._projectHandle,
      this._bucket,
      this._key,
      this._uploadId,
      options
    )) as ObjectInfo;

    this._isCommitted = true;
    return result;
  }

  /**
   * Abort the multipart upload.
   *
   * Discards all uploaded parts and cleans up resources.
   */
  async abort(): Promise<void> {
    if (!this.isActive) {
      throw new Error(ERR_MULTIPART_NOT_ACTIVE);
    }

    await native.abortUpload(this._projectHandle, this._bucket, this._key, this._uploadId);

    this._isAborted = true;
  }

  /**
   * List all uploaded parts.
   *
   * @param options - Optional list options (cursor for pagination)
   * @returns Promise resolving to array of part info
   */
  async listParts(options?: ListUploadPartsOptions): Promise<PartInfo[]> {
    if (!this.isActive) {
      throw new Error(ERR_MULTIPART_NOT_ACTIVE);
    }

    const iterator = await native.listUploadPartsCreate(
      this._projectHandle,
      this._bucket,
      this._key,
      this._uploadId,
      options
    );
    const parts: PartInfo[] = [];
    try {
      while (await native.partIteratorNext(iterator)) {
        const part = await native.partIteratorItem(iterator);
        parts.push(part as PartInfo);
      }
      const err = await native.partIteratorErr(iterator);
      if (err) {
        throw err;
      }
    } finally {
      await native.freePartIterator(iterator);
    }
    return parts;
  }
}

/**
 * Begin a new multipart upload.
 *
 * @param projectHandle - Native project handle
 * @param bucket - Bucket name
 * @param key - Object key
 * @param options - Optional upload options
 * @returns Promise resolving to a MultipartUpload helper
 */
export async function beginMultipartUpload(
  projectHandle: ProjectHandle,
  bucket: string,
  key: string,
  options?: BeginUploadOptions
): Promise<MultipartUpload> {
  const info = (await native.beginUpload(projectHandle, bucket, key, options)) as UploadInfo;
  return new MultipartUpload(projectHandle, bucket, key, info.uploadId);
}

/**
 * List pending multipart uploads in a bucket.
 *
 * @param projectHandle - Native project handle
 * @param bucket - Bucket name
 * @param options - Optional list options
 * @returns Promise resolving to array of upload info
 */
export async function listMultipartUploads(
  projectHandle: ProjectHandle,
  bucket: string,
  options?: ListUploadsOptions
): Promise<UploadInfo[]> {
  const iterator = await native.listUploadsCreate(projectHandle, bucket, options);
  const uploads: UploadInfo[] = [];
  try {
    while (await native.uploadIteratorNext(iterator)) {
      const upload = await native.uploadIteratorItem(iterator);
      uploads.push(upload as UploadInfo);
    }
    const err = await native.uploadIteratorErr(iterator);
    if (err) {
      throw err;
    }
  } finally {
    await native.freeUploadIterator(iterator);
  }
  return uploads;
}
