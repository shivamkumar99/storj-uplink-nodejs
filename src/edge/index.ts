/**
 * @file edge/index.ts
 * @brief Edge/linkshare operations for uplink-nodejs
 *
 * Provides TypeScript wrappers for edge credential and share URL operations.
 */

import type {
  EdgeConfig,
  EdgeRegisterAccessOptions,
  EdgeCredentials,
  EdgeShareURLOptions,
} from '../types';
import { native } from '../native';

/** Native handle type */
type AccessHandle = unknown;

/**
 * Register access with Storj edge services to get S3-compatible credentials.
 *
 * This allows you to access Storj using S3-compatible tools and libraries.
 *
 * @param config - Edge configuration (auth service address)
 * @param accessHandle - Native access handle
 * @param options - Optional registration options (isPublic for linkshare)
 * @returns Promise resolving to S3-compatible credentials
 *
 * @example
 * ```typescript
 * const config: EdgeConfig = {
 *   authServiceAddress: 'auth.us1.storjshare.io:7777'
 * };
 *
 * const credentials = await edgeRegisterAccess(
 *   config,
 *   access._nativeHandle,
 *   { isPublic: true }
 * );
 *
 * console.log('Access Key:', credentials.accessKeyId);
 * console.log('Secret Key:', credentials.secretKey);
 * console.log('Endpoint:', credentials.endpoint);
 * ```
 */
export async function edgeRegisterAccess(
  config: EdgeConfig,
  accessHandle: AccessHandle,
  options?: EdgeRegisterAccessOptions
): Promise<EdgeCredentials> {
  if (config == null || typeof config !== 'object') {
    throw new TypeError('config must be an object');
  }

  if (!config.authServiceAddress || typeof config.authServiceAddress !== 'string') {
    throw new TypeError('config.authServiceAddress is required');
  }

  if (accessHandle == null) {
    throw new TypeError('accessHandle is required');
  }

  return native.edgeRegisterAccess(config, accessHandle, options) as Promise<EdgeCredentials>;
}

/**
 * Create a shareable linkshare URL for an object.
 *
 * The accessKeyId must be from an access registered with `isPublic: true`.
 *
 * @param baseUrl - Linkshare service URL (e.g., https://link.us1.storjshare.io)
 * @param accessKeyId - Access key ID from edgeRegisterAccess (must be public)
 * @param bucket - Bucket name (empty string to share entire project)
 * @param key - Object key or prefix (empty string to share entire bucket)
 * @param options - Optional share URL options
 * @returns Promise resolving to the share URL
 *
 * @example
 * ```typescript
 * // Share a specific object
 * const url = await edgeJoinShareUrl(
 *   'https://link.us1.storjshare.io',
 *   credentials.accessKeyId,
 *   'my-bucket',
 *   'photos/vacation.jpg',
 *   { raw: true } // Serve file directly
 * );
 * console.log('Share URL:', url);
 *
 * // Share an entire bucket
 * const bucketUrl = await edgeJoinShareUrl(
 *   'https://link.us1.storjshare.io',
 *   credentials.accessKeyId,
 *   'my-bucket',
 *   ''
 * );
 *
 * // Share with landing page (raw: false)
 * const landingUrl = await edgeJoinShareUrl(
 *   'https://link.us1.storjshare.io',
 *   credentials.accessKeyId,
 *   'my-bucket',
 *   'document.pdf'
 * );
 * ```
 */
export async function edgeJoinShareUrl(
  baseUrl: string,
  accessKeyId: string,
  bucket: string,
  key: string,
  options?: EdgeShareURLOptions
): Promise<string> {
  if (!baseUrl || typeof baseUrl !== 'string') {
    throw new TypeError('baseUrl must be a non-empty string');
  }

  if (!accessKeyId || typeof accessKeyId !== 'string') {
    throw new TypeError('accessKeyId must be a non-empty string');
  }

  if (typeof bucket !== 'string') {
    throw new TypeError('bucket must be a string');
  }

  if (typeof key !== 'string') {
    throw new TypeError('key must be a string');
  }

  return native.edgeJoinShareUrl(baseUrl, accessKeyId, bucket, key, options);
}

/**
 * Edge service regions
 */
export const EdgeRegions = {
  /** US East region */
  US1: {
    authService: 'auth.us1.storjshare.io:7777',
    linkshare: 'https://link.us1.storjshare.io',
  },
  /** EU region */
  EU1: {
    authService: 'auth.eu1.storjshare.io:7777',
    linkshare: 'https://link.eu1.storjshare.io',
  },
  /** Asia Pacific region */
  AP1: {
    authService: 'auth.ap1.storjshare.io:7777',
    linkshare: 'https://link.ap1.storjshare.io',
  },
} as const;
