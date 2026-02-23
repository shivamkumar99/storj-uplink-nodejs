/**
 * @file test/e2e/09-cleanup.test.ts
 * @brief E2E: Cleanup — delete objects, delete bucket, verify, close project
 *
 * This is the final E2E test. It verifies deletion operations work
 * correctly and cleans up all resources created by the test suite.
 */

import { hasAnyCredentials } from '../helpers/credentials';
import {
  getE2eContext,
  cleanupE2eContext,
  type E2eContext,
} from './helpers/e2e-context';

describe('E2E: Cleanup & Deletion', () => {
  const runTest = hasAnyCredentials ? it : it.skip;
  let ctx: E2eContext;

  beforeAll(async () => {
    if (!hasAnyCredentials) {
      console.log('⏭️  Skipping E2E cleanup tests: No valid credentials configured');
      return;
    }
    ctx = await getE2eContext();
  });

  describe('Delete Objects', () => {
    runTest('should delete all tracked objects', async () => {
      const keys = Array.from(ctx.uploadedKeys);

      for (const key of keys) {
        await ctx.project.deleteObject(ctx.bucketName, key);
        ctx.uploadedKeys.delete(key);
      }

      // Verify no objects remain
      const remaining = await ctx.project.listObjects(ctx.bucketName, { recursive: true });

      // Delete any stragglers
      for (const obj of remaining) {
        await ctx.project.deleteObject(ctx.bucketName, obj.key);
      }

      const afterCleanup = await ctx.project.listObjects(ctx.bucketName, { recursive: true });
      expect(afterCleanup.length).toBe(0);
    });
  });

  describe('Delete Bucket', () => {
    runTest('should delete the test bucket', async () => {
      await ctx.project.deleteBucket(ctx.bucketName);

      // Bucket should no longer exist
      await expect(ctx.project.statBucket(ctx.bucketName)).rejects.toThrow();
    });
  });

  describe('Close Project', () => {
    runTest('should close the project gracefully', async () => {
      // Use cleanupE2eContext to close the project and reset the singleton.
      // This prevents the afterAll safety-net hook from double-closing.
      await cleanupE2eContext();
    });
  });
});
