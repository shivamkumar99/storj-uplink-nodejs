/**
 * @file test/setup.ts
 * @brief Jest test setup and global utilities
 */

import { config } from 'dotenv';
import { randomUUID } from 'crypto';

// Load test environment
config({ path: '.env.test' });

// Check if we have credentials for integration tests (grant OR satellite+apikey+passphrase)
const grant = process.env.TEST_ACCESS_GRANT;
const hasGrant = !!(grant && grant !== 'your_access_grant_here' && grant.length > 50);
const hasSatelliteCreds = !!(
  process.env.TEST_SATELLITE &&
  process.env.TEST_API_KEY &&
  process.env.TEST_PASSPHRASE &&
  process.env.TEST_PASSPHRASE !== 'your_passphrase_here' &&
  process.env.TEST_API_KEY !== 'your_api_key_here'
);
const hasCredentials = hasGrant || hasSatelliteCreds;

// Generate unique bucket name for test isolation
const testBucketName = `test-${Date.now()}-${randomUUID().substring(0, 8)}`;

// Extend global scope for test utilities using interface merging.
// This avoids `declare global { var ... }` which triggers eslint no-var.
interface TestGlobals {
  testBucketName: string;
  hasCredentials: boolean;
}

// Merge into globalThis via Window-style augmentation
const g = globalThis as unknown as TestGlobals;

// Set global test utilities
g.testBucketName = testBucketName;
g.hasCredentials = hasCredentials;

// Log test environment info
if (process.env.NODE_ENV !== 'test' || process.env.DEBUG) {
  console.log('\n📋 Test Environment:');
  console.log(`   Credentials: ${hasCredentials ? '✅ Available' : '❌ Not configured'}`);
  if (hasGrant) console.log('   Source: Pre-serialized access grant');
  else if (hasSatelliteCreds) console.log('   Source: Satellite + API key + passphrase');
  console.log(`   Test bucket: ${testBucketName}`);
  console.log('');
}

// Skip integration tests if no credentials
export function skipWithoutCredentials(): void {
  if (!hasCredentials) {
    console.log('⏭️  Skipping: No credentials configured');
  }
}

// Global cleanup handler
afterAll(async () => {
  // Any global cleanup can go here
});
