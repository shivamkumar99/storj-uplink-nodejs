/**
 * @file upload/index.ts
 * @brief UploadResultStruct class for uplink-nodejs
 *
 * Provides TypeScript wrapper for upload operations.
 */

import type { ObjectInfo, CustomMetadata } from '../types';
import { native } from '../native';

/** Native handle type */
type UploadHandle = unknown;

/**
 * Represents an in-progress upload to Storj.
 *
 * Use `write()` to upload data, then either `commit()` to finalize
 * or `abort()` to cancel the upload.
 */
export class UploadResultStruct {
  private readonly _handle: UploadHandle;
  private _isActive: boolean = true;

  /**
   * Create a new UploadResultStruct from a native handle
   * @internal
   */
  constructor(handle: UploadHandle) {
    if (handle == null) {
      throw new TypeError('Invalid upload handle');
    }
    this._handle = handle;
  }

  /**
   * Get the internal handle (for internal use only)
   * @internal
   */
  get _nativeHandle(): UploadHandle {
    this.validateActive();
    return this._handle;
  }

  /**
   * Check if the upload is still active
   */
  get isActive(): boolean {
    return this._isActive;
  }

  /**
   * Validate that the upload is still active
   * @throws Error if upload is finalized
   */
  private validateActive(): void {
    if (!this._isActive) {
      throw new Error('Upload is already finalized');
    }
  }

  /**
   * Validate buffer input
   * @param buffer - Buffer to validate
   * @throws TypeError if buffer is invalid
   */
  private validateBuffer(buffer: Buffer): void {
    if (!Buffer.isBuffer(buffer)) {
      throw new TypeError('buffer must be a Buffer');
    }
  }

  /**
   * Write data to the upload.
   *
   * Can be called multiple times to upload data in chunks.
   *
   * @param buffer - Data buffer to write
   * @param length - Optional number of bytes to write (defaults to buffer length)
   * @returns Promise resolving to the number of bytes written
   * @throws TypeError if buffer is invalid
   * @throws Error if upload is finalized
   *
   * @example
   * ```typescript
   * const upload = await project.uploadObject('my-bucket', 'file.txt');
   *
   * // Write in chunks
   * const chunk1 = Buffer.from('Hello, ');
   * const chunk2 = Buffer.from('World!');
   *
   * await upload.write(chunk1, chunk1.length);
   * await upload.write(chunk2, chunk2.length);
   * await upload.commit();
   * ```
   */
  async write(buffer: Buffer, length?: number): Promise<number> {
    this.validateActive();
    this.validateBuffer(buffer);

    const writeLength = length ?? buffer.length;
    if (writeLength < 0 || writeLength > buffer.length) {
      throw new RangeError('length must be between 0 and buffer.length');
    }

    // If length is less than buffer length, slice the buffer
    const dataToWrite = writeLength < buffer.length ? buffer.subarray(0, writeLength) : buffer;

    const bytesWritten = await native.uploadWrite(this._handle, dataToWrite, writeLength);
    return bytesWritten as number;
  }

  /**
   * Set custom metadata on the upload.
   *
   * Must be called before `commit()`. Metadata keys and values
   * must be strings.
   *
   * @param customMetadata - Object with string key-value pairs
   * @returns Promise resolving when metadata is set
   * @throws TypeError if metadata is invalid
   * @throws Error if upload is finalized
   *
   * @example
   * ```typescript
   * const upload = await project.uploadObject('my-bucket', 'photo.jpg');
   * await upload.setCustomMetadata({
   *   'Content-Type': 'image/jpeg',
   *   'X-Custom-Tag': 'vacation'
   * });
   * await upload.write(imageBuffer, imageBuffer.length);
   * await upload.commit();
   * ```
   */
  async setCustomMetadata(customMetadata: CustomMetadata): Promise<void> {
    this.validateActive();

    if (customMetadata == null || typeof customMetadata !== 'object') {
      throw new TypeError('customMetadata must be an object');
    }

    // Validate all values are strings
    for (const [key, value] of Object.entries(customMetadata)) {
      if (typeof key !== 'string' || typeof value !== 'string') {
        throw new TypeError('All metadata keys and values must be strings');
      }
    }

    return native.uploadSetCustomMetadata(this._handle, customMetadata);
  }

  /**
   * Commit (finalize) the upload.
   *
   * Makes the object available for download. After calling this,
   * no other methods should be called on this upload instance.
   *
   * @returns Promise resolving when the upload is committed
   * @throws Error if upload is already finalized
   *
   * @example
   * ```typescript
   * const upload = await project.uploadObject('my-bucket', 'file.txt');
   * await upload.write(Buffer.from('Hello, World!'), 13);
   * await upload.commit();
   * // Object is now available for download
   * ```
   */
  async commit(): Promise<void> {
    this.validateActive();

    await native.uploadCommit(this._handle);
    this._isActive = false;
  }

  /**
   * Abort the upload.
   *
   * Cancels the upload and removes any partial data.
   * After calling this, no other methods should be called
   * on this upload instance.
   *
   * @returns Promise resolving when the upload is aborted
   * @throws Error if upload is already finalized
   *
   * @example
   * ```typescript
   * const upload = await project.uploadObject('my-bucket', 'file.txt');
   * try {
   *   await upload.write(data, data.length);
   *   await upload.commit();
   * } catch (error) {
   *   await upload.abort();
   *   throw error;
   * }
   * ```
   */
  async abort(): Promise<void> {
    this.validateActive();

    await native.uploadAbort(this._handle);
    this._isActive = false;
  }

  /**
   * Get upload information.
   *
   * Returns object information for the upload in progress.
   *
   * @returns Promise resolving to object info
   * @throws Error if upload is finalized
   *
   * @example
   * ```typescript
   * const upload = await project.uploadObject('my-bucket', 'file.txt');
   * await upload.write(data, data.length);
   *
   * const info = await upload.info();
   * console.log(`Uploading: ${info.key}`);
   * ```
   */
  async info(): Promise<ObjectInfo> {
    this.validateActive();
    return native.uploadInfo(this._handle) as Promise<ObjectInfo>;
  }
}
