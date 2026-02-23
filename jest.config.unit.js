const base = require('./jest.config');

module.exports = {
  ...base,
  // Only run unit tests
  roots: ['<rootDir>/test/unit'],
  // Add unit-test setup that mocks the native module
  setupFilesAfterEnv: [
    ...(base.setupFilesAfterEnv || []),
    '<rootDir>/test/unit/jest.setup.ts',
  ],
};
