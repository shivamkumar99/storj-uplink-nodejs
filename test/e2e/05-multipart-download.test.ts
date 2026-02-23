/**
 * @file test/e2e/05-multipart-download.test.ts
 * @brief E2E: Download multipart-uploaded objects and verify data integrity
 *
 * Uploads a multipart object (2 parts) then downloads and verifies
 * byte-level integrity.
 */

import { beginMultipartUpload } from '../../src';
import { hasCredentials } from '../helpers/credentials';
import {
  getE2eContext,
  trackKey,
  type E2eContext,
} from './helpers/e2e-context';

describe('E2E: Multipart Download & Integrity', () => {
  const runTest = hasCredentials ? it : it.skip;
  let ctx: E2eContext;

  const mpDownloadKey = 'e2e-mp-download-integrity.bin';
  const MIN_PART_SIZE = 5 * 1024 * 1024; // 5 MiB
  const part1Data = Buffer.alloc(MIN_PART_SIZE, 0xAA);
  const part2Data = Buffer.alloc(512, 0xBB);
  const totalSize = MIN_PART_SIZE + 512;

  beforeAll(async () => {
    if (!hasCredentials) {
      console.log('⏭️  Skipping E2E multipart download tests: No valid credentials configured');
      return;
    }
    ctx = await getE2eContext();

    // Ensure multipart object exists
    try {
      await ctx.project.statObject(ctx.bucketName, mpDownloadKey);
    } catch (_) {
      // Upload via multipart
      const mp = await beginMultipartUpload(
        ctx.project._nativeHandle, ctx.bucketName, mpDownloadKey,
      );

      const p1 = await mp.uploadPart(1);
      await p1.write(part1Data, part1Data.length);
      await p1.commit();

      const p2 = await mp.uploadPart(2);
      await p2.write(part2Data, part2Data.length);
      await p2.commit();

      await mp.commit();
      trackKey(mpDownloadKey);
    }
  });

  describe('Download Multipart Object', () => {
    runTest('should download and verify multipart data integrity', async () => {
      const download = await ctx.project.downloadObject(ctx.bucketName, mpDownloadKey);

      try {
        const objInfo = await download.info();
        const actualSize = objInfo.system.contentLength;
        expect(actualSize).toBe(totalSize);

        const buffer = Buffer.alloc(actualSize);
        let totalRead = 0;
        const CHUNK_SIZE = 256 * 1024;
        let zeroReadRetries = 0;
        const MAX_ZERO_RETRIES = 100;

        while (totalRead < actualSize) {
          const remaining = actualSize - totalRead;
          const toRead = Math.min(CHUNK_SIZE, remaining);
          const chunk = Buffer.alloc(toRead);

          try {
            const result = await download.read(chunk, toRead);
            if (result.bytesRead > 0) {
              chunk.copy(buffer, totalRead, 0, result.bytesRead);
              totalRead += result.bytesRead;
              zeroReadRetries = 0;
            } else {
              zeroReadRetries++;
              if (zeroReadRetries >= MAX_ZERO_RETRIES) break;
            }
          } catch (err: unknown) {
            const errObj = err as Record<string, unknown>;
            const partialBytes = typeof errObj.bytesRead === 'number' ? errObj.bytesRead : 0;
            if (partialBytes > 0) {
              chunk.copy(buffer, totalRead, 0, partialBytes);
              totalRead += partialBytes;
            }
            break;
          }
        }

        expect(totalRead).toBe(totalSize);

        // Verify part 1 bytes (0xAA)
        for (let i = 0; i < 10; i++) {
          expect(buffer[i]).toBe(0xAA);
        }
        expect(buffer[MIN_PART_SIZE - 1]).toBe(0xAA);

        // Verify part 2 bytes (0xBB)
        for (let i = MIN_PART_SIZE; i < MIN_PART_SIZE + 10; i++) {
          expect(buffer[i]).toBe(0xBB);
        }
        expect(buffer[totalSize - 1]).toBe(0xBB);
      } finally {
        await download.close();
      }
    }, 180000);
  });

  describe('Stat Multipart Object', () => {
    runTest('should stat multipart-uploaded object', async () => {
      const stat = await ctx.project.statObject(ctx.bucketName, mpDownloadKey);

      expect(stat.key).toBe(mpDownloadKey);
      expect(stat.system.contentLength).toBe(totalSize);
      expect(stat.isPrefix).toBe(false);
    });
  });
});
