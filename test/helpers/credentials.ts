/**
 * @file test/helpers/credentials.ts
 * @brief Shared test credential helpers
 *
 * Centralizes credential checking and access generation so that tests
 * no longer need a pre-serialized TEST_ACCESS_GRANT. Instead, the grant
 * is derived at runtime from TEST_SATELLITE + TEST_API_KEY + TEST_PASSPHRASE.
 */

import { Uplink, AccessResultStruct } from '../../src';

// ---------------------------------------------------------------------------
// Raw env values
// ---------------------------------------------------------------------------
export const testGrant = process.env.TEST_ACCESS_GRANT;
export const testSatellite = process.env.TEST_SATELLITE;
export const testApiKey = process.env.TEST_API_KEY;
export const testPassphrase = process.env.TEST_PASSPHRASE;

// ---------------------------------------------------------------------------
// Credential checks
// ---------------------------------------------------------------------------

/** True when a pre-serialized grant is present and not a placeholder. */
export const hasAccessGrant = !!(
  testGrant &&
  testGrant !== 'your_access_grant_here' &&
  testGrant.length > 50
);

/** True when satellite + API key + passphrase are configured. */
export const hasCredentials = !!(
  testSatellite &&
  testApiKey &&
  testPassphrase &&
  testPassphrase !== 'your_passphrase_here' &&
  testApiKey !== 'your_api_key_here'
);

/** True when we can obtain an access by any means. */
export const hasAnyCredentials = hasAccessGrant || hasCredentials;

// ---------------------------------------------------------------------------
// Access helpers
// ---------------------------------------------------------------------------

/**
 * Obtain an AccessResultStruct from whatever credentials are available.
 *
 * Prefers the pre-serialized grant (parseAccess) when present, otherwise
 * falls back to requestAccessWithPassphrase.
 */
export async function getAccess(uplink: Uplink): Promise<AccessResultStruct> {
  if (hasAccessGrant && testGrant) {
    return uplink.parseAccess(testGrant);
  }
  if (hasCredentials && testSatellite && testApiKey && testPassphrase) {
    return uplink.requestAccessWithPassphrase(testSatellite, testApiKey, testPassphrase);
  }
  throw new Error(
    'No valid credentials configured – set TEST_ACCESS_GRANT or TEST_SATELLITE + TEST_API_KEY + TEST_PASSPHRASE'
  );
}

/**
 * Obtain a serialized access grant string.
 *
 * If TEST_ACCESS_GRANT is set it's returned directly; otherwise the grant is
 * derived at runtime via requestAccessWithPassphrase → serialize().
 */
export async function getSerializedGrant(uplink: Uplink): Promise<string> {
  if (hasAccessGrant && testGrant) {
    return testGrant;
  }
  if (hasCredentials && testSatellite && testApiKey && testPassphrase) {
    const access = await uplink.requestAccessWithPassphrase(
      testSatellite,
      testApiKey,
      testPassphrase
    );
    return access.serialize();
  }
  throw new Error('No valid credentials configured');
}
