/**
 * @file test/integration/encryption.test.ts
 * @brief Integration tests for encryption key operations
 *
 * Tests: deriveEncryptionKey, overrideEncryptionKey
 *
 * Requires environment variables:
 * - TEST_SATELLITE + TEST_API_KEY + TEST_PASSPHRASE
 * - (Optional) TEST_ACCESS_GRANT
 */

import { Uplink } from '../../src';
import { hasAnyCredentials, getAccess } from '../helpers/credentials';

describe('Integration: Encryption Key Operations', () => {
  const runTest = hasAnyCredentials ? it : it.skip;

  let uplink: Uplink;

  beforeAll(() => {
    uplink = new Uplink();

    if (!hasAnyCredentials) {
      console.log('⏭️  Skipping encryption tests:');
      console.log('   Set TEST_SATELLITE + TEST_API_KEY + TEST_PASSPHRASE to run these tests');
    }
  });

  describe('deriveEncryptionKey', () => {
    runTest('should derive encryption key from passphrase and salt', async () => {
      const salt = Buffer.from('unique-salt-for-testing');
      const encryptionKey = await uplink.uplinkDeriveEncryptionKey('test-passphrase', salt);

      expect(encryptionKey).toBeDefined();
      expect(encryptionKey._handle).toBeDefined();
      // Handle is an N-API external (opaque object), not a number
      expect(typeof encryptionKey._handle).toBe('object');
    });

    runTest('should derive different keys for different salts', async () => {
      const salt1 = Buffer.from('salt-one');
      const salt2 = Buffer.from('salt-two');

      const key1 = await uplink.uplinkDeriveEncryptionKey('passphrase', salt1);
      const key2 = await uplink.uplinkDeriveEncryptionKey('passphrase', salt2);

      // Different salts should produce different handles
      expect(key1._handle).not.toBe(key2._handle);
    });
  });

  describe('overrideEncryptionKey', () => {
    runTest('should override encryption key on access', async () => {
      const access = await getAccess(uplink);
      const salt = Buffer.from('test-override-salt');
      const encryptionKey = await uplink.uplinkDeriveEncryptionKey('custom-passphrase', salt);

      // Override encryption key for a specific bucket/prefix
      await expect(
        access.overrideEncryptionKey('test-bucket', 'prefix/', encryptionKey)
      ).resolves.not.toThrow();
    });
  });
});
