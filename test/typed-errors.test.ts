/**
 * @file test/typed-errors.test.ts
 * @brief Tests for typed error creation from native code
 *
 * Verifies that errors thrown from native async operations are proper
 * instances of StorjError subclasses, enabling `instanceof` checks.
 *
 * These error classes are defined entirely in native C (via embedded JS)
 * and exported on the native module — no TypeScript constructors are
 * passed to native.
 */

import { native } from '../src/native';
import {
  StorjError,
  ErrorCodes,
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
} from '../src/errors';

describe('Typed Errors from Native Code', () => {
  /**
   * Helper to call native testThrowTypedError and capture the rejection
   */
  async function getTypedError(code: number, message: string): Promise<unknown> {
    try {
      await native.testThrowTypedError(code, message);
      throw new Error('Expected promise to reject but it resolved');
    } catch (error: unknown) {
      return error;
    }
  }

  describe('instanceof checks on native errors', () => {
    it('should create BucketNotFoundError instanceof StorjError', async () => {
      const error = await getTypedError(ErrorCodes.BUCKET_NOT_FOUND, 'bucket "test" not found');

      expect(error).toBeInstanceOf(StorjError);
      expect(error).toBeInstanceOf(BucketNotFoundError);
      expect(error).toBeInstanceOf(Error);
    });

    it('should create ObjectNotFoundError instanceof StorjError', async () => {
      const error = await getTypedError(ErrorCodes.OBJECT_NOT_FOUND, 'object "file.txt" not found');

      expect(error).toBeInstanceOf(StorjError);
      expect(error).toBeInstanceOf(ObjectNotFoundError);
      expect(error).toBeInstanceOf(Error);
    });

    it('should create InternalError instanceof StorjError', async () => {
      const error = await getTypedError(ErrorCodes.INTERNAL, 'internal failure');

      expect(error).toBeInstanceOf(StorjError);
      expect(error).toBeInstanceOf(InternalError);
    });

    it('should create PermissionDeniedError instanceof StorjError', async () => {
      const error = await getTypedError(ErrorCodes.PERMISSION_DENIED, 'access denied');

      expect(error).toBeInstanceOf(StorjError);
      expect(error).toBeInstanceOf(PermissionDeniedError);
    });

    it('should create BucketNameInvalidError instanceof StorjError', async () => {
      const error = await getTypedError(ErrorCodes.BUCKET_NAME_INVALID, 'bad name');

      expect(error).toBeInstanceOf(StorjError);
      expect(error).toBeInstanceOf(BucketNameInvalidError);
    });

    it('should create BucketAlreadyExistsError instanceof StorjError', async () => {
      const error = await getTypedError(ErrorCodes.BUCKET_ALREADY_EXISTS, 'already exists');

      expect(error).toBeInstanceOf(StorjError);
      expect(error).toBeInstanceOf(BucketAlreadyExistsError);
    });

    it('should create BucketNotEmptyError instanceof StorjError', async () => {
      const error = await getTypedError(ErrorCodes.BUCKET_NOT_EMPTY, 'not empty');

      expect(error).toBeInstanceOf(StorjError);
      expect(error).toBeInstanceOf(BucketNotEmptyError);
    });

    it('should create ObjectKeyInvalidError instanceof StorjError', async () => {
      const error = await getTypedError(ErrorCodes.OBJECT_KEY_INVALID, 'bad key');

      expect(error).toBeInstanceOf(StorjError);
      expect(error).toBeInstanceOf(ObjectKeyInvalidError);
    });

    it('should create UploadDoneError instanceof StorjError', async () => {
      const error = await getTypedError(ErrorCodes.UPLOAD_DONE, 'upload done');

      expect(error).toBeInstanceOf(StorjError);
      expect(error).toBeInstanceOf(UploadDoneError);
    });

    it('should create CanceledError instanceof StorjError', async () => {
      const error = await getTypedError(ErrorCodes.CANCELED, 'canceled');

      expect(error).toBeInstanceOf(StorjError);
      expect(error).toBeInstanceOf(CanceledError);
    });

    it('should create InvalidHandleError instanceof StorjError', async () => {
      const error = await getTypedError(ErrorCodes.INVALID_HANDLE, 'bad handle');

      expect(error).toBeInstanceOf(StorjError);
      expect(error).toBeInstanceOf(InvalidHandleError);
    });

    it('should create TooManyRequestsError instanceof StorjError', async () => {
      const error = await getTypedError(ErrorCodes.TOO_MANY_REQUESTS, 'rate limited');

      expect(error).toBeInstanceOf(StorjError);
      expect(error).toBeInstanceOf(TooManyRequestsError);
    });

    it('should create BandwidthLimitExceededError instanceof StorjError', async () => {
      const error = await getTypedError(ErrorCodes.BANDWIDTH_LIMIT_EXCEEDED, 'bw exceeded');

      expect(error).toBeInstanceOf(StorjError);
      expect(error).toBeInstanceOf(BandwidthLimitExceededError);
    });

    it('should create StorageLimitExceededError instanceof StorjError', async () => {
      const error = await getTypedError(ErrorCodes.STORAGE_LIMIT_EXCEEDED, 'storage exceeded');

      expect(error).toBeInstanceOf(StorjError);
      expect(error).toBeInstanceOf(StorageLimitExceededError);
    });

    it('should create SegmentsLimitExceededError instanceof StorjError', async () => {
      const error = await getTypedError(ErrorCodes.SEGMENTS_LIMIT_EXCEEDED, 'segments exceeded');

      expect(error).toBeInstanceOf(StorjError);
      expect(error).toBeInstanceOf(SegmentsLimitExceededError);
    });
  });

  describe('error properties from native', () => {
    it('should preserve error code', async () => {
      const error = await getTypedError(ErrorCodes.BUCKET_NOT_FOUND, 'test message');

      expect(error).toBeInstanceOf(StorjError);
      const storjError = error as StorjError;
      expect(storjError.code).toBe(ErrorCodes.BUCKET_NOT_FOUND);
    });

    it('should preserve error details/message', async () => {
      const error = await getTypedError(ErrorCodes.OBJECT_NOT_FOUND, 'object "myfile.txt" not found');

      expect(error).toBeInstanceOf(StorjError);
      const storjError = error as StorjError;
      expect(storjError.details).toBe('object "myfile.txt" not found');
    });

    it('should have proper error name', async () => {
      const error = await getTypedError(ErrorCodes.BUCKET_NOT_FOUND, 'test');

      expect(error).toBeInstanceOf(StorjError);
      const storjError = error as StorjError;
      expect(storjError.name).toBe('BucketNotFoundError');
    });
  });

  describe('instanceof differentiation', () => {
    it('BucketNotFoundError should NOT be instanceof ObjectNotFoundError', async () => {
      const error = await getTypedError(ErrorCodes.BUCKET_NOT_FOUND, 'test');

      expect(error).toBeInstanceOf(BucketNotFoundError);
      expect(error).not.toBeInstanceOf(ObjectNotFoundError);
      expect(error).not.toBeInstanceOf(InternalError);
      expect(error).not.toBeInstanceOf(PermissionDeniedError);
    });

    it('ObjectNotFoundError should NOT be instanceof BucketNotFoundError', async () => {
      const error = await getTypedError(ErrorCodes.OBJECT_NOT_FOUND, 'test');

      expect(error).toBeInstanceOf(ObjectNotFoundError);
      expect(error).not.toBeInstanceOf(BucketNotFoundError);
      expect(error).not.toBeInstanceOf(InternalError);
    });

    it('all specific errors should be instanceof StorjError and Error', async () => {
      const codes = [
        ErrorCodes.INTERNAL,
        ErrorCodes.CANCELED,
        ErrorCodes.BUCKET_NOT_FOUND,
        ErrorCodes.OBJECT_NOT_FOUND,
        ErrorCodes.PERMISSION_DENIED,
      ];

      for (const code of codes) {
        const error = await getTypedError(code, 'test');
        expect(error).toBeInstanceOf(StorjError);
        expect(error).toBeInstanceOf(Error);
      }
    });
  });

  describe('switch/case pattern with instanceof', () => {
    it('should support try/catch with instanceof pattern', async () => {
      let caughtType = 'unknown';

      try {
        await native.testThrowTypedError(ErrorCodes.BUCKET_NOT_FOUND, 'bucket "mybucket" not found');
      } catch (error: unknown) {
        if (error instanceof BucketNotFoundError) {
          caughtType = 'BucketNotFoundError';
        } else if (error instanceof ObjectNotFoundError) {
          caughtType = 'ObjectNotFoundError';
        } else if (error instanceof StorjError) {
          caughtType = 'StorjError';
        } else {
          caughtType = 'unknown';
        }
      }

      expect(caughtType).toBe('BucketNotFoundError');
    });

    it('should fall through to StorjError for unmatched subtypes', async () => {
      let caughtType = 'unknown';

      try {
        await native.testThrowTypedError(ErrorCodes.INTERNAL, 'some internal error');
      } catch (error: unknown) {
        if (error instanceof BucketNotFoundError) {
          caughtType = 'BucketNotFoundError';
        } else if (error instanceof StorjError) {
          caughtType = 'StorjError';
        } else {
          caughtType = 'unknown';
        }
      }

      expect(caughtType).toBe('StorjError');
    });
  });
});
