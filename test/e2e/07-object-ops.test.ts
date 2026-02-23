/**
 * @file test/e2e/07-object-ops.test.ts
 * @brief E2E: Object operations — copy, move, updateMetadata, stat
 */

import { hasAnyCredentials } from '../helpers/credentials';
import {
  getE2eContext,
  uploadTestObject,
  deleteTestObject,
  trackKey,
  type E2eContext,
} from './helpers/e2e-context';

describe('E2E: Object Operations', () => {
  const runTest = hasAnyCredentials ? it : it.skip;
  let ctx: E2eContext;

  beforeAll(async () => {
    if (!hasAnyCredentials) {
      console.log('⏭️  Skipping E2E object operations tests: No valid credentials configured');
      return;
    }
    ctx = await getE2eContext();
  });

  describe('statObject', () => {
    runTest('should return full object info from statObject', async () => {
      const key = `e2e-stat-${Date.now()}.txt`;
      const content = Buffer.from('stat test content');
      const metadata = { 'x-stat-test': 'value-1' };

      await uploadTestObject(ctx, key, content, metadata);

      try {
        const info = await ctx.project.statObject(ctx.bucketName, key);

        expect(info.key).toBe(key);
        expect(info.isPrefix).toBe(false);
        expect(info.system).toBeDefined();
        expect(typeof info.system.created).toBe('number');
        expect(info.system.created).toBeGreaterThan(0);
        expect(info.system.contentLength).toBe(content.length);
        expect(info.custom['x-stat-test']).toBe('value-1');
      } finally {
        await deleteTestObject(ctx, key);
      }
    });
  });

  describe('copyObject', () => {
    runTest('should copy object within same bucket', async () => {
      const srcKey = `e2e-copy-src-${Date.now()}.txt`;
      const dstKey = `e2e-copy-dst-${Date.now()}.txt`;
      const content = Buffer.from('copy test content');

      await uploadTestObject(ctx, srcKey, content);

      try {
        const copiedInfo = await ctx.project.copyObject(
          ctx.bucketName, srcKey, ctx.bucketName, dstKey,
        );
        trackKey(dstKey);

        expect(copiedInfo).toBeDefined();
        expect(copiedInfo.key).toBe(dstKey);

        // Both should exist
        const srcStat = await ctx.project.statObject(ctx.bucketName, srcKey);
        const dstStat = await ctx.project.statObject(ctx.bucketName, dstKey);
        expect(srcStat.key).toBe(srcKey);
        expect(dstStat.key).toBe(dstKey);
        expect(dstStat.system.contentLength).toBe(content.length);
      } finally {
        await deleteTestObject(ctx, srcKey);
        await deleteTestObject(ctx, dstKey);
      }
    });
  });

  describe('moveObject', () => {
    runTest('should move object and verify source is gone', async () => {
      const srcKey = `e2e-move-src-${Date.now()}.txt`;
      const dstKey = `e2e-move-dst-${Date.now()}.txt`;
      const content = Buffer.from('move test content');

      await uploadTestObject(ctx, srcKey, content);

      try {
        await ctx.project.moveObject(ctx.bucketName, srcKey, ctx.bucketName, dstKey);
        trackKey(dstKey);

        // Destination should exist
        const dstStat = await ctx.project.statObject(ctx.bucketName, dstKey);
        expect(dstStat.key).toBe(dstKey);
        expect(dstStat.system.contentLength).toBe(content.length);

        // Source should be gone
        await expect(ctx.project.statObject(ctx.bucketName, srcKey)).rejects.toThrow();
      } finally {
        await deleteTestObject(ctx, dstKey);
      }
    });
  });

  describe('updateObjectMetadata', () => {
    runTest('should update object custom metadata', async () => {
      const key = `e2e-updatemeta-${Date.now()}.txt`;
      const content = Buffer.from('metadata update test');
      const originalMeta = { 'x-original': 'old-value' };
      const newMeta = { 'x-updated': 'new-value', 'x-extra': 'extra' };

      await uploadTestObject(ctx, key, content, originalMeta);

      try {
        // Verify original
        const before = await ctx.project.statObject(ctx.bucketName, key);
        expect(before.custom['x-original']).toBe('old-value');

        // Update metadata
        await ctx.project.updateObjectMetadata(ctx.bucketName, key, newMeta);

        // Verify updated
        const after = await ctx.project.statObject(ctx.bucketName, key);
        expect(after.custom['x-updated']).toBe('new-value');
        expect(after.custom['x-extra']).toBe('extra');
        expect(after.custom['x-original']).toBeUndefined();
      } finally {
        await deleteTestObject(ctx, key);
      }
    });
  });

  describe('Copy + Move with Listing Verification', () => {
    runTest('should copy, move, then verify via listing', async () => {
      const prefix = `e2e-copymove-${Date.now()}/`;
      const srcKey = `${prefix}source.txt`;
      const copyKey = `${prefix}copied.txt`;
      const moveKey = `${prefix}moved.txt`;
      const content = Buffer.from('copy-and-move-test-data');

      await uploadTestObject(ctx, srcKey, content);

      try {
        // Copy
        const copyInfo = await ctx.project.copyObject(
          ctx.bucketName, srcKey, ctx.bucketName, copyKey,
        );
        trackKey(copyKey);
        expect(copyInfo.key).toBe(copyKey);

        // Verify both exist
        let objects = await ctx.project.listObjects(ctx.bucketName, {
          prefix, recursive: true,
        });
        let keys = objects.map(o => o.key);
        expect(keys).toContain(srcKey);
        expect(keys).toContain(copyKey);

        // Move the copy
        await ctx.project.moveObject(ctx.bucketName, copyKey, ctx.bucketName, moveKey);
        trackKey(moveKey);

        // Verify: source + moved exist, copy is gone
        objects = await ctx.project.listObjects(ctx.bucketName, {
          prefix, recursive: true,
        });
        keys = objects.map(o => o.key);
        expect(keys).toContain(srcKey);
        expect(keys).toContain(moveKey);
        expect(keys).not.toContain(copyKey);

        // Download moved object and verify content
        const download = await ctx.project.downloadObject(ctx.bucketName, moveKey);
        const buf = Buffer.alloc(content.length);
        const result = await download.read(buf, buf.length);
        await download.close();
        expect(buf.subarray(0, result.bytesRead).toString()).toBe(content.toString());
      } finally {
        await deleteTestObject(ctx, srcKey);
        await deleteTestObject(ctx, copyKey);
        await deleteTestObject(ctx, moveKey);
      }
    });
  });

  describe('Error Recovery', () => {
    runTest('should handle errors gracefully and continue working', async () => {
      // Try to stat non-existent object
      await expect(
        ctx.project.statObject(ctx.bucketName, 'non-existent-xyz-999')
      ).rejects.toThrow();

      // Project should still work
      const buckets = await ctx.project.listBuckets();
      expect(Array.isArray(buckets)).toBe(true);

      // Try to download non-existent object
      await expect(
        ctx.project.downloadObject(ctx.bucketName, 'another-non-existent')
      ).rejects.toThrow();

      // Project should still work
      const objects = await ctx.project.listObjects(ctx.bucketName);
      expect(Array.isArray(objects)).toBe(true);
    });
  });
});
