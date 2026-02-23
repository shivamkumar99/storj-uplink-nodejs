/**
 * @file test/e2e/08-access-sharing.test.ts
 * @brief E2E: Access sharing — permissions, serialize roundtrip, restricted ops
 */

import { AccessResultStruct } from '../../src';
import type { Permission, ObjectInfo } from '../../src/types';
import { hasAnyCredentials } from '../helpers/credentials';
import {
  getE2eContext,
  uploadTestObject,
  deleteTestObject,
  type E2eContext,
} from './helpers/e2e-context';

describe('E2E: Access Sharing', () => {
  const runTest = hasAnyCredentials ? it : it.skip;
  let ctx: E2eContext;

  beforeAll(async () => {
    if (!hasAnyCredentials) {
      console.log('⏭️  Skipping E2E access sharing tests: No valid credentials configured');
      return;
    }
    ctx = await getE2eContext();
  });

  describe('Share and Serialize Roundtrip', () => {
    const permissionCases: { label: string; permission: Permission }[] = [
      {
        label: 'read-only (download + list)',
        permission: { allowDownload: true, allowUpload: false, allowList: true, allowDelete: false },
      },
      {
        label: 'write-only (upload only)',
        permission: { allowDownload: false, allowUpload: true, allowList: false, allowDelete: false },
      },
      {
        label: 'full access',
        permission: { allowDownload: true, allowUpload: true, allowList: true, allowDelete: true },
      },
    ];

    for (const { label, permission } of permissionCases) {
      runTest(`should share with ${label} and roundtrip serialize`, async () => {
        const shared = await ctx.access.share(permission, []);
        expect(shared).toBeInstanceOf(AccessResultStruct);

        const serialized = await shared.serialize();
        expect(typeof serialized).toBe('string');
        expect(serialized.length).toBeGreaterThan(0);

        const reparsed = await ctx.uplink.parseAccess(serialized);
        expect(reparsed).toBeInstanceOf(AccessResultStruct);
      });
    }
  });

  describe('Restricted Read-Only Access', () => {
    runTest('should list and download with read-only shared access', async () => {
      const prefix = `e2e-shared-${Date.now()}/`;
      const keys = [
        `${prefix}shared-a.txt`,
        `${prefix}shared-b.txt`,
      ];

      // Upload with full access
      for (const key of keys) {
        await uploadTestObject(ctx, key, Buffer.from(`shared-content-${key}`));
      }

      try {
        const sharedAccess = await ctx.access.share(
          { allowDownload: true, allowUpload: false, allowList: true, allowDelete: false },
          [{ bucket: ctx.bucketName, prefix }],
        );

        const sharedProject = await sharedAccess.openProject();

        try {
          // List should work
          const objects: ObjectInfo[] = await sharedProject.listObjects(ctx.bucketName, {
            prefix,
            recursive: true,
          });
          expect(objects.length).toBe(2);

          // Download should work
          const download = await sharedProject.downloadObject(ctx.bucketName, keys[0]);
          const buf = Buffer.alloc(1024);
          const result = await download.read(buf, buf.length);
          expect(result.bytesRead).toBeGreaterThan(0);
          await download.close();

          // Upload should fail
          let uploadFailed = false;
          try {
            const upload = await sharedProject.uploadObject(
              ctx.bucketName, `${prefix}unauthorized.txt`,
            );
            const data = Buffer.from('should-not-work');
            await upload.write(data, data.length);
            await upload.commit();
          } catch (_) {
            uploadFailed = true;
          }
          expect(uploadFailed).toBe(true);
        } finally {
          await sharedProject.close();
        }
      } finally {
        for (const key of keys) {
          await deleteTestObject(ctx, key);
        }
      }
    });
  });

  describe('Time-Restricted Access', () => {
    runTest('should share access with time restrictions', async () => {
      const now = new Date();
      const oneHourFromNow = new Date(now.getTime() + 60 * 60 * 1000);
      const oneDayFromNow = new Date(now.getTime() + 24 * 60 * 60 * 1000);

      const shared = await ctx.access.share(
        {
          allowDownload: true,
          allowUpload: true,
          allowList: true,
          allowDelete: false,
          notBefore: oneHourFromNow,
          notAfter: oneDayFromNow,
        },
        [{ bucket: ctx.bucketName }],
      );

      expect(shared).toBeInstanceOf(AccessResultStruct);

      const serialized = await shared.serialize();
      expect(typeof serialized).toBe('string');
      expect(serialized.length).toBeGreaterThan(0);
    });
  });

  describe('Empty Permissions', () => {
    runTest('should create a useless access with all permissions false', async () => {
      // uplink-c allows creating an access with all permissions false.
      // The result is a valid but useless access — no operations will succeed.
      const shared = await ctx.access.share(
        { allowDownload: false, allowUpload: false, allowList: false, allowDelete: false },
        [],
      );
      expect(shared).toBeInstanceOf(AccessResultStruct);

      // The shared access should be serializable
      const serialized = await shared.serialize();
      expect(typeof serialized).toBe('string');
      expect(serialized.length).toBeGreaterThan(0);

      // Verify the access is truly restricted — opening a project and listing
      // should fail with permission denied
      const restrictedProject = await shared.openProject();
      try {
        await expect(
          restrictedProject.listBuckets(),
        ).rejects.toThrow();
      } finally {
        await restrictedProject.close();
      }
    });
  });
});
