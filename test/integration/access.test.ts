/**
 * @file test/integration/access.test.ts
 * @brief Integration tests for core access operations
 *
 * Tests: parseAccess, requestAccessWithPassphrase, satelliteAddress,
 *        configRequestAccessWithPassphrase, serialize, openProject
 *
 * Related test files:
 * - access-sharing.test.ts    → share + serialize chains
 * - access-revocation.test.ts → share → openProject → revokeAccess → closeProject
 * - encryption.test.ts        → deriveEncryptionKey, overrideEncryptionKey
 *
 * Requires environment variables:
 * - TEST_SATELLITE: Satellite address
 * - TEST_API_KEY: API key
 * - TEST_PASSPHRASE: Encryption passphrase
 * - (Optional) TEST_ACCESS_GRANT: Pre-serialized access grant
 */

import { Uplink, AccessResultStruct, ProjectResultStruct } from '../../src';
import {
  hasAnyCredentials,
  hasCredentials,
  getSerializedGrant,
  testSatellite,
  testApiKey,
  testPassphrase,
} from '../helpers/credentials';

describe('Integration: Access Operations', () => {
  let uplink: Uplink;

  beforeAll(() => {
    uplink = new Uplink();

    if (!hasAnyCredentials) {
      console.log('⏭️  Skipping access integration tests:');
      console.log('   Set TEST_SATELLITE, TEST_API_KEY, TEST_PASSPHRASE to run these tests');
    }
  });

  describe('parseAccess', () => {
    const runTest = hasAnyCredentials ? it : it.skip;

    runTest('should parse a valid access grant', async () => {
      const grant = await getSerializedGrant(uplink);
      const access = await uplink.parseAccess(grant);

      expect(access).toBeInstanceOf(AccessResultStruct);
      // Handle is an N-API external (opaque pointer), not a number
      expect(access['_handle']).toBeDefined();
      expect(access['_handle']).not.toBeNull();
    });

    runTest('should open a project from parsed access', async () => {
      const grant = await getSerializedGrant(uplink);
      const access = await uplink.parseAccess(grant);
      const project = await access.openProject();

      expect(project).toBeInstanceOf(ProjectResultStruct);

      await project.close();
    });

    runTest('should serialize and re-parse access', async () => {
      const grant = await getSerializedGrant(uplink);
      const access = await uplink.parseAccess(grant);
      const serialized = await access.serialize();

      expect(typeof serialized).toBe('string');
      expect(serialized.length).toBeGreaterThan(0);

      // Re-parse the serialized access
      const reparsed = await uplink.parseAccess(serialized);
      expect(reparsed).toBeInstanceOf(AccessResultStruct);

      // Should be able to open project with reparsed access
      const project = await reparsed.openProject();
      await project.close();
    });

    runTest('should reject invalid access grant', async () => {
      await expect(uplink.parseAccess('invalid-grant')).rejects.toThrow();
    });
  });

  describe('requestAccessWithPassphrase', () => {
    const runTest = hasCredentials ? it : it.skip;

    runTest('should request access with valid credentials', async () => {
      const access = await uplink.requestAccessWithPassphrase(
        testSatellite!,
        testApiKey!,
        testPassphrase!
      );

      expect(access).toBeInstanceOf(AccessResultStruct);
    });

    runTest('should open a project from requested access', async () => {
      const access = await uplink.requestAccessWithPassphrase(
        testSatellite!,
        testApiKey!,
        testPassphrase!
      );
      const project = await access.openProject();

      expect(project).toBeInstanceOf(ProjectResultStruct);

      await project.close();
    });

    runTest('should serialize requested access', async () => {
      const access = await uplink.requestAccessWithPassphrase(
        testSatellite!,
        testApiKey!,
        testPassphrase!
      );
      const serialized = await access.serialize();

      expect(typeof serialized).toBe('string');
      expect(serialized.length).toBeGreaterThan(0);
    });

    runTest('should reject invalid satellite address', async () => {
      await expect(
        uplink.requestAccessWithPassphrase(
          'invalid-satellite:9999',
          testApiKey!,
          testPassphrase!
        )
      ).rejects.toThrow();
    }, 30000);

    runTest('should reject invalid API key', async () => {
      await expect(
        uplink.requestAccessWithPassphrase(
          testSatellite!,
          'invalid-api-key',
          testPassphrase!
        )
      ).rejects.toThrow();
    }, 30000);
  });

  describe('satelliteAddress', () => {
    const runTest = hasCredentials ? it : it.skip;

    runTest('should return satellite address matching the one used in requestAccessWithPassphrase', async () => {
      const access = await uplink.requestAccessWithPassphrase(
        testSatellite!,
        testApiKey!,
        testPassphrase!
      );

      const satelliteAddr = await access.satelliteAddress();

      expect(typeof satelliteAddr).toBe('string');
      expect(satelliteAddr.length).toBeGreaterThan(0);

      // TEST_SATELLITE format is "nodeID@host:port"
      // satelliteAddress() returns the full address including node ID
      const hostPort = testSatellite!.split('@').pop() || '';
      const host = hostPort.split(':')[0];
      expect(satelliteAddr).toContain(host);
    }, 30000);

    runTest('should return consistent satellite address after serialize roundtrip', async () => {
      const access = await uplink.requestAccessWithPassphrase(
        testSatellite!,
        testApiKey!,
        testPassphrase!
      );

      const originalAddr = await access.satelliteAddress();
      const serialized = await access.serialize();
      const reparsed = await uplink.parseAccess(serialized);
      const roundtripAddr = await reparsed.satelliteAddress();

      expect(roundtripAddr).toBe(originalAddr);
    }, 30000);
  });

  describe('configRequestAccessWithPassphrase', () => {
    const runTest = hasCredentials ? it : it.skip;

    runTest('should request access with custom config', async () => {
      const access = await uplink.configRequestAccessWithPassphrase(
        { dialTimeoutMilliseconds: 30000 },
        testSatellite!,
        testApiKey!,
        testPassphrase!
      );

      expect(access).toBeInstanceOf(AccessResultStruct);
    });

    runTest('should respect timeout configuration', async () => {
      // Very short timeout should fail for invalid satellite
      await expect(
        uplink.configRequestAccessWithPassphrase(
          { dialTimeoutMilliseconds: 1 },
          'invalid-satellite:9999',
          'api-key',
          'passphrase'
        )
      ).rejects.toThrow();
    }, 5000);
  });
});
