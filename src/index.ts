/**
 * Storj Uplink Node.js Bindings
 *
 * Native bindings for Storj's uplink-c library, providing
 * access to Storj's decentralized cloud storage network.
 *
 * @packageDocumentation
 */

// Export types
export * from './types';

// Export error classes and utilities
export * from './errors';

// Export main classes
export { Uplink } from './uplink';
export { AccessResultStruct } from './access';
export { ProjectResultStruct } from './project';
export { UploadResultStruct } from './upload';
export { DownloadResultStruct } from './download';

// Export multipart upload classes and functions
export {
  MultipartUpload,
  PartUploadResultStruct,
  beginMultipartUpload,
  listMultipartUploads,
} from './multipart';

// Export edge/linkshare functions
export { edgeRegisterAccess, edgeJoinShareUrl, EdgeRegions } from './edge';

// Export debug utilities
export {
  internalUniverseIsEmpty,
  uplinkInternalUniverseIsEmpty,
  testThrowTypedError,
} from './debug';

// Export centralized native module for internal use
export { native as _native, NativeModule } from './native';

/** Package version */
export const VERSION = '0.1.0';
