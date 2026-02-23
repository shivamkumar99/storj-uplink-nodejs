/**
 * @file debug/index.ts
 * @brief Debug utilities for uplink-nodejs
 *
 * Provides debug functions for testing and development purposes.
 */

import { native } from '../native';

/**
 * Check if the internal handle universe is empty.
 *
 * This is a debug function that returns true if nothing is stored
 * in the global handle map. Useful for testing memory leaks and
 * ensuring all resources have been properly cleaned up.
 *
 * @returns Promise resolving to true if empty, false otherwise
 *
 * @example
 * ```typescript
 * import { internalUniverseIsEmpty } from 'uplink-nodejs';
 *
 * // After closing all resources
 * const isEmpty = await internalUniverseIsEmpty();
 * if (isEmpty) {
 *   console.log('All handles cleaned up successfully');
 * } else {
 *   console.warn('Some handles may not have been freed');
 * }
 * ```
 *
 * @example
 * ```typescript
 * // Use in tests to verify cleanup
 * describe('Resource cleanup', () => {
 *   it('should clean up all handles', async () => {
 *     const access = await parseAccess(accessGrant);
 *     const project = await access.openProject();
 *
 *     // Do some operations...
 *
 *     await project.close();
 *     // Note: access doesn't need explicit cleanup
 *
 *     const isEmpty = await internalUniverseIsEmpty();
 *     expect(isEmpty).toBe(true);
 *   });
 * });
 * ```
 */
export async function internalUniverseIsEmpty(): Promise<boolean> {
  return native.internalUniverseIsEmpty();
}

/**
 * Alias for internalUniverseIsEmpty with a more descriptive name
 * @see internalUniverseIsEmpty
 */
export const uplinkInternalUniverseIsEmpty = internalUniverseIsEmpty;

/**
 * Test function: throws a typed StorjError from native code.
 *
 * Always returns a rejected promise with a properly-typed StorjError
 * subclass instance. This is used to verify that `instanceof` checks
 * work on errors originating from native async operations.
 *
 * @param code - Error code (e.g. 0x13 for BUCKET_NOT_FOUND)
 * @param message - Error message / details string
 * @returns Promise that always rejects with a typed StorjError
 *
 * @example
 * ```typescript
 * import { testThrowTypedError, BucketNotFoundError } from 'uplink-nodejs';
 *
 * try {
 *   await testThrowTypedError(0x13, 'bucket "test" not found');
 * } catch (error) {
 *   console.log(error instanceof BucketNotFoundError); // true
 * }
 * ```
 */
export async function testThrowTypedError(code: number, message: string): Promise<never> {
  return native.testThrowTypedError(code, message);
}
