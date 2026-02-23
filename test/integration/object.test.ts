/**
 * @file test/integration/object.test.ts
 * @brief Integration tests for object operations
 *
 * Tests: statObject fields, deleteObject, listObjects (recursive, prefix, options),
 *        copyObject (same/cross bucket), moveObject, updateObjectMetadata
 *
 * Tests the chains:
 *   1. upload → statObject → verify all ObjectInfo fields
 *   2. upload → deleteObject → statObject(throws)
 *   3. upload nested objects → listObjects({recursive:true}) → verify count
 *   4. upload nested → listObjects({prefix, recursive:false}) → verify prefix items
 *   5. upload → copyObject(same bucket) → stat both
 *   6. upload → copyObject(other bucket) → stat both
 *   7. upload → moveObject → stat new(ok) → stat old(throws)
 *   8. upload with metadata → updateObjectMetadata({new}) → statObject → verify
 *   9. upload → listObjects({system:true, custom:true}) → verify metadata
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

/** Helper to upload a test object */
async function uploadTestObject(
  project: ProjectResultStruct,
  bucket: string,
  key: string,
  content: Buffer,
  metadata?: Record<string, string>
): Promise<void> {
  const upload = await project.uploadObject(bucket, key);
  if (metadata) {
    await upload.setCustomMetadata(metadata);
  }
  await upload.write(content, content.length);
  await upload.commit();
}

describe('Integration: Object Operations', () => {
  const validCredentials = hasAnyCredentials;
  const runTest = validCredentials ? it : it.skip;
  const bucketName = `int-obj-${Date.now()}`;
  const crossBucket = `int-obj-cross-${Date.now()}`;
  const content = Buffer.from('object-operations-test-content');

  let uplink: Uplink;
  let access: AccessResultStruct;
  let project: ProjectResultStruct;

  beforeAll(async () => {
    if (!validCredentials) {
      console.log('⏭️  Skipping object integration tests: No valid credentials configured');
      return;
    }

    uplink = new Uplink();
    access = await getAccess(uplink);
    project = await access.openProject();
    await project.ensureBucket(bucketName);
    await project.ensureBucket(crossBucket);
  });

  afterAll(async () => {
    if (!validCredentials) return;

    for (const bkt of [bucketName, crossBucket]) {
      try {
        const objects = await project.listObjects(bkt, { recursive: true });
        for (const obj of objects) {
          await project.deleteObject(bkt, obj.key);
        }
        await project.deleteBucket(bkt);
      } catch (_) {
        // may not exist
      }
    }
    await project.close();
  });

  describe('statObject fields', () => {
    runTest('should verify all ObjectInfo fields from statObject', async () => {
      const key = `stat-fields-${Date.now()}.txt`;
      const metadata = { 'x-stat-test': 'value-1' };

      await uploadTestObject(project, bucketName, key, content, metadata);

      const info = await project.statObject(bucketName, key);

      // key
      expect(info.key).toBe(key);

      // isPrefix
      expect(info.isPrefix).toBe(false);

      // system metadata
      expect(info.system).toBeDefined();
      // Native layer returns Unix timestamps as integers
      expect(typeof info.system.created).toBe('number');
      expect(info.system.created).toBeGreaterThan(0);
      expect(info.system.contentLength).toBe(content.length);

      // custom metadata
      expect(info.custom).toBeDefined();
      expect(info.custom['x-stat-test']).toBe('value-1');

      // cleanup
      await project.deleteObject(bucketName, key);
    });
  });

  describe('deleteObject', () => {
    runTest('should confirm object is gone after deleteObject', async () => {
      const key = `delete-verify-${Date.now()}.txt`;

      await uploadTestObject(project, bucketName, key, content);

      // Verify it exists
      const info = await project.statObject(bucketName, key);
      expect(info.key).toBe(key);

      // Delete
      await project.deleteObject(bucketName, key);

      // Should be gone
      await expect(project.statObject(bucketName, key)).rejects.toThrow();
    });
  });

  describe('listObjects', () => {
    runTest('should list objects recursively', async () => {
      const prefix = `list-rec-${Date.now()}`;
      const keys = [
        `${prefix}/a.txt`,
        `${prefix}/b.txt`,
        `${prefix}/sub/c.txt`,
      ];

      for (const key of keys) {
        await uploadTestObject(project, bucketName, key, content);
      }

      const objects = await project.listObjects(bucketName, {
        prefix: `${prefix}/`,
        recursive: true,
      });

      expect(objects.length).toBe(3);
      const objectKeys = objects.map((o) => o.key);
      for (const key of keys) {
        expect(objectKeys).toContain(key);
      }

      // cleanup
      for (const key of keys) {
        await project.deleteObject(bucketName, key);
      }
    });

    runTest('should list objects with prefix non-recursively', async () => {
      const prefix = `list-nonrec-${Date.now()}`;
      const keys = [
        `${prefix}/top.txt`,
        `${prefix}/sub/nested.txt`,
      ];

      for (const key of keys) {
        await uploadTestObject(project, bucketName, key, content);
      }

      const objects = await project.listObjects(bucketName, {
        prefix: `${prefix}/`,
        recursive: false,
      });

      // Non-recursive: should see top.txt and sub/ prefix
      expect(objects.length).toBe(2);

      const directFile = objects.find((o) => o.key === `${prefix}/top.txt`);
      expect(directFile).toBeDefined();
      expect(directFile!.isPrefix).toBe(false);

      const prefixItem = objects.find((o) => o.key === `${prefix}/sub/`);
      expect(prefixItem).toBeDefined();
      expect(prefixItem!.isPrefix).toBe(true);

      // cleanup
      for (const key of keys) {
        await project.deleteObject(bucketName, key);
      }
    });

    runTest('should list objects with system and custom metadata', async () => {
      const prefix = `list-meta-${Date.now()}/`;
      const key = `${prefix}data.txt`;
      const metadata = { 'x-list-meta': 'test-value' };

      await uploadTestObject(project, bucketName, key, content, metadata);

      const objects = await project.listObjects(bucketName, {
        prefix,
        recursive: true,
        system: true,
        custom: true,
      });

      expect(objects.length).toBe(1);
      const obj = objects[0];
      expect(obj.key).toBe(key);
      expect(obj.system.contentLength).toBe(content.length);
      expect(obj.custom['x-list-meta']).toBe('test-value');

      // cleanup
      await project.deleteObject(bucketName, key);
    });
  });

  describe('copyObject', () => {
    runTest('should copy object within same bucket', async () => {
      const srcKey = `copy-src-${Date.now()}.txt`;
      const dstKey = `copy-dst-${Date.now()}.txt`;

      await uploadTestObject(project, bucketName, srcKey, content);

      const copiedInfo = await project.copyObject(
        bucketName, srcKey, bucketName, dstKey
      );

      expect(copiedInfo).toBeDefined();
      expect(copiedInfo.key).toBe(dstKey);

      // Both should exist
      const srcStat = await project.statObject(bucketName, srcKey);
      const dstStat = await project.statObject(bucketName, dstKey);
      expect(srcStat.key).toBe(srcKey);
      expect(dstStat.key).toBe(dstKey);
      expect(dstStat.system.contentLength).toBe(content.length);

      // cleanup
      await project.deleteObject(bucketName, srcKey);
      await project.deleteObject(bucketName, dstKey);
    });

    runTest('should copy object to different bucket', async () => {
      const srcKey = `copy-cross-src-${Date.now()}.txt`;
      const dstKey = `copy-cross-dst-${Date.now()}.txt`;

      await uploadTestObject(project, bucketName, srcKey, content);

      const copiedInfo = await project.copyObject(
        bucketName, srcKey, crossBucket, dstKey
      );

      expect(copiedInfo).toBeDefined();
      expect(copiedInfo.key).toBe(dstKey);

      // Both should exist in their respective buckets
      const srcStat = await project.statObject(bucketName, srcKey);
      const dstStat = await project.statObject(crossBucket, dstKey);
      expect(srcStat.key).toBe(srcKey);
      expect(dstStat.key).toBe(dstKey);

      // cleanup
      await project.deleteObject(bucketName, srcKey);
      await project.deleteObject(crossBucket, dstKey);
    });
  });

  describe('moveObject', () => {
    runTest('should move object and verify source gone', async () => {
      const srcKey = `move-src-${Date.now()}.txt`;
      const dstKey = `move-dst-${Date.now()}.txt`;

      await uploadTestObject(project, bucketName, srcKey, content);

      await project.moveObject(bucketName, srcKey, bucketName, dstKey);

      // Destination should exist
      const dstStat = await project.statObject(bucketName, dstKey);
      expect(dstStat.key).toBe(dstKey);
      expect(dstStat.system.contentLength).toBe(content.length);

      // Source should be gone
      await expect(project.statObject(bucketName, srcKey)).rejects.toThrow();

      // cleanup
      await project.deleteObject(bucketName, dstKey);
    });
  });

  describe('updateObjectMetadata', () => {
    runTest('should update object custom metadata', async () => {
      const key = `update-meta-${Date.now()}.txt`;
      const originalMeta = { 'x-original': 'old-value' };
      const newMeta = { 'x-updated': 'new-value', 'x-extra': 'extra-value' };

      await uploadTestObject(project, bucketName, key, content, originalMeta);

      // Verify original metadata
      const before = await project.statObject(bucketName, key);
      expect(before.custom['x-original']).toBe('old-value');

      // Update metadata (replaces all custom metadata)
      await project.updateObjectMetadata(bucketName, key, newMeta);

      // Verify new metadata
      const after = await project.statObject(bucketName, key);
      expect(after.custom['x-updated']).toBe('new-value');
      expect(after.custom['x-extra']).toBe('extra-value');
      // Original metadata should be replaced
      expect(after.custom['x-original']).toBeUndefined();

      // cleanup
      await project.deleteObject(bucketName, key);
    });
  });
});
