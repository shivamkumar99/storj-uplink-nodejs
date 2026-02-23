/**
 * @file access/index.ts
 * @brief AccessResultStruct class for uplink-nodejs
 *
 * Provides TypeScript wrapper for access operations.
 */

import type { Permission, SharePrefix, UplinkConfig, EncryptionKey } from '../types';
import { ProjectResultStruct } from '../project';
import { native } from '../native';

/** Native handle type */
type AccessHandle = unknown;

/** Native permission format */
interface NativePermission {
  allowDownload: boolean;
  allowUpload: boolean;
  allowList: boolean;
  allowDelete: boolean;
  notBefore?: number;
  notAfter?: number;
}

/** Native share prefix format */
interface NativeSharePrefix {
  bucket: string;
  prefix?: string;
}

/**
 * Represents an access grant to Storj.
 *
 * An access grant contains all information needed to connect to a
 * Storj project and access its data.
 */
export class AccessResultStruct {
  private readonly _handle: AccessHandle;
  private _closed: boolean = false;

  /**
   * Create a new AccessResultStruct from a native handle
   * @internal
   */
  constructor(handle: AccessHandle) {
    if (handle == null) {
      throw new TypeError('Invalid access handle');
    }
    this._handle = handle;
  }

  /**
   * Get the internal handle (for internal use only)
   * @internal
   */
  get _nativeHandle(): AccessHandle {
    this.validateNotClosed();
    return this._handle;
  }

  /**
   * Get the satellite address for this access grant.
   * @returns Promise resolving to the satellite address URL
   */
  async satelliteAddress(): Promise<string> {
    this.validateNotClosed();
    return native.accessSatelliteAddress(this._handle);
  }

  /**
   * Serialize the access grant to a string.
   *
   * The returned string can be used with `Uplink.parseAccess()` to
   * recreate this access grant.
   *
   * @returns Promise resolving to the serialized access grant string
   */
  async serialize(): Promise<string> {
    this.validateNotClosed();
    return native.accessSerialize(this._handle);
  }

  /**
   * Create a new access grant with restricted permissions.
   *
   * @param permission - The permissions to grant
   * @param prefixes - The bucket/prefix combinations to restrict to
   * @returns Promise resolving to a new restricted AccessResultStruct
   *
   * @example
   * ```typescript
   * const restrictedAccess = await access.share(
   *   { allowDownload: true, allowList: true },
   *   [{ bucket: 'my-bucket', prefix: 'public/' }]
   * );
   * ```
   */
  async share(permission: Permission, prefixes: SharePrefix[]): Promise<AccessResultStruct> {
    this.validateNotClosed();

    // Convert permission to native format
    const nativePermission: NativePermission = {
      allowDownload: permission.allowDownload ?? false,
      allowUpload: permission.allowUpload ?? false,
      allowList: permission.allowList ?? false,
      allowDelete: permission.allowDelete ?? false,
      notBefore: permission.notBefore
        ? Math.floor(permission.notBefore.getTime() / 1000)
        : undefined,
      notAfter: permission.notAfter ? Math.floor(permission.notAfter.getTime() / 1000) : undefined,
    };

    // Convert prefixes to native format
    const nativePrefixes: NativeSharePrefix[] = prefixes.map((p) => ({
      bucket: p.bucket,
      prefix: p.prefix,
    }));

    const newHandle = await native.accessShare(this._handle, nativePermission, nativePrefixes);
    return new AccessResultStruct(newHandle);
  }

  /**
   * Override the encryption key for a specific bucket/prefix.
   *
   * This is useful for implementing multitenancy where different
   * users have different encryption keys for the same bucket.
   *
   * @param bucket - The bucket name
   * @param prefix - The object key prefix
   * @param encryptionKey - The new encryption key
   */
  async overrideEncryptionKey(
    bucket: string,
    prefix: string,
    encryptionKey: EncryptionKey
  ): Promise<void> {
    this.validateNotClosed();

    if (!bucket || typeof bucket !== 'string') {
      throw new TypeError('bucket must be a non-empty string');
    }
    if (prefix == null) {
      prefix = '';
    }
    if (encryptionKey == null) {
      throw new TypeError('encryptionKey is required');
    }

    await native.accessOverrideEncryptionKey(this._handle, bucket, prefix, encryptionKey._handle);
  }

  /**
   * Open a project using this access grant.
   *
   * @returns Promise resolving to a ProjectResultStruct
   *
   * @example
   * ```typescript
   * const project = await access.openProject();
   * try {
   *   await project.createBucket('my-bucket');
   * } finally {
   *   await project.close();
   * }
   * ```
   */
  async openProject(): Promise<ProjectResultStruct> {
    this.validateNotClosed();
    const projectHandle = await native.openProject(this._handle);
    return new ProjectResultStruct(projectHandle);
  }

  /**
   * Open a project with custom configuration.
   *
   * @param config - Configuration options
   * @returns Promise resolving to a ProjectResultStruct
   */
  async configOpenProject(config: UplinkConfig): Promise<ProjectResultStruct> {
    this.validateNotClosed();

    if (config == null || typeof config !== 'object') {
      throw new TypeError('config must be an object');
    }

    const projectHandle = await native.configOpenProject(config, this._handle);
    return new ProjectResultStruct(projectHandle);
  }

  /**
   * Mark this access as closed (internal use)
   * @internal
   */
  _markClosed(): void {
    this._closed = true;
  }

  private validateNotClosed(): void {
    if (this._closed) {
      throw new Error('Access has been closed');
    }
  }
}
