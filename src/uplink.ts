/**
 * @file uplink.ts
 * @brief Main Uplink class for uplink-nodejs
 *
 * Entry point for all Storj operations.
 */

import type { UplinkConfig, EncryptionKey } from './types';
import { AccessResultStruct } from './access';
import { native } from './native';

/** Validation helper for required non-empty string parameters */
function requireString(value: unknown, name: string): void {
  if (typeof value !== 'string' || value === '') {
    throw new TypeError(`${name} must be a non-empty string`);
  }
}

/**
 * Main entry point for Storj Uplink operations.
 *
 * Use this class to obtain access grants and connect to Storj.
 *
 * @example
 * ```typescript
 * import { Uplink } from 'uplink-nodejs';
 *
 * const uplink = new Uplink();
 * const access = await uplink.requestAccessWithPassphrase(
 *   'satellite-address',
 *   'api-key',
 *   'passphrase'
 * );
 * ```
 */
export class Uplink {
  /**
   * Parse a serialized access grant string.
   *
   * @param accessGrant - The serialized access grant string
   * @returns Promise resolving to an AccessResultStruct
   *
   * @example
   * ```typescript
   * const access = await uplink.parseAccess('1J5F2K...');
   * ```
   */
  async parseAccess(accessGrant: string): Promise<AccessResultStruct> {
    requireString(accessGrant, 'accessGrant');

    const handle = await native.parseAccess(accessGrant);
    return new AccessResultStruct(handle);
  }

  /**
   * Request a new access grant using satellite address, API key, and passphrase.
   *
   * This is the most common way to connect to Storj. The passphrase is used
   * to derive encryption keys for your data.
   *
   * @param satellite - The satellite address (e.g., 'us1.storj.io:7777')
   * @param apiKey - Your API key from the Storj console
   * @param passphrase - Your encryption passphrase (keep this secret!)
   * @returns Promise resolving to an AccessResultStruct
   *
   * @example
   * ```typescript
   * const access = await uplink.requestAccessWithPassphrase(
   *   'us1.storj.io:7777',
   *   'your-api-key',
   *   'your-secret-passphrase'
   * );
   * ```
   */
  async requestAccessWithPassphrase(
    satellite: string,
    apiKey: string,
    passphrase: string
  ): Promise<AccessResultStruct> {
    requireString(satellite, 'satellite');
    requireString(apiKey, 'apiKey');
    requireString(passphrase, 'passphrase');

    const handle = await native.requestAccessWithPassphrase(satellite, apiKey, passphrase);
    return new AccessResultStruct(handle);
  }

  /**
   * Request a new access grant with custom configuration.
   *
   * @param config - Configuration options
   * @param satellite - The satellite address
   * @param apiKey - Your API key
   * @param passphrase - Your encryption passphrase
   * @returns Promise resolving to an AccessResultStruct
   *
   * @example
   * ```typescript
   * const access = await uplink.configRequestAccessWithPassphrase(
   *   { dialTimeoutMilliseconds: 30000 },
   *   'us1.storj.io:7777',
   *   'your-api-key',
   *   'your-secret-passphrase'
   * );
   * ```
   */
  async configRequestAccessWithPassphrase(
    config: UplinkConfig,
    satellite: string,
    apiKey: string,
    passphrase: string
  ): Promise<AccessResultStruct> {
    if (config == null || typeof config !== 'object') {
      throw new TypeError('config must be an object');
    }
    requireString(satellite, 'satellite');
    requireString(apiKey, 'apiKey');
    requireString(passphrase, 'passphrase');

    const handle = await native.configRequestAccessWithPassphrase(
      config,
      satellite,
      apiKey,
      passphrase
    );
    return new AccessResultStruct(handle);
  }

  /**
   * Derive an encryption key from a passphrase and salt.
   *
   * This can be used with `access.overrideEncryptionKey()` to implement
   * multitenancy where different users have different encryption keys.
   *
   * @param passphrase - The passphrase to derive the key from
   * @param salt - A unique salt value (should be unique per bucket/prefix)
   * @returns Promise resolving to an EncryptionKey
   *
   * @example
   * ```typescript
   * const salt = Buffer.from('unique-salt-for-user-123');
   * const encryptionKey = await uplink.uplinkDeriveEncryptionKey('user-passphrase', salt);
   * await access.overrideEncryptionKey('my-bucket', 'user-123/', encryptionKey);
   * ```
   */
  async uplinkDeriveEncryptionKey(passphrase: string, salt: Buffer): Promise<EncryptionKey> {
    requireString(passphrase, 'passphrase');
    if (!Buffer.isBuffer(salt)) {
      throw new TypeError('salt must be a Buffer');
    }

    const handle = await native.deriveEncryptionKey(passphrase, salt);
    return { _handle: handle as number };
  }
}
