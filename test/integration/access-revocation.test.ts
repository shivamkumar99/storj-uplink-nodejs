/**
 * @file test/integration/access-revocation.test.ts
 * @brief Integration tests for access revocation chains
 *
 * Tests the chain: access → share → openProject → revokeAccess → closeProject
 * Covers revocation from access obtained via:
 *   - getAccess (parseAccess or requestAccessWithPassphrase)
 *   - share with different permission combinations
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

describe('Integration: Access Revocation', () => {
  let uplink: Uplink;

  beforeAll(() => {
    uplink = new Uplink();

    if (!hasAnyCredentials) {
      console.log('⏭️  Skipping access revocation tests:');
      console.log('   Set TEST_SATELLITE, TEST_API_KEY, TEST_PASSPHRASE to run these tests');
    }
  });

  describe('access → openProject → revokeAccess → closeProject', () => {
    const runTest = hasAnyCredentials ? it : it.skip;

    runTest('should open project, revoke shared access, then close project', async () => {
      const access = await getAccess(uplink);

      // Create a shared access to revoke
      const sharedAccess = await access.share(
        { allowDownload: true, allowUpload: false, allowList: true, allowDelete: false },
        []
      );
      expect(sharedAccess).toBeInstanceOf(AccessResultStruct);

      // Open project from the root access
      const project = await access.openProject();
      expect(project).toBeInstanceOf(ProjectResultStruct);

      try {
        // Revoke the shared access
        await project.revokeAccess(sharedAccess);
      } finally {
        await project.close();
      }
    });
  });

  describe('requestAccessWithPassphrase → openProject → revokeAccess → closeProject', () => {
    const runTest = hasCredentials ? it : it.skip;

    runTest('should open project, revoke shared access, then close project', async () => {
      const access = await uplink.requestAccessWithPassphrase(
        testSatellite!,
        testApiKey!,
        testPassphrase!
      );

      // Create a shared access to revoke
      const sharedAccess = await access.share(
        { allowDownload: true, allowUpload: true, allowList: true, allowDelete: false },
        []
      );
      expect(sharedAccess).toBeInstanceOf(AccessResultStruct);

      // Open project from the root access
      const project = await access.openProject();
      expect(project).toBeInstanceOf(ProjectResultStruct);

      try {
        // Revoke the shared access
        await project.revokeAccess(sharedAccess);
      } finally {
        await project.close();
      }
    }, 30000);
  });

  describe('share (different permissions) → openProject → revokeAccess → closeProject', () => {
    const runTest = hasAnyCredentials ? it : it.skip;

    const sharePermissions: { label: string; permission: Permission }[] = [
      {
        label: 'read-only',
        permission: { allowDownload: true, allowUpload: false, allowList: true, allowDelete: false },
      },
      {
        label: 'write-only',
        permission: { allowDownload: false, allowUpload: true, allowList: false, allowDelete: false },
      },
      {
        label: 'full access',
        permission: { allowDownload: true, allowUpload: true, allowList: true, allowDelete: true },
      },
      {
        label: 'list-only',
        permission: { allowDownload: false, allowUpload: false, allowList: true, allowDelete: false },
      },
      {
        label: 'upload-and-delete',
        permission: { allowDownload: false, allowUpload: true, allowList: false, allowDelete: true },
      },
      {
        label: 'no permissions',
        permission: { allowDownload: false, allowUpload: false, allowList: false, allowDelete: false },
      },
    ];

    for (const { label, permission } of sharePermissions) {
      if (label === 'no permissions') {
        // Storj uplink-c rejects share() with all permissions false ("permission is empty")
        runTest(`should reject sharing with ${label}`, async () => {
          const access = await getAccess(uplink);
          await expect(access.share(permission, [])).rejects.toThrow();
        });
        continue;
      }

      runTest(`should revoke ${label} shared access via openProject → revokeAccess → closeProject`, async () => {
        const access = await getAccess(uplink);

        // Share with specific permission
        const sharedAccess = await access.share(permission, []);
        expect(sharedAccess).toBeInstanceOf(AccessResultStruct);

        // Open project from root access
        const project = await access.openProject();
        expect(project).toBeInstanceOf(ProjectResultStruct);

        try {
          // Revoke the shared access
          await project.revokeAccess(sharedAccess);
        } finally {
          await project.close();
        }
      });
    }
  });
});
