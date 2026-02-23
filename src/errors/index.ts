/**
 * @file errors/index.ts
 * @brief Error handling module for Storj uplink operations
 *
 * Provides typed error classes that can be used with `instanceof` to
 * determine the specific error type thrown by uplink operations.
 *
 * @example
 * ```typescript
 * import {
 *     StorjError,
 *     BucketNotFoundError,
 *     ObjectNotFoundError,
 *     ErrorCodes
 * } from 'uplink-nodejs';
 *
 * try {
 *     const info = await project.statObject('mybucket', 'mykey');
 * } catch (error) {
 *     if (error instanceof ObjectNotFoundError) {
 *         console.log('Object does not exist');
 *     } else if (error instanceof BucketNotFoundError) {
 *         console.log('Bucket does not exist');
 *     } else if (error instanceof StorjError) {
 *         console.log(`Storj error (code ${error.code}): ${error.message}`);
 *     } else {
 *         throw error; // Re-throw unknown errors
 *     }
 * }
 * ```
 */

// Export error codes
export { ErrorCodes, ErrorCode } from './codes';

// Export all exception classes
export {
  StorjError,
  // General errors
  InternalError,
  CanceledError,
  InvalidHandleError,
  TooManyRequestsError,
  BandwidthLimitExceededError,
  StorageLimitExceededError,
  SegmentsLimitExceededError,
  PermissionDeniedError,
  // Bucket errors
  BucketNameInvalidError,
  BucketAlreadyExistsError,
  BucketNotEmptyError,
  BucketNotFoundError,
  // Object errors
  ObjectKeyInvalidError,
  ObjectNotFoundError,
  UploadDoneError,
  // Edge errors
  EdgeAuthDialFailedError,
  EdgeRegisterAccessFailedError,
} from './exceptions';

// Export factory functions and utilities
export {
  NativeError,
  createStorjError,
  throwStorjError,
  fromNativeError,
  isStorjError,
  hasErrorCode,
} from './factory';
