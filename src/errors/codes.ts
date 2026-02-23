/**
 * @file errors/codes.ts
 * @brief Error code constants matching uplink-c/uplink_definitions.h
 */

/**
 * Error codes matching uplink-c definitions
 * @see uplink_definitions.h
 */
export const ErrorCodes = {
  // General errors
  INTERNAL: 0x02,
  CANCELED: 0x03,
  INVALID_HANDLE: 0x04,
  TOO_MANY_REQUESTS: 0x05,
  BANDWIDTH_LIMIT_EXCEEDED: 0x06,
  STORAGE_LIMIT_EXCEEDED: 0x07,
  SEGMENTS_LIMIT_EXCEEDED: 0x08,
  PERMISSION_DENIED: 0x09,

  // Bucket errors
  BUCKET_NAME_INVALID: 0x10,
  BUCKET_ALREADY_EXISTS: 0x11,
  BUCKET_NOT_EMPTY: 0x12,
  BUCKET_NOT_FOUND: 0x13,

  // Object errors
  OBJECT_KEY_INVALID: 0x20,
  OBJECT_NOT_FOUND: 0x21,
  UPLOAD_DONE: 0x22,

  // Edge errors
  EDGE_AUTH_DIAL_FAILED: 0x30,
  EDGE_REGISTER_ACCESS_FAILED: 0x31,
} as const;

export type ErrorCode = (typeof ErrorCodes)[keyof typeof ErrorCodes];
