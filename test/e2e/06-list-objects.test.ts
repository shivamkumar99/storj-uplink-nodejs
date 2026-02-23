/**
 * @file test/e2e/06-list-objects.test.ts
 * @brief E2E: List objects — recursive, non-recursive, prefix, metadata
 *
 * Tests listing operations with various options. Ensures prerequisite
 * objects exist before listing.
 */

import { hasCredentials } from '../helpers/credentials';
import {
  getE2eContext,
  uploadTestObject,
  deleteTestObject,
  type E2eContext,
} from './helpers/e2e-context';

describe('E2E: List Objects', () => {
  const runTest = hasCredentials ? it : it.skip;
  let ctx: E2eContext;

  const listPrefix = `e2e-list-${Date.now()}/`;
  const listKeys = [
    `${listPrefix}file-a.txt`,
    `${listPrefix}file-b.txt`,
    `${listPrefix}sub/file-c.txt`,
    `${listPrefix}sub/file-d.txt`,
    `${listPrefix}sub/deep/file-e.txt`,
  ];

  beforeAll(async () => {
    if (!hasCredentials) {
      console.log('⏭️  Skipping E2E list objects tests: No valid credentials configured');
      return;
    }
    ctx = await getE2eContext();

    // Upload list test objects
    for (const key of listKeys) {
      await uploadTestObject(ctx, key, Buffer.from(`content-of-${key}`));
    }
  });

  afterAll(async () => {
    if (!hasCredentials || !ctx) return;
    for (const key of listKeys) {
      await deleteTestObject(ctx, key);
    }
  });

  describe('Recursive Listing', () => {
    runTest('should list all objects recursively', async () => {
      const objects = await ctx.project.listObjects(ctx.bucketName, {
        prefix: listPrefix,
        recursive: true,
      });

      expect(objects.length).toBe(5);
      const keys = objects.map(o => o.key);
      for (const key of listKeys) {
        expect(keys).toContain(key);
      }
    });
  });

  describe('Non-Recursive Listing', () => {
    runTest('should list top-level items with prefix entries', async () => {
      const objects = await ctx.project.listObjects(ctx.bucketName, {
        prefix: listPrefix,
        recursive: false,
      });

      // Should have: file-a.txt, file-b.txt, sub/ (prefix)
      expect(objects.length).toBe(3);

      const directFiles = objects.filter(o => !o.isPrefix);
      const prefixItems = objects.filter(o => o.isPrefix);
      expect(directFiles.length).toBe(2);
      expect(prefixItems.length).toBe(1);
      expect(prefixItems[0].key).toBe(`${listPrefix}sub/`);
    });

    runTest('should list nested sub-prefix non-recursively', async () => {
      const objects = await ctx.project.listObjects(ctx.bucketName, {
        prefix: `${listPrefix}sub/`,
        recursive: false,
      });

      // Should have: file-c.txt, file-d.txt, deep/ (prefix)
      expect(objects.length).toBe(3);

      const directFiles = objects.filter(o => !o.isPrefix);
      const prefixItems = objects.filter(o => o.isPrefix);
      expect(directFiles.length).toBe(2);
      expect(prefixItems.length).toBe(1);
      expect(prefixItems[0].key).toBe(`${listPrefix}sub/deep/`);
    });
  });

  describe('Listing with Metadata', () => {
    runTest('should include system and custom metadata in results', async () => {
      const objects = await ctx.project.listObjects(ctx.bucketName, {
        prefix: listPrefix,
        recursive: true,
        system: true,
        custom: true,
      });

      expect(objects.length).toBe(5);
      for (const obj of objects) {
        expect(obj.system).toBeDefined();
        expect(typeof obj.system.contentLength).toBe('number');
        expect(obj.system.contentLength).toBeGreaterThan(0);
      }
    });
  });

  describe('Empty Prefix Listing', () => {
    runTest('should return empty array for non-existent prefix', async () => {
      const objects = await ctx.project.listObjects(ctx.bucketName, {
        prefix: 'nonexistent-prefix-xyz-999/',
        recursive: true,
      });

      expect(objects.length).toBe(0);
    });
  });

  describe('Many Objects (Iterator Stress)', () => {
    const manyPrefix = `e2e-many-${Date.now()}/`;
    const count = 25;
    const manyKeys: string[] = [];

    beforeAll(async () => {
      if (!hasCredentials || !ctx) return;
      for (let i = 0; i < count; i++) {
        const key = `${manyPrefix}item-${String(i).padStart(4, '0')}.txt`;
        manyKeys.push(key);
        await uploadTestObject(ctx, key, Buffer.from(`data-${i}`));
      }
    });

    afterAll(async () => {
      if (!hasCredentials || !ctx) return;
      for (const key of manyKeys) {
        await deleteTestObject(ctx, key);
      }
    });

    runTest('should list many objects correctly', async () => {
      const objects = await ctx.project.listObjects(ctx.bucketName, {
        prefix: manyPrefix,
        recursive: true,
      });

      expect(objects.length).toBe(count);

      const returnedKeys = objects.map(o => o.key).sort();
      for (let i = 0; i < count; i++) {
        const expectedKey = `${manyPrefix}item-${String(i).padStart(4, '0')}.txt`;
        expect(returnedKeys).toContain(expectedKey);
      }
    }, 180000);
  });
});
