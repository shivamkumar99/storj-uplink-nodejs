/**
 * @file download/index.ts
 * @description Download operations for Storj objects
 *
 * Provides the DownloadResultStruct class for downloading objects from Storj buckets.
 */

import { ObjectInfo, ReadResult } from '../types';
import { native } from '../native';

/**
 * Represents an active download operation.
 *
 * Use this class to read data from objects stored in Storj buckets.
 *
 * @example
 * ```typescript
 * const download = await project.downloadObject('my-bucket', 'file.txt');
 * const buffer = Buffer.alloc(1024);
 * const { bytesRead } = await download.read(buffer, buffer.length);
 * console.log('Read', bytesRead, 'bytes');
 * await download.close();
 * ```
 */
export class DownloadResultStruct {
  private _downloadHandle: unknown;
  private _closed: boolean = false;

  /**
   * Creates a new DownloadResultStruct instance.
   *
   * @param downloadHandle - Native download handle from uplink-c
   * @internal
   */
  constructor(downloadHandle: unknown) {
    this._downloadHandle = downloadHandle;
  }

  /**
   * Whether the download has been closed.
   */
  get isClosed(): boolean {
    return this._closed;
  }

  /**
   * Reads data from the download into a buffer.
   *
   * Makes a single call to uplink_download_read. The caller is responsible
   * for looping until all bytes are read, matching the old uplink-nodejs pattern.
   *
   * On EOF the promise rejects with an error that has `bytesRead` attached.
   * The caller should catch this, copy any partial data, then stop reading.
   *
   * @param buffer - Buffer to read data into
   * @param length - Maximum number of bytes to read
   * @returns Promise resolving to the number of bytes read
   * @throws Error if download is closed, read fails, or EOF is reached
   *
   * @example
   * ```typescript
   * const objInfo = await download.info();
   * const objectSize = objInfo.system.contentLength;
   * const buffer = Buffer.alloc(objectSize);
   * let totalRead = 0;
   * while (totalRead < objectSize) {
   *   const remaining = objectSize - totalRead;
   *   const toRead = Math.min(BUFFER_SIZE, remaining);
   *   const chunk = Buffer.alloc(toRead);
   *   try {
   *     const { bytesRead } = await download.read(chunk, toRead);
   *     chunk.copy(buffer, totalRead, 0, bytesRead);
   *     totalRead += bytesRead;
   *   } catch (err) {
   *     // EOF or error — copy any partial data from err.bytesRead
   *     break;
   *   }
   * }
   * ```
   */
  async read(buffer: Buffer, length: number): Promise<ReadResult> {
    if (this._closed) {
      throw new Error('Download is closed');
    }

    if (!Buffer.isBuffer(buffer)) {
      throw new TypeError('First argument must be a Buffer');
    }

    if (typeof length !== 'number' || length < 0) {
      throw new TypeError('Length must be a non-negative number');
    }

    if (length > buffer.length) {
      throw new RangeError('Length exceeds buffer size');
    }

    // Native makes a single uplink_download_read call.
    // On success: resolves with { bytesRead: number }
    // On EOF/error: rejects with error (error.bytesRead has partial bytes read)
    const result = (await native.downloadRead(this._downloadHandle, buffer, length)) as {
      bytesRead: number;
    };
    return { bytesRead: result.bytesRead };
  }

  /**
   * Gets information about the downloaded object.
   *
   * @returns Promise resolving to object information
   * @throws Error if download is closed or info retrieval fails
   *
   * @example
   * ```typescript
   * const info = await download.info();
   * console.log('Object key:', info.key);
   * console.log('Size:', info.system.contentLength, 'bytes');
   * ```
   */
  async info(): Promise<ObjectInfo> {
    if (this._closed) {
      throw new Error('Download is closed');
    }

    const result = (await native.downloadInfo(this._downloadHandle)) as {
      key: string;
      isPrefix: boolean;
      system: { created: number; expires: number; contentLength: number };
      custom: Record<string, string>;
    };

    return {
      key: result.key,
      isPrefix: result.isPrefix,
      system: {
        created: result.system.created,
        expires: result.system.expires > 0 ? result.system.expires : null,
        contentLength: result.system.contentLength,
      },
      custom: (result.custom as Record<string, string> | null) ?? {},
    };
  }

  /**
   * Closes the download stream.
   *
   * Always close downloads when done to free resources.
   *
   * @returns Promise resolving when the download is closed
   * @throws Error if close fails
   *
   * @example
   * ```typescript
   * const download = await project.downloadObject('my-bucket', 'file.txt');
   * try {
   *     // ... read data ...
   * } finally {
   *     await download.close();
   * }
   * ```
   */
  async close(): Promise<void> {
    if (this._closed) {
      return; // Already closed, no-op
    }

    await native.closeDownload(this._downloadHandle);
    this._closed = true;
  }
}
