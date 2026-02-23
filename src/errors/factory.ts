/**
 * @file errors/factory.ts
 * @brief Factory function to create typed errors from uplink-c error codes
 */

import { ErrorCodes, ErrorCode } from './codes';
import {
  StorjError,
  InternalError,
  CanceledError,
  InvalidHandleError,
  TooManyRequestsError,
  BandwidthLimitExceededError,
  StorageLimitExceededError,
  SegmentsLimitExceededError,
  PermissionDeniedError,
  BucketNameInvalidError,
  BucketAlreadyExistsError,
  BucketNotEmptyError,
  BucketNotFoundError,
  ObjectKeyInvalidError,
  ObjectNotFoundError,
  UploadDoneError,
  EdgeAuthDialFailedError,
  EdgeRegisterAccessFailedError,
} from './exceptions';

/**
 * Error structure returned from native module
 */
export interface NativeError {
  code: number;
  message: string;
}

/** Map of error codes to their constructor classes */
type ErrorConstructor = new (details?: string) => StorjError;

const errorConstructors: ReadonlyMap<ErrorCode, ErrorConstructor> = new Map<
  ErrorCode,
  ErrorConstructor
>([
  [ErrorCodes.INTERNAL, InternalError],
  [ErrorCodes.CANCELED, CanceledError],
  [ErrorCodes.INVALID_HANDLE, InvalidHandleError],
  [ErrorCodes.TOO_MANY_REQUESTS, TooManyRequestsError],
  [ErrorCodes.BANDWIDTH_LIMIT_EXCEEDED, BandwidthLimitExceededError],
  [ErrorCodes.STORAGE_LIMIT_EXCEEDED, StorageLimitExceededError],
  [ErrorCodes.SEGMENTS_LIMIT_EXCEEDED, SegmentsLimitExceededError],
  [ErrorCodes.PERMISSION_DENIED, PermissionDeniedError],
  [ErrorCodes.BUCKET_NAME_INVALID, BucketNameInvalidError],
  [ErrorCodes.BUCKET_ALREADY_EXISTS, BucketAlreadyExistsError],
  [ErrorCodes.BUCKET_NOT_EMPTY, BucketNotEmptyError],
  [ErrorCodes.BUCKET_NOT_FOUND, BucketNotFoundError],
  [ErrorCodes.OBJECT_KEY_INVALID, ObjectKeyInvalidError],
  [ErrorCodes.OBJECT_NOT_FOUND, ObjectNotFoundError],
  [ErrorCodes.UPLOAD_DONE, UploadDoneError],
  [ErrorCodes.EDGE_AUTH_DIAL_FAILED, EdgeAuthDialFailedError],
  [ErrorCodes.EDGE_REGISTER_ACCESS_FAILED, EdgeRegisterAccessFailedError],
]);

/**
 * Creates a typed StorjError from an error code and message.
 *
 * @param code - Error code from uplink-c
 * @param details - Error message/details from uplink-c
 * @returns A typed StorjError subclass instance
 *
 * @example
 * ```typescript
 * const error = createStorjError(0x13, 'bucket "test" not found');
 * console.log(error instanceof BucketNotFoundError); // true
 * ```
 */
export function createStorjError(code: number, details?: string): StorjError {
  const ErrorClass = errorConstructors.get(code as ErrorCode);
  if (ErrorClass) {
    return new ErrorClass(details);
  }
  return new InternalError(details ?? `Unknown error code: ${code}`);
}

/**
 * Throws a typed StorjError from an error code and message.
 *
 * @param code - Error code from uplink-c
 * @param details - Error message/details from uplink-c
 * @throws A typed StorjError subclass
 *
 * @example
 * ```typescript
 * try {
 *     throwStorjError(0x21, 'object "file.txt" not found');
 * } catch (error) {
 *     if (error instanceof ObjectNotFoundError) {
 *         // Handle object not found
 *     }
 * }
 * ```
 */
export function throwStorjError(code: number, details?: string): never {
  throw createStorjError(code, details);
}

/**
 * Converts a native error object to a typed StorjError.
 *
 * @param nativeError - Error object from native module
 * @returns A typed StorjError subclass instance
 */
export function fromNativeError(nativeError: NativeError): StorjError {
  return createStorjError(nativeError.code, nativeError.message);
}

/**
 * Type guard to check if an error is a StorjError.
 *
 * @param error - Any error object
 * @returns True if the error is a StorjError
 */
export function isStorjError(error: unknown): error is StorjError {
  return error instanceof StorjError;
}

/**
 * Type guard to check if an error has a specific error code.
 *
 * @param error - Any error object
 * @param code - Error code to check
 * @returns True if the error is a StorjError with the specified code
 */
export function hasErrorCode(error: unknown, code: ErrorCode): boolean {
  return isStorjError(error) && error.code === code;
}
