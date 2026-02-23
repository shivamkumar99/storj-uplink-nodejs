/**
 * @file test/integration/download.test.ts
 * @brief Integration tests for download operations (info, partial, chunked)
 *
 * Tests the chains:
 *   1. downloadObject → info() → verify matches statObject
 *   2. downloadObject({offset, length}) → read → verify partial
 *   3. downloadObject({offset}) → read → verify remaining
 *   4. downloadObject → read in multiple chunks → verify total
 *
 * Requires environment variables:
 * - TEST_ACCESS_GRANT: Pre-serialized access grant, OR
 * - TEST_SATELLITE, TEST_API_KEY, TEST_PASSPHRASE
 */

import {
  Uplink,
  AccessResultStruct,
  ProjectResultStruct,
  DownloadResultStruct,
} from '../../src';
import { hasAnyCredentials, getAccess } from '../helpers/credentials';

describe('Integration: Download Operations', () => {
  const validCredentials = hasAnyCredentials;
  const runTest = validCredentials ? it : it.skip;
  const bucketName = `int-dl-${Date.now()}`;
  const testKey = 'download-test-file.bin';

  // 1024 bytes of known content
  const testContent = Buffer.alloc(1024);
  for (let i = 0; i < 1024; i++) {
    testContent[i] = i % 256;
  }

  let uplink: Uplink;
  let access: AccessResultStruct;
  let project: ProjectResultStruct;

  beforeAll(async () => {
    if (!validCredentials) {
      console.log('⏭️  Skipping download integration tests: No valid credentials configured');
      return;
    }

    uplink = new Uplink();
    access = await getAccess(uplink);
    project = await access.openProject();
    await project.ensureBucket(bucketName);

    // Upload known content for download tests
    const upload = await project.uploadObject(bucketName, testKey);
    await upload.write(testContent, testContent.length);
    await upload.commit();
  });

  afterAll(async () => {
    if (!validCredentials) return;

    try {
      const objects = await project.listObjects(bucketName, { recursive: true });
      for (const obj of objects) {
        await project.deleteObject(bucketName, obj.key);
      }
      await project.deleteBucket(bucketName);
    } catch (_) {
      // bucket may not exist
    }
    await project.close();
  });

  runTest('should return object info from download', async () => {
    const download = await project.downloadObject(bucketName, testKey);
    expect(download).toBeInstanceOf(DownloadResultStruct);

    try {
      const info = await download.info();
      expect(info).toBeDefined();
      expect(info.key).toBe(testKey);
      expect(info.system.contentLength).toBe(testContent.length);
      // Native layer returns Unix timestamps as integers
      expect(typeof info.system.created).toBe('number');
      expect(info.system.created).toBeGreaterThan(0);

      // Cross-check with statObject
      const stat = await project.statObject(bucketName, testKey);
      expect(info.key).toBe(stat.key);
      expect(info.system.contentLength).toBe(stat.system.contentLength);
    } finally {
      await download.close();
    }
  });

  runTest('should download with offset and length (partial download)', async () => {
    const offset = 100;
    const length = 200;

    const download = await project.downloadObject(bucketName, testKey, {
      offset,
      length,
    });

    try {
      const buffer = Buffer.alloc(length);
      const result = await download.read(buffer, length);

      expect(result.bytesRead).toBe(length);

      // Verify content matches expected slice
      const expected = testContent.subarray(offset, offset + length);
      expect(buffer.subarray(0, result.bytesRead).equals(expected)).toBe(true);
    } finally {
      await download.close();
    }
  });

  runTest('should download with offset only (reads to end)', async () => {
    const offset = 500;
    const expectedLength = testContent.length - offset; // 524 bytes

    const download = await project.downloadObject(bucketName, testKey, {
      offset,
      length: -1, // read all remaining
    });

    try {
      // Use download.info() to get exact size, then read exactly that amount
      // (matching old uplink-nodejs pattern from HelloStorjTS.ts)
      const info = await download.info();
      const objectSize = info.system.contentLength - offset;
      const buffer = Buffer.alloc(objectSize);
      const result = await download.read(buffer, objectSize);

      expect(result.bytesRead).toBe(expectedLength);

      // Verify content matches expected slice
      const expected = testContent.subarray(offset);
      expect(buffer.subarray(0, result.bytesRead).equals(expected)).toBe(true);
    } finally {
      await download.close();
    }
  });

  runTest('should read in multiple chunks', async () => {
    const download = await project.downloadObject(bucketName, testKey);

    try {
      // Get object size first (old uplink-nodejs pattern from HelloStorjTS.ts)
      const info = await download.info();
      const objectSize = info.system.contentLength;

      const CHUNK_SIZE = 256;
      const chunks: Buffer[] = [];
      let totalBytesRead = 0;
      let zeroReadRetries = 0;
      const MAX_ZERO_RETRIES = 100;

      while (totalBytesRead < objectSize) {
        const remaining = objectSize - totalBytesRead;
        // Adjust last chunk to exact remaining size to avoid EOF on the read
        const toRead = Math.min(CHUNK_SIZE, remaining);
        const buffer = Buffer.alloc(toRead);

        try {
          const result = await download.read(buffer, toRead);
          if (result.bytesRead > 0) {
            chunks.push(buffer.subarray(0, result.bytesRead));
            totalBytesRead += result.bytesRead;
            zeroReadRetries = 0;
          } else {
            // Zero-byte read at segment boundary — retry
            zeroReadRetries++;
            if (zeroReadRetries >= MAX_ZERO_RETRIES) break;
          }
        } catch (err: unknown) {
          // EOF — collect any partial bytes attached to the error
          const errObj = err as Record<string, unknown>;
          const partialBytes = typeof errObj.bytesRead === 'number' ? errObj.bytesRead : 0;
          if (partialBytes > 0) {
            chunks.push(buffer.subarray(0, partialBytes));
            totalBytesRead += partialBytes;
          }
          break;
        }
      }

      expect(totalBytesRead).toBe(testContent.length);

      // Reconstruct and verify
      const reconstructed = Buffer.concat(chunks);
      expect(reconstructed.equals(testContent)).toBe(true);
    } finally {
      await download.close();
    }
  });
});
