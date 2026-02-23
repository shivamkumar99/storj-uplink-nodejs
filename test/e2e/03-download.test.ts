/**
 * @file test/e2e/03-download.test.ts
 * @brief E2E: Download operations — full, partial, chunked, info
 *
 * Downloads objects uploaded by 02-upload.test.ts. When running
 * individually, uploads its own prerequisite objects.
 */

import { DownloadResultStruct } from '../../src';
import { hasCredentials } from '../helpers/credentials';
import {
  getE2eContext,
  uploadTestObject,
  type E2eContext,
} from './helpers/e2e-context';

describe('E2E: Download Operations', () => {
  const runTest = hasCredentials ? it : it.skip;
  let ctx: E2eContext;

  // Known content for download tests
  const downloadKey = 'e2e-download-test.bin';
  const downloadContent = Buffer.alloc(1024);
  for (let i = 0; i < 1024; i++) {
    downloadContent[i] = i % 256;
  }

  beforeAll(async () => {
    if (!hasCredentials) {
      console.log('⏭️  Skipping E2E download tests: No valid credentials configured');
      return;
    }
    ctx = await getE2eContext();

    // Ensure download test object exists
    try {
      await ctx.project.statObject(ctx.bucketName, downloadKey);
    } catch (_) {
      await uploadTestObject(ctx, downloadKey, downloadContent);
    }
  });

  describe('Full Download', () => {
    runTest('should download entire object and verify content', async () => {
      const download = await ctx.project.downloadObject(ctx.bucketName, downloadKey);
      expect(download).toBeInstanceOf(DownloadResultStruct);

      const buffer = Buffer.alloc(downloadContent.length);
      const result = await download.read(buffer, buffer.length);
      await download.close();

      expect(result.bytesRead).toBe(downloadContent.length);
      expect(buffer.subarray(0, result.bytesRead).equals(downloadContent)).toBe(true);
    });
  });

  describe('Download Info', () => {
    runTest('should return object info from download handle', async () => {
      const download = await ctx.project.downloadObject(ctx.bucketName, downloadKey);

      try {
        const info = await download.info();
        expect(info).toBeDefined();
        expect(info.key).toBe(downloadKey);
        expect(info.system.contentLength).toBe(downloadContent.length);
        expect(typeof info.system.created).toBe('number');
        expect(info.system.created).toBeGreaterThan(0);
      } finally {
        await download.close();
      }
    });
  });

  describe('Partial Download (offset + length)', () => {
    runTest('should download a partial range of the object', async () => {
      const offset = 100;
      const length = 200;

      const download = await ctx.project.downloadObject(ctx.bucketName, downloadKey, {
        offset,
        length,
      });

      try {
        const buffer = Buffer.alloc(length);
        const result = await download.read(buffer, length);

        expect(result.bytesRead).toBe(length);

        const expected = downloadContent.subarray(offset, offset + length);
        expect(buffer.subarray(0, result.bytesRead).equals(expected)).toBe(true);
      } finally {
        await download.close();
      }
    });

    runTest('should download from offset to end', async () => {
      const offset = 500;
      const expectedLength = downloadContent.length - offset;

      const download = await ctx.project.downloadObject(ctx.bucketName, downloadKey, {
        offset,
        length: -1,
      });

      try {
        const info = await download.info();
        const objectSize = info.system.contentLength - offset;
        const buffer = Buffer.alloc(objectSize);
        const result = await download.read(buffer, objectSize);

        expect(result.bytesRead).toBe(expectedLength);

        const expected = downloadContent.subarray(offset);
        expect(buffer.subarray(0, result.bytesRead).equals(expected)).toBe(true);
      } finally {
        await download.close();
      }
    });
  });

  describe('Chunked Download', () => {
    runTest('should read object in multiple chunks', async () => {
      const download = await ctx.project.downloadObject(ctx.bucketName, downloadKey);

      try {
        const info = await download.info();
        const objectSize = info.system.contentLength;
        const CHUNK_SIZE = 256;
        const chunks: Buffer[] = [];
        let totalBytesRead = 0;
        let zeroReadRetries = 0;
        const MAX_ZERO_RETRIES = 100;

        while (totalBytesRead < objectSize) {
          const remaining = objectSize - totalBytesRead;
          const toRead = Math.min(CHUNK_SIZE, remaining);
          const buffer = Buffer.alloc(toRead);

          try {
            const result = await download.read(buffer, toRead);
            if (result.bytesRead > 0) {
              chunks.push(buffer.subarray(0, result.bytesRead));
              totalBytesRead += result.bytesRead;
              zeroReadRetries = 0;
            } else {
              zeroReadRetries++;
              if (zeroReadRetries >= MAX_ZERO_RETRIES) break;
            }
          } catch (err: unknown) {
            const errObj = err as Record<string, unknown>;
            const partialBytes = typeof errObj.bytesRead === 'number' ? errObj.bytesRead : 0;
            if (partialBytes > 0) {
              chunks.push(buffer.subarray(0, partialBytes));
              totalBytesRead += partialBytes;
            }
            break;
          }
        }

        expect(totalBytesRead).toBe(downloadContent.length);

        const reconstructed = Buffer.concat(chunks);
        expect(reconstructed.equals(downloadContent)).toBe(true);
      } finally {
        await download.close();
      }
    });
  });

  describe('Download Non-Existent Object', () => {
    runTest('should reject download of non-existent object', async () => {
      await expect(
        ctx.project.downloadObject(ctx.bucketName, 'non-existent-object-xyz-999')
      ).rejects.toThrow();
    });
  });
});
