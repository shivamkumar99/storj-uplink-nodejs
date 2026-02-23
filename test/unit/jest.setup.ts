
/* eslint-disable @typescript-eslint/no-explicit-any */
/**
 * @file test/unit/jest.setup.ts
 * @brief Unit test setup — mocks the native module so unit tests
 *        can run without building the C addon or having libuplink installed.
 */


// Mock the native module before any src/ imports resolve it.
// This replaces src/native/index.ts with stub functions so that
// class-structure and input-validation tests work without the real binary.
jest.mock('../../src/native', () => {

  // Helper: Synchronous type guard throw for certain functions
  function throwTypeErrorSync(msg = 'Invalid argument type'): never {
    throw new TypeError(msg);
  }

  // Helper: Async type guard rejection for others
  function rejectTypeErrorAsync(msg = 'Invalid argument type'): Promise<never> {
    return Promise.reject(new TypeError(msg));
  }

  // Map of function names to their mock implementations
  const native: { [key: string]: any } = {
    // Synchronous type-guarded functions (throw synchronously)
    parseAccess: throwTypeErrorSync,
    accessSerialize: throwTypeErrorSync,
    uploadWrite: throwTypeErrorSync,
    // Add more if needed for sync throws

    // Async functions (reject with TypeError)
    downloadRead: rejectTypeErrorAsync,
    accessShare: rejectTypeErrorAsync,
    uploadSetCustomMetadata: rejectTypeErrorAsync,
    updateObjectMetadata: rejectTypeErrorAsync,
    uploadPart: rejectTypeErrorAsync,
    partUploadWrite: rejectTypeErrorAsync,
    // Fallback for all others
    // (for brevity, you can add more as needed)
  };

  // Fill in all other expected native methods with a generic async rejection
  const stub = (): Promise<unknown> => Promise.reject(new Error('native function not available in unit tests'));

  // List of all expected native method names (from NativeModule interface)
  const allNativeMethods = [
    'parseAccess',
    'requestAccessWithPassphrase',
    'configRequestAccessWithPassphrase',
    'accessSatelliteAddress',
    'accessSerialize',
    'accessShare',
    'accessOverrideEncryptionKey',
    'openProject',
    'configOpenProject',
    'closeProject',
    'revokeAccess',
    'createBucket',
    'ensureBucket',
    'statBucket',
    'deleteBucket',
    'deleteBucketWithObjects',
    'listBucketsCreate',
    'bucketIteratorNext',
    'bucketIteratorItem',
    'bucketIteratorErr',
    'freeBucketIterator',
    'statObject',
    'deleteObject',
    'listObjectsCreate',
    'objectIteratorNext',
    'objectIteratorItem',
    'objectIteratorErr',
    'freeObjectIterator',
    'copyObject',
    'moveObject',
    'updateObjectMetadata',
    'uploadObject',
    'uploadWrite',
    'uploadCommit',
    'uploadAbort',
    'uploadSetCustomMetadata',
    'uploadInfo',
    'downloadObject',
    'downloadRead',
    'downloadInfo',
    'closeDownload',
    'deriveEncryptionKey',
    'beginUpload',
    'commitUpload',
    'abortUpload',
    'uploadPart',
    'partUploadWrite',
    'partUploadCommit',
    'partUploadAbort',
    'partUploadSetEtag',
    'partUploadInfo',
    'listUploadPartsCreate',
    'partIteratorNext',
    'partIteratorItem',
    'partIteratorErr',
    'freePartIterator',
    'listUploadsCreate',
    'uploadIteratorNext',
    'uploadIteratorItem',
    'uploadIteratorErr',
    'freeUploadIterator',
    'edgeRegisterAccess',
    'edgeJoinShareUrl',
    'internalUniverseIsEmpty',
    'testThrowTypedError',
    'initErrorClasses',
  ];
  for (const fn of allNativeMethods) {
    if (!(fn in native)) native[fn] = stub;
  }


  // Use real error classes from src/errors for correct instanceof checks
  // eslint-disable-next-line @typescript-eslint/no-var-requires
  const errors = require('../../src/errors');
  const mockErrorClasses = {
    StorjError: errors.StorjError,
    InternalError: errors.InternalError,
    CanceledError: errors.CanceledError,
    InvalidHandleError: errors.InvalidHandleError,
    TooManyRequestsError: errors.TooManyRequestsError,
    BandwidthLimitExceededError: errors.BandwidthLimitExceededError,
    StorageLimitExceededError: errors.StorageLimitExceededError,
    SegmentsLimitExceededError: errors.SegmentsLimitExceededError,
    PermissionDeniedError: errors.PermissionDeniedError,
    BucketNameInvalidError: errors.BucketNameInvalidError,
    BucketAlreadyExistsError: errors.BucketAlreadyExistsError,
    BucketNotEmptyError: errors.BucketNotEmptyError,
    BucketNotFoundError: errors.BucketNotFoundError,
    ObjectKeyInvalidError: errors.ObjectKeyInvalidError,
    ObjectNotFoundError: errors.ObjectNotFoundError,
    UploadDoneError: errors.UploadDoneError,
    EdgeAuthDialFailedError: errors.EdgeAuthDialFailedError,
    EdgeRegisterAccessFailedError: errors.EdgeRegisterAccessFailedError,
  };

  return {
    __esModule: true,
    native,
    errorClasses: mockErrorClasses,
    default: {},
  };
});
