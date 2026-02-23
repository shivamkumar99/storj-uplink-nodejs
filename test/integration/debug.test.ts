/**
 * @file test/integration/debug.test.ts
 * @brief Integration tests for debug utilities
 *
 * Tests: internalUniverseIsEmpty
 *
 * This test does not require Storj credentials as it tests internal
 * handle management only.
 */

import { internalUniverseIsEmpty } from '../../src';

describe('Integration: Debug Utilities', () => {
  it('should return a boolean from internalUniverseIsEmpty', async () => {
    const result = await internalUniverseIsEmpty();

    expect(typeof result).toBe('boolean');
  });
});
