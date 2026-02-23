/**
 * Type definitions for Storj Uplink Node.js bindings
 *
 * @packageDocumentation
 */

/**
 * Configuration options for the Uplink client
 */
export interface UplinkConfig {
  /** User agent string to send with requests */
  userAgent?: string;
  /** Dial timeout in milliseconds */
  dialTimeoutMilliseconds?: number;
  /** Temporary directory for file operations */
  tempDirectory?: string;
}

/**
 * Permission settings for access grants
 */
export interface Permission {
  /** Allow downloading objects */
  allowDownload?: boolean;
  /** Allow uploading objects */
  allowUpload?: boolean;
  /** Allow listing buckets and objects */
  allowList?: boolean;
  /** Allow deleting objects */
  allowDelete?: boolean;
  /** Grant becomes valid at this time */
  notBefore?: Date;
  /** Grant expires at this time */
  notAfter?: Date;
}

/**
 * A prefix to share within a bucket
 */
export interface SharePrefix {
  /** Name of the bucket */
  bucket: string;
  /** Object key prefix (optional) */
  prefix?: string;
}

/**
 * Bucket information returned from bucket operations
 */
export interface BucketInfo {
  /** Name of the bucket */
  name: string;
  /** When the bucket was created (Unix timestamp in seconds) */
  created: number;
}

/**
 * Object information returned from object operations
 */
export interface ObjectInfo {
  /** Object key (path) */
  key: string;
  /** Whether this is a prefix (folder) */
  isPrefix: boolean;
  /** System metadata */
  system: SystemMetadata;
  /** Custom metadata key-value pairs */
  custom: CustomMetadata;
}

/**
 * System-managed metadata for objects
 */
export interface SystemMetadata {
  /** When the object was created (Unix timestamp in seconds) */
  created: number;
  /** When the object expires (Unix timestamp in seconds, null if no expiration) */
  expires: number | null;
  /** Size of the object in bytes */
  contentLength: number;
}

/**
 * Custom user-defined metadata
 */
export interface CustomMetadata {
  /** Custom key-value pairs */
  [key: string]: string;
}

/**
 * Options for listing buckets
 */
export interface ListBucketsOptions {
  /** Cursor for pagination */
  cursor?: string;
}

/**
 * Options for listing objects
 */
export interface ListObjectsOptions {
  /** Object key prefix filter */
  prefix?: string;
  /** Cursor for pagination */
  cursor?: string;
  /** Include system metadata in results */
  system?: boolean;
  /** Include custom metadata in results */
  custom?: boolean;
  /** Treat '/' as a directory separator */
  recursive?: boolean;
}

/**
 * Options for uploading objects
 */
export interface UploadOptions {
  /** When the object should expire */
  expires?: Date;
}

/**
 * Options for downloading objects
 */
export interface DownloadOptions {
  /** Starting byte offset */
  offset?: number;
  /** Number of bytes to download (-1 for all) */
  length?: number;
}

/**
 * Result from a write operation
 */
export interface WriteResult {
  /** Number of bytes written */
  bytesWritten: number;
}

/**
 * Result from a read operation
 */
export interface ReadResult {
  /** Number of bytes read */
  bytesRead: number;
}

/**
 * Options for copying objects
 */
export interface CopyObjectOptions {
  /** When the copied object should expire (optional) */
  expires?: Date;
}

/**
 * Options for moving objects
 */
export interface MoveObjectOptions {
  /** When the moved object should expire (optional) */
  expires?: Date;
}

/**
 * Uplink error codes from the native library
 */
export enum UplinkErrorCode {
  /** Internal error */
  Internal = 0x02,
  /** Operation was canceled */
  Canceled = 0x03,
  /** Invalid handle provided */
  InvalidHandle = 0x04,
  /** Too many concurrent requests */
  TooManyRequests = 0x05,
  /** Bandwidth limit exceeded */
  BandwidthLimitExceeded = 0x06,
  /** Storage limit exceeded */
  StorageLimitExceeded = 0x07,
  /** Segments limit exceeded */
  SegmentsLimitExceeded = 0x08,
  /** Permission denied */
  PermissionDenied = 0x09,
  /** Invalid bucket name */
  BucketNameInvalid = 0x10,
  /** Bucket already exists */
  BucketAlreadyExists = 0x11,
  /** Cannot delete non-empty bucket */
  BucketNotEmpty = 0x12,
  /** Bucket not found */
  BucketNotFound = 0x13,
  /** Invalid object key */
  ObjectKeyInvalid = 0x20,
  /** Object not found */
  ObjectNotFound = 0x21,
  /** Upload already completed */
  UploadDone = 0x22,
}

/**
 * Error thrown by uplink operations
 */
export class UplinkError extends Error {
  /** Error code from native library */
  readonly code: UplinkErrorCode;

  constructor(message: string, code: UplinkErrorCode) {
    super(message);
    this.name = 'UplinkError';
    this.code = code;
    Object.setPrototypeOf(this, UplinkError.prototype);
  }
}

/**
 * Encryption key for overriding access encryption
 */
export interface EncryptionKey {
  /** Internal key representation (opaque) */
  _handle: number;
}

// ========== Multipart Upload Types ==========

/**
 * Information about a pending multipart upload
 */
export interface UploadInfo {
  /** Unique identifier for the multipart upload */
  uploadId: string;
  /** Object key being uploaded */
  key: string;
  /** Whether this is a prefix */
  isPrefix: boolean;
  /** System metadata */
  system: SystemMetadata;
  /** Custom metadata */
  custom: CustomMetadata;
}

/**
 * Information about an uploaded part
 */
export interface PartInfo {
  /** Part number (1-based) */
  partNumber: number;
  /** Size of the part in bytes */
  size: number;
  /** When the part was modified */
  modified: Date;
  /** ETag for the part */
  etag: string;
}

/**
 * Options for beginning a multipart upload
 */
export interface BeginUploadOptions {
  /** When the object should expire (Unix timestamp) */
  expires?: number;
}

/**
 * Options for committing a multipart upload
 */
export interface CommitUploadOptions {
  /** Custom metadata to attach to the object */
  customMetadata?: CustomMetadata;
}

/**
 * Options for listing uploaded parts
 */
export interface ListUploadPartsOptions {
  /** Cursor for pagination (part number to start after) */
  cursor?: number;
}

/**
 * Options for listing pending uploads
 */
export interface ListUploadsOptions {
  /** Object key prefix filter */
  prefix?: string;
  /** Cursor for pagination */
  cursor?: string;
  /** Include system metadata */
  system?: boolean;
  /** Include custom metadata */
  custom?: boolean;
  /** List recursively */
  recursive?: boolean;
}

// ========== Edge/Linkshare Types ==========

/**
 * Configuration for edge services
 */
export interface EdgeConfig {
  /** Auth service address (e.g., auth.us1.storjshare.io:7777) */
  authServiceAddress: string;
  /** Certificate PEM for TLS (optional) */
  certificatePem?: string;
  /** Use unencrypted connection (not recommended) */
  insecureUnencryptedConnection?: boolean;
}

/**
 * Options for registering access with edge services
 */
export interface EdgeRegisterAccessOptions {
  /** Whether the access should be publicly accessible */
  isPublic?: boolean;
}

/**
 * S3-compatible credentials from edge services
 */
export interface EdgeCredentials {
  /** Access key ID (used in S3 auth and linkshare URLs) */
  accessKeyId: string;
  /** Secret key for S3 authentication */
  secretKey: string;
  /** S3-compatible endpoint URL */
  endpoint: string;
}

/**
 * Options for creating share URLs
 */
export interface EdgeShareURLOptions {
  /** Serve file directly without landing page */
  raw?: boolean;
}
