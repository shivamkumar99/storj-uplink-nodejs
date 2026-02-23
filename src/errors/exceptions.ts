/**
 * @file errors/exceptions.ts
 * @brief Typed error classes for Storj uplink operations
 *
 * These error classes are defined entirely in native C code (via embedded JS
 * in error_registry.c) and exported on the native module object.
 * This file re-exports them with proper TypeScript type information.
 *
 * Because the JS engine owns the entire prototype chain (StorjError extends
 * Error, subclasses extend StorjError), `instanceof` works correctly:
 *
 * @example
 * ```typescript
 * try {
 *     await project.statBucket('non-existent');
 * } catch (error) {
 *     if (error instanceof BucketNotFoundError) {
 *         console.log('Bucket does not exist');
 *     } else if (error instanceof StorjError) {
 *         console.log('Storj error:', error.code, error.message);
 *     }
 * }
 * ```
 */

import { errorClasses } from '../native';
import type { ErrorCode } from './codes';

/* ========== Type interfaces ========== */

/**
 * Instance type for the base StorjError class.
 * All Storj errors have a numeric code and optional details string.
 */
export interface IStorjError extends Error {
  readonly code: ErrorCode;
  readonly details?: string;
}

/* ========== Re-export native error constructors with TypeScript types ========== */

/**
 * Base error class for all Storj uplink errors.
 * Defined in native C via embedded JS. All subclasses extend this.
 */
export const StorjError = errorClasses.StorjError as {
  new (message: string, code: ErrorCode, details?: string): IStorjError;
  readonly prototype: IStorjError;
};
/** StorjError instance type for use in type annotations */
export type StorjError = InstanceType<typeof StorjError>;

/* --- General errors --- */

export const InternalError = errorClasses.InternalError as {
  new (details?: string): IStorjError;
  readonly prototype: IStorjError;
};
export type InternalError = InstanceType<typeof InternalError>;

export const CanceledError = errorClasses.CanceledError as {
  new (details?: string): IStorjError;
  readonly prototype: IStorjError;
};
export type CanceledError = InstanceType<typeof CanceledError>;

export const InvalidHandleError = errorClasses.InvalidHandleError as {
  new (details?: string): IStorjError;
  readonly prototype: IStorjError;
};
export type InvalidHandleError = InstanceType<typeof InvalidHandleError>;

export const TooManyRequestsError = errorClasses.TooManyRequestsError as {
  new (details?: string): IStorjError;
  readonly prototype: IStorjError;
};
export type TooManyRequestsError = InstanceType<typeof TooManyRequestsError>;

export const BandwidthLimitExceededError = errorClasses.BandwidthLimitExceededError as {
  new (details?: string): IStorjError;
  readonly prototype: IStorjError;
};
export type BandwidthLimitExceededError = InstanceType<typeof BandwidthLimitExceededError>;

export const StorageLimitExceededError = errorClasses.StorageLimitExceededError as {
  new (details?: string): IStorjError;
  readonly prototype: IStorjError;
};
export type StorageLimitExceededError = InstanceType<typeof StorageLimitExceededError>;

export const SegmentsLimitExceededError = errorClasses.SegmentsLimitExceededError as {
  new (details?: string): IStorjError;
  readonly prototype: IStorjError;
};
export type SegmentsLimitExceededError = InstanceType<typeof SegmentsLimitExceededError>;

export const PermissionDeniedError = errorClasses.PermissionDeniedError as {
  new (details?: string): IStorjError;
  readonly prototype: IStorjError;
};
export type PermissionDeniedError = InstanceType<typeof PermissionDeniedError>;

/* --- Bucket errors --- */

export const BucketNameInvalidError = errorClasses.BucketNameInvalidError as {
  new (details?: string): IStorjError;
  readonly prototype: IStorjError;
};
export type BucketNameInvalidError = InstanceType<typeof BucketNameInvalidError>;

export const BucketAlreadyExistsError = errorClasses.BucketAlreadyExistsError as {
  new (details?: string): IStorjError;
  readonly prototype: IStorjError;
};
export type BucketAlreadyExistsError = InstanceType<typeof BucketAlreadyExistsError>;

export const BucketNotEmptyError = errorClasses.BucketNotEmptyError as {
  new (details?: string): IStorjError;
  readonly prototype: IStorjError;
};
export type BucketNotEmptyError = InstanceType<typeof BucketNotEmptyError>;

export const BucketNotFoundError = errorClasses.BucketNotFoundError as {
  new (details?: string): IStorjError;
  readonly prototype: IStorjError;
};
export type BucketNotFoundError = InstanceType<typeof BucketNotFoundError>;

/* --- Object errors --- */

export const ObjectKeyInvalidError = errorClasses.ObjectKeyInvalidError as {
  new (details?: string): IStorjError;
  readonly prototype: IStorjError;
};
export type ObjectKeyInvalidError = InstanceType<typeof ObjectKeyInvalidError>;

export const ObjectNotFoundError = errorClasses.ObjectNotFoundError as {
  new (details?: string): IStorjError;
  readonly prototype: IStorjError;
};
export type ObjectNotFoundError = InstanceType<typeof ObjectNotFoundError>;

export const UploadDoneError = errorClasses.UploadDoneError as {
  new (details?: string): IStorjError;
  readonly prototype: IStorjError;
};
export type UploadDoneError = InstanceType<typeof UploadDoneError>;

/* --- Edge errors --- */

export const EdgeAuthDialFailedError = errorClasses.EdgeAuthDialFailedError as {
  new (details?: string): IStorjError;
  readonly prototype: IStorjError;
};
export type EdgeAuthDialFailedError = InstanceType<typeof EdgeAuthDialFailedError>;

export const EdgeRegisterAccessFailedError = errorClasses.EdgeRegisterAccessFailedError as {
  new (details?: string): IStorjError;
  readonly prototype: IStorjError;
};
export type EdgeRegisterAccessFailedError = InstanceType<typeof EdgeRegisterAccessFailedError>;
