/**
 * Jest configuration for E2E tests.
 *
 * E2E tests run against a real Storj satellite and must execute in
 * series (--runInBand) because they share state (bucket, objects).
 *
 * Run all E2E tests:
 *   npm run test:e2e
 *
 * Run a single E2E test file:
 *   npm run test:e2e:upload
 *   npm run test:e2e:download
 *   etc.
 */
const base = require('./jest.config');

module.exports = {
  ...base,
  roots: ['<rootDir>/test/e2e'],
  testPathIgnorePatterns: ['/node_modules/', 'fullCycle'],
  testTimeout: 120000,
  // E2E tests share state and must run in order
  // Use --runInBand in the npm script
};
