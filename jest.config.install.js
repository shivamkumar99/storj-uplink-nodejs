/**
 * Jest configuration for install method tests.
 *
 * These tests run `make install-*` commands and verify the Makefile
 * installation targets. They are slow and require platform-specific
 * prerequisites (Go, C compiler, etc.), so they are excluded from
 * the default test suite and run separately via:
 *
 *   npm run test:install
 */
module.exports = {
  preset: 'ts-jest',
  testEnvironment: 'node',
  roots: ['<rootDir>/test/install'],
  testMatch: ['**/*.test.ts'],
  // No testPathIgnorePatterns — we want to run install tests
  testTimeout: 600000, // 10 minutes
  verbose: true,
};
