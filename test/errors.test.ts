/**
 * @file errors.test.ts
 * @brief Tests for error handling module
 */

import {
    // Error codes
    ErrorCodes,
    
    // Base error
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
    
    // Factory functions
    createStorjError,
    throwStorjError,
    isStorjError,
    hasErrorCode,
} from '../src/errors';

describe('Error Handling Module', () => {
    describe('ErrorCodes', () => {
        it('should have correct error code values', () => {
            expect(ErrorCodes.INTERNAL).toBe(0x02);
            expect(ErrorCodes.CANCELED).toBe(0x03);
            expect(ErrorCodes.INVALID_HANDLE).toBe(0x04);
            expect(ErrorCodes.TOO_MANY_REQUESTS).toBe(0x05);
            expect(ErrorCodes.BANDWIDTH_LIMIT_EXCEEDED).toBe(0x06);
            expect(ErrorCodes.STORAGE_LIMIT_EXCEEDED).toBe(0x07);
            expect(ErrorCodes.SEGMENTS_LIMIT_EXCEEDED).toBe(0x08);
            expect(ErrorCodes.PERMISSION_DENIED).toBe(0x09);
            
            expect(ErrorCodes.BUCKET_NAME_INVALID).toBe(0x10);
            expect(ErrorCodes.BUCKET_ALREADY_EXISTS).toBe(0x11);
            expect(ErrorCodes.BUCKET_NOT_EMPTY).toBe(0x12);
            expect(ErrorCodes.BUCKET_NOT_FOUND).toBe(0x13);
            
            expect(ErrorCodes.OBJECT_KEY_INVALID).toBe(0x20);
            expect(ErrorCodes.OBJECT_NOT_FOUND).toBe(0x21);
            expect(ErrorCodes.UPLOAD_DONE).toBe(0x22);
            
            expect(ErrorCodes.EDGE_AUTH_DIAL_FAILED).toBe(0x30);
            expect(ErrorCodes.EDGE_REGISTER_ACCESS_FAILED).toBe(0x31);
        });
    });

    describe('StorjError base class', () => {
        it('should create error with message and code', () => {
            const error = new StorjError('Test error', ErrorCodes.INTERNAL);
            
            expect(error).toBeInstanceOf(Error);
            expect(error).toBeInstanceOf(StorjError);
            expect(error.message).toBe('Test error');
            expect(error.code).toBe(ErrorCodes.INTERNAL);
            expect(error.details).toBeUndefined();
        });

        it('should include details in message when provided', () => {
            const error = new StorjError('Test error', ErrorCodes.INTERNAL, 'some details');
            
            expect(error.message).toBe('Test error: some details');
            expect(error.details).toBe('some details');
        });
    });

    describe('Specific error classes', () => {
        describe('General errors', () => {
            it('should create InternalError', () => {
                const error = new InternalError('details');
                expect(error).toBeInstanceOf(StorjError);
                expect(error).toBeInstanceOf(InternalError);
                expect(error.code).toBe(ErrorCodes.INTERNAL);
                expect(error.name).toBe('InternalError');
            });

            it('should create CanceledError', () => {
                const error = new CanceledError();
                expect(error).toBeInstanceOf(CanceledError);
                expect(error.code).toBe(ErrorCodes.CANCELED);
            });

            it('should create InvalidHandleError', () => {
                const error = new InvalidHandleError();
                expect(error).toBeInstanceOf(InvalidHandleError);
                expect(error.code).toBe(ErrorCodes.INVALID_HANDLE);
            });

            it('should create TooManyRequestsError', () => {
                const error = new TooManyRequestsError();
                expect(error).toBeInstanceOf(TooManyRequestsError);
                expect(error.code).toBe(ErrorCodes.TOO_MANY_REQUESTS);
            });

            it('should create BandwidthLimitExceededError', () => {
                const error = new BandwidthLimitExceededError();
                expect(error).toBeInstanceOf(BandwidthLimitExceededError);
                expect(error.code).toBe(ErrorCodes.BANDWIDTH_LIMIT_EXCEEDED);
            });

            it('should create StorageLimitExceededError', () => {
                const error = new StorageLimitExceededError();
                expect(error).toBeInstanceOf(StorageLimitExceededError);
                expect(error.code).toBe(ErrorCodes.STORAGE_LIMIT_EXCEEDED);
            });

            it('should create SegmentsLimitExceededError', () => {
                const error = new SegmentsLimitExceededError();
                expect(error).toBeInstanceOf(SegmentsLimitExceededError);
                expect(error.code).toBe(ErrorCodes.SEGMENTS_LIMIT_EXCEEDED);
            });

            it('should create PermissionDeniedError', () => {
                const error = new PermissionDeniedError();
                expect(error).toBeInstanceOf(PermissionDeniedError);
                expect(error.code).toBe(ErrorCodes.PERMISSION_DENIED);
            });
        });

        describe('Bucket errors', () => {
            it('should create BucketNameInvalidError', () => {
                const error = new BucketNameInvalidError('bucket name too short');
                expect(error).toBeInstanceOf(BucketNameInvalidError);
                expect(error.code).toBe(ErrorCodes.BUCKET_NAME_INVALID);
            });

            it('should create BucketAlreadyExistsError', () => {
                const error = new BucketAlreadyExistsError();
                expect(error).toBeInstanceOf(BucketAlreadyExistsError);
                expect(error.code).toBe(ErrorCodes.BUCKET_ALREADY_EXISTS);
            });

            it('should create BucketNotEmptyError', () => {
                const error = new BucketNotEmptyError();
                expect(error).toBeInstanceOf(BucketNotEmptyError);
                expect(error.code).toBe(ErrorCodes.BUCKET_NOT_EMPTY);
            });

            it('should create BucketNotFoundError', () => {
                const error = new BucketNotFoundError('bucket "test" not found');
                expect(error).toBeInstanceOf(BucketNotFoundError);
                expect(error.code).toBe(ErrorCodes.BUCKET_NOT_FOUND);
            });
        });

        describe('Object errors', () => {
            it('should create ObjectKeyInvalidError', () => {
                const error = new ObjectKeyInvalidError();
                expect(error).toBeInstanceOf(ObjectKeyInvalidError);
                expect(error.code).toBe(ErrorCodes.OBJECT_KEY_INVALID);
            });

            it('should create ObjectNotFoundError', () => {
                const error = new ObjectNotFoundError('object "file.txt" not found');
                expect(error).toBeInstanceOf(ObjectNotFoundError);
                expect(error.code).toBe(ErrorCodes.OBJECT_NOT_FOUND);
            });

            it('should create UploadDoneError', () => {
                const error = new UploadDoneError();
                expect(error).toBeInstanceOf(UploadDoneError);
                expect(error.code).toBe(ErrorCodes.UPLOAD_DONE);
            });
        });

        describe('Edge errors', () => {
            it('should create EdgeAuthDialFailedError', () => {
                const error = new EdgeAuthDialFailedError();
                expect(error).toBeInstanceOf(EdgeAuthDialFailedError);
                expect(error.code).toBe(ErrorCodes.EDGE_AUTH_DIAL_FAILED);
            });

            it('should create EdgeRegisterAccessFailedError', () => {
                const error = new EdgeRegisterAccessFailedError();
                expect(error).toBeInstanceOf(EdgeRegisterAccessFailedError);
                expect(error.code).toBe(ErrorCodes.EDGE_REGISTER_ACCESS_FAILED);
            });
        });
    });

    describe('createStorjError factory', () => {
        it('should create correct error type for each code', () => {
            expect(createStorjError(ErrorCodes.INTERNAL)).toBeInstanceOf(InternalError);
            expect(createStorjError(ErrorCodes.CANCELED)).toBeInstanceOf(CanceledError);
            expect(createStorjError(ErrorCodes.INVALID_HANDLE)).toBeInstanceOf(InvalidHandleError);
            expect(createStorjError(ErrorCodes.TOO_MANY_REQUESTS)).toBeInstanceOf(TooManyRequestsError);
            expect(createStorjError(ErrorCodes.BANDWIDTH_LIMIT_EXCEEDED)).toBeInstanceOf(BandwidthLimitExceededError);
            expect(createStorjError(ErrorCodes.STORAGE_LIMIT_EXCEEDED)).toBeInstanceOf(StorageLimitExceededError);
            expect(createStorjError(ErrorCodes.SEGMENTS_LIMIT_EXCEEDED)).toBeInstanceOf(SegmentsLimitExceededError);
            expect(createStorjError(ErrorCodes.PERMISSION_DENIED)).toBeInstanceOf(PermissionDeniedError);
            
            expect(createStorjError(ErrorCodes.BUCKET_NAME_INVALID)).toBeInstanceOf(BucketNameInvalidError);
            expect(createStorjError(ErrorCodes.BUCKET_ALREADY_EXISTS)).toBeInstanceOf(BucketAlreadyExistsError);
            expect(createStorjError(ErrorCodes.BUCKET_NOT_EMPTY)).toBeInstanceOf(BucketNotEmptyError);
            expect(createStorjError(ErrorCodes.BUCKET_NOT_FOUND)).toBeInstanceOf(BucketNotFoundError);
            
            expect(createStorjError(ErrorCodes.OBJECT_KEY_INVALID)).toBeInstanceOf(ObjectKeyInvalidError);
            expect(createStorjError(ErrorCodes.OBJECT_NOT_FOUND)).toBeInstanceOf(ObjectNotFoundError);
            expect(createStorjError(ErrorCodes.UPLOAD_DONE)).toBeInstanceOf(UploadDoneError);
            
            expect(createStorjError(ErrorCodes.EDGE_AUTH_DIAL_FAILED)).toBeInstanceOf(EdgeAuthDialFailedError);
            expect(createStorjError(ErrorCodes.EDGE_REGISTER_ACCESS_FAILED)).toBeInstanceOf(EdgeRegisterAccessFailedError);
        });

        it('should return InternalError for unknown codes', () => {
            const error = createStorjError(0xFF);
            expect(error).toBeInstanceOf(InternalError);
            expect(error.message).toContain('Unknown error code: 255');
        });

        it('should return InternalError with details for unknown codes', () => {
            const error = createStorjError(0xFF, 'unknown error');
            expect(error).toBeInstanceOf(InternalError);
            expect(error.details).toBe('unknown error');
        });

        it('should include details in the error', () => {
            const error = createStorjError(ErrorCodes.BUCKET_NOT_FOUND, 'bucket "test" not found');
            expect(error.details).toBe('bucket "test" not found');
        });
    });

    describe('throwStorjError', () => {
        it('should throw the correct error type', () => {
            expect(() => throwStorjError(ErrorCodes.BUCKET_NOT_FOUND)).toThrow(BucketNotFoundError);
            expect(() => throwStorjError(ErrorCodes.OBJECT_NOT_FOUND)).toThrow(ObjectNotFoundError);
        });
    });

    describe('isStorjError type guard', () => {
        it('should return true for StorjError instances', () => {
            expect(isStorjError(new StorjError('test', ErrorCodes.INTERNAL))).toBe(true);
            expect(isStorjError(new BucketNotFoundError())).toBe(true);
            expect(isStorjError(new ObjectNotFoundError())).toBe(true);
        });

        it('should return false for non-StorjError instances', () => {
            expect(isStorjError(new Error('test'))).toBe(false);
            expect(isStorjError('string')).toBe(false);
            expect(isStorjError(null)).toBe(false);
            expect(isStorjError(undefined)).toBe(false);
            expect(isStorjError({})).toBe(false);
        });
    });

    describe('hasErrorCode', () => {
        it('should return true when error has matching code', () => {
            const error = new BucketNotFoundError();
            expect(hasErrorCode(error, ErrorCodes.BUCKET_NOT_FOUND)).toBe(true);
        });

        it('should return false when error has different code', () => {
            const error = new BucketNotFoundError();
            expect(hasErrorCode(error, ErrorCodes.OBJECT_NOT_FOUND)).toBe(false);
        });

        it('should return false for non-StorjError', () => {
            expect(hasErrorCode(new Error('test'), ErrorCodes.INTERNAL)).toBe(false);
        });
    });

    describe('instanceof usage pattern', () => {
        it('should allow catching specific error types', () => {
            const handleError = (error: unknown): string => {
                if (error instanceof BucketNotFoundError) {
                    return 'bucket_not_found';
                } else if (error instanceof ObjectNotFoundError) {
                    return 'object_not_found';
                } else if (error instanceof StorjError) {
                    return 'storj_error';
                } else {
                    return 'unknown';
                }
            };

            expect(handleError(new BucketNotFoundError())).toBe('bucket_not_found');
            expect(handleError(new ObjectNotFoundError())).toBe('object_not_found');
            expect(handleError(new InternalError())).toBe('storj_error');
            expect(handleError(new Error('test'))).toBe('unknown');
        });
    });
});
