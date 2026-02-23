/**
 * @file test/integration/multipart.test.ts
 * @brief Integration tests for all 11 multipart upload functions
 *
 * Tests: beginMultipartUpload, commitUpload, abortUpload, uploadPart,
 *        partUploadWrite, partUploadCommit, partUploadAbort,
 *        partUploadSetEtag, partUploadInfo, listUploadParts, listMultipartUploads
 *
 * Tests the chains:
 *   1.  beginMultipartUpload → verify uploadId → abort
 *   2.  begin → uploadPart → write → partCommit → uploadPart → write → partCommit → commit → statObject
 *   3.  begin → uploadPart → write → partCommit → listParts → verify
 *   4.  begin → listMultipartUploads → verify uploadId in list → abort
 *   5.  begin → uploadPart → info() → verify partNumber
 *   6.  begin → uploadPart → setEtag → partCommit → listParts → verify etag
 *   7.  begin → uploadPart → write → partAbort → uploadPart → write → partCommit → commit
 *   8.  begin → part(1) write → commit → part(2) write → commit → commit upload → download → verify
 *   9.  begin → abort → begin(same key) → part → write → partCommit → commit
 *   10. begin → part → write → partCommit → commit({customMetadata}) → statObject → verify
 *   11. begin({expires}) → part → write → partCommit → commit → statObject → verify expires
 *   12. begin → 3 parts → listParts({cursor}) → verify pagination
 *
 * Requires environment variables:
 * - TEST_ACCESS_GRANT: Pre-serialized access grant, OR
 * - TEST_SATELLITE, TEST_API_KEY, TEST_PASSPHRASE
 */

import {
  Uplink,
  AccessResultStruct,
  ProjectResultStruct,
  MultipartUpload,
  PartUploadResultStruct,
  beginMultipartUpload,
  listMultipartUploads,
} from '../../src';
import { hasAnyCredentials, getAccess } from '../helpers/credentials';

describe('Integration: Multipart Upload Operations', () => {
  const validCredentials = hasAnyCredentials;
  const runTest = validCredentials ? it : it.skip;
  const bucketName = `int-mp-${Date.now()}`;

  let uplink: Uplink;
  let access: AccessResultStruct;
  let project: ProjectResultStruct;

  beforeAll(async () => {
    if (!validCredentials) {
      console.log('⏭️  Skipping multipart integration tests: No valid credentials configured');
      return;
    }

    uplink = new Uplink();
    access = await getAccess(uplink);
    project = await access.openProject();
    await project.ensureBucket(bucketName);
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

  runTest('should begin and abort a multipart upload', async () => {
    const key = `mp-abort-${Date.now()}.bin`;

    const mp = await beginMultipartUpload(
      project._nativeHandle, bucketName, key
    );

    expect(mp).toBeInstanceOf(MultipartUpload);
    expect(typeof mp.uploadId).toBe('string');
    expect(mp.uploadId.length).toBeGreaterThan(0);
    expect(mp.bucket).toBe(bucketName);
    expect(mp.key).toBe(key);
    expect(mp.isActive).toBe(true);

    await mp.abort();
    expect(mp.isActive).toBe(false);

    // Object should not exist
    await expect(project.statObject(bucketName, key)).rejects.toThrow();
  }, 60000);

  runTest('should complete full multipart lifecycle (2 parts)', async () => {
    const key = `mp-full-${Date.now()}.bin`;
    // Storj requires each part (except the last) to be at least 5 MiB
    const MIN_PART_SIZE = 5 * 1024 * 1024; // 5 MiB
    const part1Data = Buffer.alloc(MIN_PART_SIZE, 0x41); // 'A'
    const part2Data = Buffer.from('B'.repeat(256));

    const mp = await beginMultipartUpload(
      project._nativeHandle, bucketName, key
    );

    // Part 1 (>= 5 MiB)
    const part1 = await mp.uploadPart(1);
    expect(part1).toBeInstanceOf(PartUploadResultStruct);
    await part1.write(part1Data, part1Data.length);
    await part1.commit();
    expect(part1.isOpen).toBe(false);

    // Part 2 (last part, can be any size)
    const part2 = await mp.uploadPart(2);
    await part2.write(part2Data, part2Data.length);
    await part2.commit();

    // Commit multipart upload
    const objInfo = await mp.commit();
    expect(objInfo).toBeDefined();
    expect(objInfo.key).toBe(key);

    // Verify object exists
    const stat = await project.statObject(bucketName, key);
    expect(stat.key).toBe(key);
    expect(stat.system.contentLength).toBe(part1Data.length + part2Data.length);

    // cleanup
    await project.deleteObject(bucketName, key);
  }, 120000);

  runTest('should list uploaded parts', async () => {
    const key = `mp-listparts-${Date.now()}.bin`;
    const partData = Buffer.from('C'.repeat(128));

    const mp = await beginMultipartUpload(
      project._nativeHandle, bucketName, key
    );

    try {
      const part = await mp.uploadPart(1);
      await part.write(partData, partData.length);
      await part.commit();

      const parts = await mp.listParts();
      expect(Array.isArray(parts)).toBe(true);
      expect(parts.length).toBe(1);
      expect(parts[0].partNumber).toBe(1);
      expect(parts[0].size).toBe(partData.length);
    } finally {
      await mp.abort().catch(() => {});
    }
  }, 60000);

  runTest('should list pending multipart uploads', async () => {
    const key = `mp-listuploads-${Date.now()}.bin`;

    const mp = await beginMultipartUpload(
      project._nativeHandle, bucketName, key
    );

    try {
      const uploads = await listMultipartUploads(
        project._nativeHandle, bucketName
      );

      expect(Array.isArray(uploads)).toBe(true);
      const found = uploads.find((u) => u.uploadId === mp.uploadId);
      expect(found).toBeDefined();
      expect(found!.key).toBe(key);
    } finally {
      await mp.abort();
    }
  }, 60000);

  runTest('should get part info', async () => {
    const key = `mp-partinfo-${Date.now()}.bin`;

    const mp = await beginMultipartUpload(
      project._nativeHandle, bucketName, key
    );

    try {
      const part = await mp.uploadPart(1);
      const info = await part.info();

      expect(info).toBeDefined();
      expect(info.partNumber).toBe(1);

      await part.abort();
    } finally {
      await mp.abort().catch(() => {});
    }
  }, 60000);

  runTest('should set and verify part etag', async () => {
    const key = `mp-etag-${Date.now()}.bin`;
    const partData = Buffer.from('D'.repeat(64));
    const testEtag = 'test-etag-value-123';

    const mp = await beginMultipartUpload(
      project._nativeHandle, bucketName, key
    );

    try {
      const part = await mp.uploadPart(1);
      await part.setEtag(testEtag);
      await part.write(partData, partData.length);
      await part.commit();

      const parts = await mp.listParts();
      expect(parts.length).toBe(1);
      expect(parts[0].etag).toBe(testEtag);
    } finally {
      await mp.abort().catch(() => {});
    }
  }, 60000);

  runTest('should abort part then retry', async () => {
    const key = `mp-partabort-${Date.now()}.bin`;
    const badData = Buffer.from('wrong data');
    const goodData = Buffer.from('correct data');

    const mp = await beginMultipartUpload(
      project._nativeHandle, bucketName, key
    );

    // Upload bad data, then abort
    const badPart = await mp.uploadPart(1);
    await badPart.write(badData, badData.length);
    await badPart.abort();
    expect(badPart.isOpen).toBe(false);

    // Retry with good data
    const goodPart = await mp.uploadPart(1);
    await goodPart.write(goodData, goodData.length);
    await goodPart.commit();

    // Commit upload
    const objInfo = await mp.commit();
    expect(objInfo.key).toBe(key);

    // Verify
    const stat = await project.statObject(bucketName, key);
    expect(stat.system.contentLength).toBe(goodData.length);

    // cleanup
    await project.deleteObject(bucketName, key);
  }, 60000);

  runTest('should verify multipart data integrity', async () => {
    const key = `mp-integrity-${Date.now()}.bin`;
    // Storj requires each part (except the last) to be at least 5 MiB
    const MIN_PART_SIZE = 5 * 1024 * 1024; // 5 MiB
    const part1Data = Buffer.alloc(MIN_PART_SIZE, 0xAA);
    const part2Data = Buffer.alloc(256, 0xBB);
    const totalSize = MIN_PART_SIZE + 256;

    const mp = await beginMultipartUpload(
      project._nativeHandle, bucketName, key
    );

    // Write two parts
    const p1 = await mp.uploadPart(1);
    const p1Written = await p1.write(part1Data, part1Data.length);
    console.log(`Part 1 write: ${p1Written} bytes (expected ${part1Data.length})`);
    await p1.commit();

    const p2 = await mp.uploadPart(2);
    const p2Written = await p2.write(part2Data, part2Data.length);
    console.log(`Part 2 write: ${p2Written} bytes (expected ${part2Data.length})`);
    await p2.commit();

    await mp.commit();

    // Download and verify using loop-based reads (matching old uplink-nodejs pattern)
    const download = await project.downloadObject(bucketName, key);
    try {
      // Get actual object size from download info
      const objInfo = await download.info();
      const actualSize = objInfo.system.contentLength;
      console.log(`Download info: contentLength=${actualSize}, expected=${totalSize}`);

      // Verify total size matches expected
      expect(actualSize).toBe(totalSize);

      // Read in a loop with smaller chunks, like the old HelloStorj.js pattern
      const buffer = Buffer.alloc(actualSize);
      let totalRead = 0;
      const CHUNK_SIZE = 256 * 1024; // 256 KiB chunks
      let zeroReadRetries = 0;
      const MAX_ZERO_RETRIES = 100;

      while (totalRead < actualSize) {
        const remaining = actualSize - totalRead;
        const toRead = Math.min(CHUNK_SIZE, remaining);
        const chunk = Buffer.alloc(toRead);
        try {
          const result = await download.read(chunk, toRead);
          console.log(`Read chunk: requested=${toRead}, got=${result.bytesRead}, totalSoFar=${totalRead + result.bytesRead}`);
          if (result.bytesRead > 0) {
            chunk.copy(buffer, totalRead, 0, result.bytesRead);
            totalRead += result.bytesRead;
            zeroReadRetries = 0;
          } else {
            // Zero-byte read at multipart segment boundary — retry
            zeroReadRetries++;
            console.log(`Zero-byte read (retry ${zeroReadRetries}/${MAX_ZERO_RETRIES})`);
            if (zeroReadRetries >= MAX_ZERO_RETRIES) {
              console.log('Too many zero-byte reads, breaking');
              break;
            }
          }
        } catch (err: unknown) {
          // EOF error — collect any partial bytes attached to the error
          const errObj = err as Record<string, unknown>;
          const partialBytes = typeof errObj.bytesRead === 'number' ? errObj.bytesRead : 0;
          if (partialBytes > 0) {
            chunk.copy(buffer, totalRead, 0, partialBytes);
            totalRead += partialBytes;
          }
          const errMsg = err instanceof Error ? err.message : String(err);
          console.log(`Read error at totalRead=${totalRead}: ${errMsg} (partialBytes=${partialBytes})`);
          break;
        }
      }

      console.log(`Total downloaded: ${totalRead} bytes (expected ${totalSize})`);
      expect(totalRead).toBe(totalSize);

      // First MIN_PART_SIZE bytes should be 0xAA
      for (let i = 0; i < 10; i++) {
        expect(buffer[i]).toBe(0xAA);
      }
      expect(buffer[MIN_PART_SIZE - 1]).toBe(0xAA);
      // Last 256 bytes should be 0xBB
      for (let i = MIN_PART_SIZE; i < MIN_PART_SIZE + 10; i++) {
        expect(buffer[i]).toBe(0xBB);
      }
      expect(buffer[totalSize - 1]).toBe(0xBB);
    } finally {
      await download.close();
    }

    // cleanup
    await project.deleteObject(bucketName, key);
  }, 120000);

  runTest('should abort upload then re-upload with same key', async () => {
    const key = `mp-reupload-${Date.now()}.bin`;
    const partData = Buffer.from('re-uploaded content');

    // First attempt: begin and abort
    const mp1 = await beginMultipartUpload(
      project._nativeHandle, bucketName, key
    );
    await mp1.abort();

    // Second attempt: begin, write, commit
    const mp2 = await beginMultipartUpload(
      project._nativeHandle, bucketName, key
    );
    const part = await mp2.uploadPart(1);
    await part.write(partData, partData.length);
    await part.commit();

    const objInfo = await mp2.commit();
    expect(objInfo.key).toBe(key);

    // Verify
    const stat = await project.statObject(bucketName, key);
    expect(stat.system.contentLength).toBe(partData.length);

    // cleanup
    await project.deleteObject(bucketName, key);
  }, 60000);

  runTest('should commit with custom metadata', async () => {
    const key = `mp-meta-${Date.now()}.bin`;
    const partData = Buffer.from('metadata content');
    const customMeta = { 'x-mp-test': 'multipart-value', 'x-author': 'integration' };

    const mp = await beginMultipartUpload(
      project._nativeHandle, bucketName, key
    );

    const part = await mp.uploadPart(1);
    await part.write(partData, partData.length);
    await part.commit();

    const objInfo = await mp.commit({ customMetadata: customMeta });
    expect(objInfo.key).toBe(key);

    // Verify metadata
    const stat = await project.statObject(bucketName, key);
    expect(stat.custom['x-mp-test']).toBe('multipart-value');
    expect(stat.custom['x-author']).toBe('integration');

    // cleanup
    await project.deleteObject(bucketName, key);
  }, 60000);

  runTest('should begin upload with expires option', async () => {
    const key = `mp-expires-${Date.now()}.bin`;
    const partData = Buffer.from('expiring content');
    const expiresTimestamp = Math.floor(Date.now() / 1000) + 24 * 60 * 60; // +24h

    const mp = await beginMultipartUpload(
      project._nativeHandle, bucketName, key,
      { expires: expiresTimestamp }
    );

    const part = await mp.uploadPart(1);
    await part.write(partData, partData.length);
    await part.commit();

    await mp.commit();

    const stat = await project.statObject(bucketName, key);
    expect(stat.system.expires).not.toBeNull();

    if (stat.system.expires) {
      // stat.system.expires is a Unix timestamp in seconds
      const diffSeconds = stat.system.expires - Math.floor(Date.now() / 1000);
      expect(diffSeconds).toBeGreaterThan(23 * 60 * 60);
      expect(diffSeconds).toBeLessThan(25 * 60 * 60);
    }

    // cleanup
    await project.deleteObject(bucketName, key);
  }, 60000);

  runTest('should list parts with cursor for pagination', async () => {
    const key = `mp-cursor-${Date.now()}.bin`;
    const partData = Buffer.from('E'.repeat(32));

    const mp = await beginMultipartUpload(
      project._nativeHandle, bucketName, key
    );

    try {
      // Upload 3 parts
      for (let i = 1; i <= 3; i++) {
        const part = await mp.uploadPart(i);
        await part.write(partData, partData.length);
        await part.commit();
      }

      // List all parts
      const allParts = await mp.listParts();
      expect(allParts.length).toBe(3);

      // List with cursor = 1 (should return parts after part 1)
      const afterPart1 = await mp.listParts({ cursor: 1 });
      expect(afterPart1.length).toBe(2);
      expect(afterPart1[0].partNumber).toBe(2);
      expect(afterPart1[1].partNumber).toBe(3);
    } finally {
      await mp.abort().catch(() => {});
    }
  }, 60000);
});
