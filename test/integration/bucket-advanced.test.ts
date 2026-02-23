/**
 * @file test/integration/bucket-advanced.test.ts
 * @brief Integration tests for advanced bucket operations
 *
 * Tests: deleteBucketWithObjects, listBuckets with options
 *
 * Tests the chains:
 *   1. ensureBucket → upload objects → deleteBucketWithObjects → statBucket(throws)
 *   2. listBuckets() → verify array
 *   3. create 2 buckets → listBuckets({cursor}) → verify pagination
 *
 * Requires environment variables:
 * - TEST_ACCESS_GRANT: Pre-serialized access grant, OR
 * - TEST_SATELLITE, TEST_API_KEY, TEST_PASSPHRASE
 */

import {
  Uplink,
  AccessResultStruct,
  ProjectResultStruct,
} from '../../src';
import { hasAnyCredentials, getAccess } from '../helpers/credentials';

describe('Integration: Advanced Bucket Operations', () => {
  const validCredentials = hasAnyCredentials;
  const runTest = validCredentials ? it : it.skip;

  let uplink: Uplink;
  let access: AccessResultStruct;
  let project: ProjectResultStruct;

  // Track buckets for cleanup
  const bucketsToClean: string[] = [];

  beforeAll(async () => {
    if (!validCredentials) {
      console.log('⏭️  Skipping advanced bucket integration tests: No valid credentials configured');
      return;
    }

    uplink = new Uplink();
    access = await getAccess(uplink);
    project = await access.openProject();
  });

  afterAll(async () => {
    if (!validCredentials) return;

    for (const name of bucketsToClean) {
      try {
        await project.deleteBucketWithObjects(name);
      } catch (_) {
        // bucket may already be deleted
      }
    }
    await project.close();
  });

  describe('deleteBucketWithObjects', () => {
    runTest('should delete bucket with objects', async () => {
      const bucketName = `int-bkt-adv-${Date.now()}`;
      bucketsToClean.push(bucketName);

      await project.ensureBucket(bucketName);

      // Upload 2 objects into the bucket
      const content = Buffer.from('bucket-with-objects-test');

      const upload1 = await project.uploadObject(bucketName, 'file1.txt');
      await upload1.write(content, content.length);
      await upload1.commit();

      const upload2 = await project.uploadObject(bucketName, 'nested/file2.txt');
      await upload2.write(content, content.length);
      await upload2.commit();

      // Verify objects exist
      const objects = await project.listObjects(bucketName, { recursive: true });
      expect(objects.length).toBe(2);

      // Delete bucket with objects
      await project.deleteBucketWithObjects(bucketName);

      // Bucket should no longer exist
      await expect(project.statBucket(bucketName)).rejects.toThrow();

      // Remove from cleanup list since it's already deleted
      const idx = bucketsToClean.indexOf(bucketName);
      if (idx >= 0) bucketsToClean.splice(idx, 1);
    }, 60000);

    runTest('should delete empty bucket with deleteBucketWithObjects', async () => {
      const bucketName = `int-bkt-empty-${Date.now()}`;
      bucketsToClean.push(bucketName);

      await project.ensureBucket(bucketName);

      // deleteBucketWithObjects should work on empty buckets too
      await project.deleteBucketWithObjects(bucketName);

      await expect(project.statBucket(bucketName)).rejects.toThrow();

      const idx = bucketsToClean.indexOf(bucketName);
      if (idx >= 0) bucketsToClean.splice(idx, 1);
    }, 60000);
  });

  describe('listBuckets with options', () => {
    runTest('should list buckets without options', async () => {
      const buckets = await project.listBuckets();

      expect(Array.isArray(buckets)).toBe(true);
      // Each bucket should have name and created (Unix timestamp)
      for (const bucket of buckets) {
        expect(typeof bucket.name).toBe('string');
        expect(typeof bucket.created).toBe('number');
        expect(bucket.created).toBeGreaterThan(0);
      }
    });

    runTest('should list buckets with cursor option for pagination', async () => {
      // Create 2 buckets with predictable names (sorted)
      const prefix = `int-cur-${Date.now()}`;
      const bucketA = `${prefix}-aaa`;
      const bucketB = `${prefix}-bbb`;
      bucketsToClean.push(bucketA, bucketB);

      await project.ensureBucket(bucketA);
      await project.ensureBucket(bucketB);

      // List with cursor set to first bucket name (should return buckets after it)
      const afterCursor = await project.listBuckets({ cursor: bucketA });
      expect(Array.isArray(afterCursor)).toBe(true);

      // bucketB should be in results, bucketA should NOT be
      const namesAfterCursor = afterCursor.map((b) => b.name);
      expect(namesAfterCursor).not.toContain(bucketA);
      expect(namesAfterCursor).toContain(bucketB);

      // cleanup
      await project.deleteBucket(bucketA);
      await project.deleteBucket(bucketB);
      const idxA = bucketsToClean.indexOf(bucketA);
      if (idxA >= 0) bucketsToClean.splice(idxA, 1);
      const idxB = bucketsToClean.indexOf(bucketB);
      if (idxB >= 0) bucketsToClean.splice(idxB, 1);
    }, 60000);
  });
});
