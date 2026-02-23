/**
 * @file test/e2e/01-bucket-ops.test.ts
 * @brief E2E: Bucket operations — create, ensure, stat, list
 *
 * Tests the full bucket lifecycle. When running as part of the full suite,
 * the shared context bucket is already created; this file tests additional
 * bucket operations.
 */

import { hasCredentials } from '../helpers/credentials';
import {
  getE2eContext,
  type E2eContext,
} from './helpers/e2e-context';

describe('E2E: Bucket Operations', () => {
  const runTest = hasCredentials ? it : it.skip;
  let ctx: E2eContext;

  beforeAll(async () => {
    if (!hasCredentials) {
      console.log('⏭️  Skipping E2E bucket tests: No valid credentials configured');
      return;
    }
    ctx = await getE2eContext();
  });

  describe('ensureBucket', () => {
    runTest('should ensure the test bucket exists (idempotent)', async () => {
      const bucket = await ctx.project.ensureBucket(ctx.bucketName);

      expect(bucket).toBeDefined();
      expect(bucket.name).toBe(ctx.bucketName);
    });
  });

  describe('statBucket', () => {
    runTest('should stat the test bucket', async () => {
      const bucket = await ctx.project.statBucket(ctx.bucketName);

      expect(bucket).toBeDefined();
      expect(bucket.name).toBe(ctx.bucketName);
      expect(typeof bucket.created).toBe('number');
      expect(bucket.created).toBeGreaterThan(0);
    });
  });

  describe('listBuckets', () => {
    runTest('should list buckets and find the test bucket', async () => {
      const buckets = await ctx.project.listBuckets();

      expect(Array.isArray(buckets)).toBe(true);
      expect(buckets.length).toBeGreaterThan(0);

      const found = buckets.find(b => b.name === ctx.bucketName);
      expect(found).toBeDefined();
      expect(found!.name).toBe(ctx.bucketName);
    });

    runTest('should list buckets with cursor for pagination', async () => {
      const extraBucket = `${ctx.bucketName}-extra`;

      await ctx.project.ensureBucket(extraBucket);

      try {
        const all = await ctx.project.listBuckets();
        const allNames = all.map(b => b.name);
        expect(allNames).toContain(extraBucket);

        // Use cursor to paginate past the main test bucket
        const afterCursor = await ctx.project.listBuckets({ cursor: ctx.bucketName });
        const afterNames = afterCursor.map(b => b.name);
        expect(afterNames).not.toContain(ctx.bucketName);
      } finally {
        await ctx.project.deleteBucket(extraBucket);
      }
    });
  });

  describe('createBucket + deleteBucket cycle', () => {
    runTest('should create and delete a temporary bucket', async () => {
      const tempBucket = `e2e-temp-${Date.now()}`;

      const created = await ctx.project.createBucket(tempBucket);
      expect(created).toBeDefined();
      expect(created.name).toBe(tempBucket);
      expect(typeof created.created).toBe('number');

      // Stat should succeed
      const stat = await ctx.project.statBucket(tempBucket);
      expect(stat.name).toBe(tempBucket);

      // Delete
      await ctx.project.deleteBucket(tempBucket);

      // Stat should fail after deletion
      await expect(ctx.project.statBucket(tempBucket)).rejects.toThrow();
    });
  });
});
