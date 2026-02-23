/**
 * @file test/e2e/04-multipart-upload.test.ts
 * @brief E2E: Multipart upload operations
 *
 * Tests: beginMultipartUpload, uploadPart, partCommit, partAbort,
 *        setEtag, listParts, listMultipartUploads, commit, abort
 */

import {
  MultipartUpload,
  PartUploadResultStruct,
  beginMultipartUpload,
  listMultipartUploads,
} from '../../src';
import { hasCredentials } from '../helpers/credentials';
import {
  getE2eContext,
  trackKey,
  type E2eContext,
} from './helpers/e2e-context';

describe('E2E: Multipart Upload Operations', () => {
  const runTest = hasCredentials ? it : it.skip;
  let ctx: E2eContext;

  beforeAll(async () => {
    if (!hasCredentials) {
      console.log('⏭️  Skipping E2E multipart upload tests: No valid credentials configured');
      return;
    }
    ctx = await getE2eContext();
  });

  describe('Begin and Abort', () => {
    runTest('should begin and abort a multipart upload', async () => {
      const key = `e2e-mp-abort-${Date.now()}.bin`;

      const mp = await beginMultipartUpload(
        ctx.project._nativeHandle, ctx.bucketName, key,
      );

      expect(mp).toBeInstanceOf(MultipartUpload);
      expect(typeof mp.uploadId).toBe('string');
      expect(mp.uploadId.length).toBeGreaterThan(0);
      expect(mp.bucket).toBe(ctx.bucketName);
      expect(mp.key).toBe(key);
      expect(mp.isActive).toBe(true);

      await mp.abort();
      expect(mp.isActive).toBe(false);

      await expect(ctx.project.statObject(ctx.bucketName, key)).rejects.toThrow();
    });
  });

  describe('Full Multipart Lifecycle', () => {
    runTest('should complete multipart upload with 2 parts', async () => {
      const key = 'e2e-mp-full.bin';
      const MIN_PART_SIZE = 5 * 1024 * 1024; // 5 MiB
      const part1Data = Buffer.alloc(MIN_PART_SIZE, 0x41);
      const part2Data = Buffer.from('B'.repeat(256));

      const mp = await beginMultipartUpload(
        ctx.project._nativeHandle, ctx.bucketName, key,
      );

      // Part 1 (>= 5 MiB)
      const part1 = await mp.uploadPart(1);
      expect(part1).toBeInstanceOf(PartUploadResultStruct);
      await part1.write(part1Data, part1Data.length);
      await part1.commit();
      expect(part1.isOpen).toBe(false);

      // Part 2 (last part, any size)
      const part2 = await mp.uploadPart(2);
      await part2.write(part2Data, part2Data.length);
      await part2.commit();

      // Commit multipart upload
      const objInfo = await mp.commit();
      expect(objInfo).toBeDefined();
      expect(objInfo.key).toBe(key);
      trackKey(key);

      // Verify
      const stat = await ctx.project.statObject(ctx.bucketName, key);
      expect(stat.key).toBe(key);
      expect(stat.system.contentLength).toBe(part1Data.length + part2Data.length);
    }, 180000);
  });

  describe('List Parts', () => {
    runTest('should list uploaded parts', async () => {
      const key = `e2e-mp-listparts-${Date.now()}.bin`;
      const partData = Buffer.from('C'.repeat(128));

      const mp = await beginMultipartUpload(
        ctx.project._nativeHandle, ctx.bucketName, key,
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
    });
  });

  describe('List Pending Uploads', () => {
    runTest('should list pending multipart uploads', async () => {
      const key = `e2e-mp-pending-${Date.now()}.bin`;

      const mp = await beginMultipartUpload(
        ctx.project._nativeHandle, ctx.bucketName, key,
      );

      try {
        const uploads = await listMultipartUploads(
          ctx.project._nativeHandle, ctx.bucketName,
        );

        expect(Array.isArray(uploads)).toBe(true);
        const found = uploads.find(u => u.uploadId === mp.uploadId);
        expect(found).toBeDefined();
        expect(found!.key).toBe(key);
      } finally {
        await mp.abort();
      }
    });
  });

  describe('Part Etag', () => {
    runTest('should set and verify part etag', async () => {
      const key = `e2e-mp-etag-${Date.now()}.bin`;
      const partData = Buffer.from('D'.repeat(64));
      const testEtag = 'e2e-test-etag-value';

      const mp = await beginMultipartUpload(
        ctx.project._nativeHandle, ctx.bucketName, key,
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
    });
  });

  describe('Part Abort and Retry', () => {
    runTest('should abort a part then retry with new data', async () => {
      const key = `e2e-mp-partabort-${Date.now()}.bin`;
      const badData = Buffer.from('wrong data');
      const goodData = Buffer.from('correct data');

      const mp = await beginMultipartUpload(
        ctx.project._nativeHandle, ctx.bucketName, key,
      );

      // Upload bad data, then abort the part
      const badPart = await mp.uploadPart(1);
      await badPart.write(badData, badData.length);
      await badPart.abort();
      expect(badPart.isOpen).toBe(false);

      // Retry with good data
      const goodPart = await mp.uploadPart(1);
      await goodPart.write(goodData, goodData.length);
      await goodPart.commit();

      const objInfo = await mp.commit();
      expect(objInfo.key).toBe(key);
      trackKey(key);

      const stat = await ctx.project.statObject(ctx.bucketName, key);
      expect(stat.system.contentLength).toBe(goodData.length);
    });
  });

  describe('Commit with Custom Metadata', () => {
    runTest('should commit multipart upload with custom metadata', async () => {
      const key = `e2e-mp-meta-${Date.now()}.bin`;
      const partData = Buffer.from('metadata content');
      const customMeta = { 'x-mp-test': 'multipart-value', 'x-author': 'e2e' };

      const mp = await beginMultipartUpload(
        ctx.project._nativeHandle, ctx.bucketName, key,
      );

      const part = await mp.uploadPart(1);
      await part.write(partData, partData.length);
      await part.commit();

      const objInfo = await mp.commit({ customMetadata: customMeta });
      expect(objInfo.key).toBe(key);
      trackKey(key);

      const stat = await ctx.project.statObject(ctx.bucketName, key);
      expect(stat.custom['x-mp-test']).toBe('multipart-value');
      expect(stat.custom['x-author']).toBe('e2e');
    });
  });
});
