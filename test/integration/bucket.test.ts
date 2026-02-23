/**
 * @file test/integration/bucket.test.ts
 * @brief Integration tests for bucket operations chained from access
 *
 * Tests the chains:
 *   1. access → openProject → createBucket → ensureBucket → statBucket
 *      → deleteBucket → ensureBucket → closeProject
 *   2. requestAccessWithPassphrase → openProject → (same bucket ops) → closeProject
 *   3. requestAccessWithPassphrase → share (different permissions) → openProject
 *      → (bucket ops) → closeProject
 *
 * Requires environment variables:
 * - TEST_SATELLITE, TEST_API_KEY, TEST_PASSPHRASE
 * - (Optional) TEST_ACCESS_GRANT
 */

import { Uplink, AccessResultStruct, ProjectResultStruct } from '../../src';
import type { Permission } from '../../src/types';
import {
  hasAnyCredentials,
  hasCredentials,
  getAccess,
  testSatellite,
  testApiKey,
  testPassphrase,
} from '../helpers/credentials';

/**
 * Helper to check that a value is a Unix timestamp (number, seconds).
 * The native layer returns raw int64 Unix timestamps.
 */
function expectTimestamp(value: unknown): void {
  expect(value).toBeDefined();
  expect(typeof value).toBe('number');
  expect(Number.isFinite(value as number)).toBe(true);
  expect(value as number).toBeGreaterThan(0);
}

describe('Integration: Bucket Operations', () => {
  let uplink: Uplink;

  beforeAll(() => {
    uplink = new Uplink();

    if (!hasAnyCredentials) {
      console.log('⏭️  Skipping bucket integration tests:');
      console.log('   Set TEST_SATELLITE, TEST_API_KEY, TEST_PASSPHRASE to run these tests');
    }
  });

  describe('access → openProject → bucket ops → closeProject', () => {
    const runTest = hasAnyCredentials ? it : it.skip;
    const bucketName = `int-pa-${Date.now()}`;

    runTest('should run full bucket lifecycle from parsed access', async () => {
      const access = await getAccess(uplink);
      const project = await access.openProject();
      expect(project).toBeInstanceOf(ProjectResultStruct);

      try {
        // createBucket
        const created = await project.createBucket(bucketName);
        expect(created).toBeDefined();
        expect(created.name).toBe(bucketName);
        expectTimestamp(created.created);

        // ensureBucket (idempotent on existing bucket)
        const ensured = await project.ensureBucket(bucketName);
        expect(ensured).toBeDefined();
        expect(ensured.name).toBe(bucketName);

        // statBucket
        const stat = await project.statBucket(bucketName);
        expect(stat).toBeDefined();
        expect(stat.name).toBe(bucketName);
        expectTimestamp(stat.created);

        // deleteBucket
        await project.deleteBucket(bucketName);

        // statBucket should fail after delete
        await expect(project.statBucket(bucketName)).rejects.toThrow();

        // ensureBucket (re-creates after delete)
        const reensured = await project.ensureBucket(bucketName);
        expect(reensured).toBeDefined();
        expect(reensured.name).toBe(bucketName);

        // cleanup
        await project.deleteBucket(bucketName);
      } finally {
        await project.close();
      }
    });
  });

  describe('requestAccessWithPassphrase → openProject → bucket ops → closeProject', () => {
    const runTest = hasCredentials ? it : it.skip;
    const bucketName = `int-ra-${Date.now()}`;

    runTest('should run full bucket lifecycle from requested access', async () => {
      const access = await uplink.requestAccessWithPassphrase(
        testSatellite!,
        testApiKey!,
        testPassphrase!,
      );
      const project = await access.openProject();
      expect(project).toBeInstanceOf(ProjectResultStruct);

      try {
        // createBucket
        const created = await project.createBucket(bucketName);
        expect(created).toBeDefined();
        expect(created.name).toBe(bucketName);
        expectTimestamp(created.created);

        // ensureBucket (idempotent on existing bucket)
        const ensured = await project.ensureBucket(bucketName);
        expect(ensured).toBeDefined();
        expect(ensured.name).toBe(bucketName);

        // statBucket
        const stat = await project.statBucket(bucketName);
        expect(stat).toBeDefined();
        expect(stat.name).toBe(bucketName);
        expectTimestamp(stat.created);

        // deleteBucket
        await project.deleteBucket(bucketName);

        // statBucket should fail after delete
        await expect(project.statBucket(bucketName)).rejects.toThrow();

        // ensureBucket (re-creates after delete)
        const reensured = await project.ensureBucket(bucketName);
        expect(reensured).toBeDefined();
        expect(reensured.name).toBe(bucketName);

        // cleanup
        await project.deleteBucket(bucketName);
      } finally {
        await project.close();
      }
    }, 60000);
  });

  describe('shared access (different permissions) → openProject → bucket ops → closeProject', () => {
    const runTest = hasCredentials ? it : it.skip;

    /**
     * Storj permission model:
     *
     * - Bucket creation requires the shared access to have allowUpload set.
     *   Without it, the satellite returns "permission denied".
     *
     * - Sharing with all permissions set to false causes an "permission is
     *   empty" error from the uplink-c library itself (not from the satellite).
     */
    const sharePermissions: { label: string; permission: Permission; canCreate: boolean }[] = [
      {
        label: 'full access',
        permission: { allowDownload: true, allowUpload: true, allowList: true, allowDelete: true },
        canCreate: true,
      },
      {
        label: 'read-only (download + list)',
        permission: { allowDownload: true, allowUpload: false, allowList: true, allowDelete: false },
        canCreate: false,
      },
      {
        label: 'write-only (upload only)',
        permission: { allowDownload: false, allowUpload: true, allowList: false, allowDelete: false },
        canCreate: true,
      },
      {
        label: 'list-only',
        permission: { allowDownload: false, allowUpload: false, allowList: true, allowDelete: false },
        canCreate: false,
      },
      {
        label: 'upload-and-delete',
        permission: { allowDownload: false, allowUpload: true, allowList: false, allowDelete: true },
        canCreate: true,
      },
    ];

    for (const { label, permission, canCreate } of sharePermissions) {
      if (canCreate) {
        runTest(`should create bucket with ${label} shared access`, async () => {
          const bucketName = `int-sh-${Date.now()}`;

          const rootAccess = await uplink.requestAccessWithPassphrase(
            testSatellite!,
            testApiKey!,
            testPassphrase!,
          );

          const sharedAccess = await rootAccess.share(permission, []);
          expect(sharedAccess).toBeInstanceOf(AccessResultStruct);

          const project = await sharedAccess.openProject();
          expect(project).toBeInstanceOf(ProjectResultStruct);

          try {
            // createBucket should succeed (requires allowUpload)
            const created = await project.createBucket(bucketName);
            expect(created).toBeDefined();
            expect(created.name).toBe(bucketName);
            expectTimestamp(created.created);
          } finally {
            await project.close();
            // Cleanup with root access (shared access may lack delete permission)
            const rootProject = await rootAccess.openProject();
            try {
              await rootProject.deleteBucket(bucketName);
            } catch (_) { /* bucket may already be deleted */ }
            await rootProject.close();
          }
        }, 60000);
      } else {
        runTest(`should deny bucket creation with ${label} shared access`, async () => {
          const bucketName = `int-sh-deny-${Date.now()}`;

          const rootAccess = await uplink.requestAccessWithPassphrase(
            testSatellite!,
            testApiKey!,
            testPassphrase!,
          );

          const sharedAccess = await rootAccess.share(permission, []);
          expect(sharedAccess).toBeInstanceOf(AccessResultStruct);

          const project = await sharedAccess.openProject();
          expect(project).toBeInstanceOf(ProjectResultStruct);

          try {
            // createBucket should fail — satellite requires upload + delete permissions
            await expect(project.createBucket(bucketName)).rejects.toThrow(/permission denied/i);
          } finally {
            await project.close();
          }
        }, 60000);
      }
    }

    runTest('should reject share with empty permissions (all false)', async () => {
      const rootAccess = await uplink.requestAccessWithPassphrase(
        testSatellite!,
        testApiKey!,
        testPassphrase!,
      );

      // uplink-c rejects sharing with all permissions set to false
      await expect(
        rootAccess.share(
          { allowDownload: false, allowUpload: false, allowList: false, allowDelete: false },
          [],
        ),
      ).rejects.toThrow(/permission/i);
    }, 60000);
  });
});
