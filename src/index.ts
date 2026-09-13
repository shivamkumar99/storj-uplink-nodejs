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
import { native } from './native';

/** Package version, read from package.json (dist/ and src/ both sit one level below it). */
// eslint-disable-next-line @typescript-eslint/no-require-imports, @typescript-eslint/no-var-requires
export const VERSION: string = (require('../package.json') as { version: string }).version;

/** Which uplink-c and storj.io/uplink the loaded native addon was built against. */
export interface UplinkCVersionInfo {
  /** The UPLINK_C_VERSION the library was built with (tag or commit SHA). */
  ref: string;
  /** Go module version of storj.io/uplink-c as embedded in libuplink. */
  version: string;
  /** VCS revision Go recorded for libuplink. */
  revision: string;
  /** Version of storj.io/uplink (the Go library uplink-c wraps). */
  storjUplink: string;
}

export function uplinkCVersion(): UplinkCVersionInfo {
  return {
    ref: native.uplinkCRef ?? 'unknown',
    version: native.uplinkCVersion ?? 'unknown',
    revision: native.uplinkCRevision ?? 'unknown',
    storjUplink: native.storjUplinkVersion ?? 'unknown',
  };
}
