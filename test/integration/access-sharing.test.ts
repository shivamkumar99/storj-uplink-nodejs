/**
 * @file test/integration/access-sharing.test.ts
 * @brief Integration tests for access sharing and serialization chains
 *
 * Tests the chain: access → share (various permissions) → serialize → parseAccess roundtrip
 *
 * Requires environment variables:
 * - TEST_SATELLITE, TEST_API_KEY, TEST_PASSPHRASE
 * - (Optional) TEST_ACCESS_GRANT
 */

import { Uplink, AccessResultStruct } from '../../src';
import type { Permission, SharePrefix } from '../../src/types';
import { hasAnyCredentials, getAccess } from '../helpers/credentials';

describe('Integration: Access Sharing', () => {
  const runTest = hasAnyCredentials ? it : it.skip;

  let uplink: Uplink;

  beforeAll(() => {
    uplink = new Uplink();

    if (!hasAnyCredentials) {
      console.log('⏭️  Skipping access sharing tests:');
      console.log('   Set TEST_SATELLITE, TEST_API_KEY, TEST_PASSPHRASE to run these tests');
    }
  });

  const testPrefixes: SharePrefix[] = [
    { bucket: 'test-share-bucket', prefix: 'data/' },
  ];

  /**
   * 6 distinct permission combinations covering all boolean permission fields.
   * Each tests a different real-world access pattern.
   */
  const permissionCombinations: { label: string; permission: Permission }[] = [
    {
      label: 'read-only (download + list)',
      permission: {
        allowDownload: true,
        allowUpload: false,
        allowList: true,
        allowDelete: false,
      },
    },
    {
      label: 'write-only (upload only)',
      permission: {
        allowDownload: false,
        allowUpload: true,
        allowList: false,
        allowDelete: false,
      },
    },
    {
      label: 'full access (all permissions)',
      permission: {
        allowDownload: true,
        allowUpload: true,
        allowList: true,
        allowDelete: true,
      },
    },
    {
      label: 'list-only (browse without download)',
      permission: {
        allowDownload: false,
        allowUpload: false,
        allowList: true,
        allowDelete: false,
      },
    },
    {
      label: 'upload-and-delete (no read)',
      permission: {
        allowDownload: false,
        allowUpload: true,
        allowList: false,
        allowDelete: true,
      },
    },
    {
      label: 'no permissions (all denied)',
      permission: {
        allowDownload: false,
        allowUpload: false,
        allowList: false,
        allowDelete: false,
      },
    },
  ];

  describe('share with 6 permission combinations → serialize', () => {
    for (const { label, permission } of permissionCombinations) {
      // uplink-c rejects sharing with all permissions set to false
      const allDenied =
        !permission.allowDownload &&
        !permission.allowUpload &&
        !permission.allowList &&
        !permission.allowDelete;

      if (allDenied) {
        runTest(`should reject share with ${label}`, async () => {
          const access = await getAccess(uplink);
          await expect(access.share(permission, testPrefixes)).rejects.toThrow(/permission/i);
        });
      } else {
        runTest(`should share access with ${label} and serialize`, async () => {
          const access = await getAccess(uplink);

          const sharedAccess = await access.share(permission, testPrefixes);
          expect(sharedAccess).toBeInstanceOf(AccessResultStruct);

          // Serialize shared access
          const serialized = await sharedAccess.serialize();
          expect(typeof serialized).toBe('string');
          expect(serialized.length).toBeGreaterThan(0);

          // Verify roundtrip
          const reparsed = await uplink.parseAccess(serialized);
          expect(reparsed).toBeInstanceOf(AccessResultStruct);
        });
      }
    }
  });

  describe('share with time restrictions → serialize', () => {
    runTest('should share access with notBefore and notAfter time restrictions', async () => {
      const access = await getAccess(uplink);

      const now = new Date();
      const oneHourFromNow = new Date(now.getTime() + 60 * 60 * 1000);
      const oneDayFromNow = new Date(now.getTime() + 24 * 60 * 60 * 1000);

      const timeRestrictedPermission: Permission = {
        allowDownload: true,
        allowUpload: true,
        allowList: true,
        allowDelete: false,
        notBefore: oneHourFromNow,
        notAfter: oneDayFromNow,
      };

      const sharedAccess = await access.share(timeRestrictedPermission, testPrefixes);
      expect(sharedAccess).toBeInstanceOf(AccessResultStruct);

      const serialized = await sharedAccess.serialize();
      expect(typeof serialized).toBe('string');
      expect(serialized.length).toBeGreaterThan(0);

      const reparsed = await uplink.parseAccess(serialized);
      expect(reparsed).toBeInstanceOf(AccessResultStruct);
    });
  });

  describe('share with prefix variations', () => {
    runTest('should share access with single prefix restriction', async () => {
      const access = await getAccess(uplink);
      const bucketName = `test-${Date.now()}`;

      const sharedAccess = await access.share(
        {
          allowDownload: true,
          allowUpload: true,
          allowDelete: false,
          allowList: true,
        },
        [{ bucket: bucketName, prefix: 'shared/' }]
      );

      expect(sharedAccess).toBeInstanceOf(AccessResultStruct);
    });

    runTest('should share access with multiple prefix restrictions', async () => {
      const access = await getAccess(uplink);

      const multiplePrefixes: SharePrefix[] = [
        { bucket: 'bucket-a', prefix: 'public/' },
        { bucket: 'bucket-a', prefix: 'shared/' },
        { bucket: 'bucket-b' },
      ];

      const sharedAccess = await access.share(
        { allowDownload: true, allowUpload: true, allowList: true, allowDelete: false },
        multiplePrefixes
      );
      expect(sharedAccess).toBeInstanceOf(AccessResultStruct);

      const serialized = await sharedAccess.serialize();
      expect(typeof serialized).toBe('string');
      expect(serialized.length).toBeGreaterThan(0);
    });

    runTest('should share access with empty prefixes (project-wide)', async () => {
      const access = await getAccess(uplink);

      const sharedAccess = await access.share(
        { allowDownload: true, allowUpload: false, allowList: true, allowDelete: false },
        []
      );
      expect(sharedAccess).toBeInstanceOf(AccessResultStruct);

      const serialized = await sharedAccess.serialize();
      expect(typeof serialized).toBe('string');
      expect(serialized.length).toBeGreaterThan(0);
    });
  });
});
