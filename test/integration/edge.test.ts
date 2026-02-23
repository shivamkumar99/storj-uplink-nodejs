/**
 * @file test/integration/edge.test.ts
 * @brief Integration tests for edge/linkshare operations
 *
 * Tests: edgeRegisterAccess, edgeJoinShareUrl
 *
 * Tests the chains:
 *   1. parseAccess → share(public) → edgeRegisterAccess → verify credentials
 *   2. edgeRegisterAccess → edgeJoinShareUrl → verify URL
 *   3. edgeJoinShareUrl({raw: true}) → verify URL
 *
 * Requires environment variables:
 * - TEST_ACCESS_GRANT or (TEST_SATELLITE + TEST_API_KEY + TEST_PASSPHRASE)
 * - TEST_EDGE_AUTH_SERVICE (optional, defaults to EdgeRegions.US1.authService)
 */

import {
  Uplink,
  AccessResultStruct,
  edgeRegisterAccess,
  edgeJoinShareUrl,
  EdgeRegions,
} from '../../src';
import type { EdgeConfig } from '../../src/types';
import { hasAnyCredentials, getAccess } from '../helpers/credentials';

describe('Integration: Edge/Linkshare Operations', () => {
  const validCredentials = hasAnyCredentials;
  const runTest = validCredentials ? it : it.skip;

  const authService = process.env.TEST_EDGE_AUTH_SERVICE || EdgeRegions.US1.authService;
  const linkshareBase = EdgeRegions.US1.linkshare;

  let uplink: Uplink;
  let access: AccessResultStruct;

  beforeAll(async () => {
    if (!validCredentials) {
      console.log('⏭️  Skipping edge integration tests: No valid credentials configured');
      return;
    }

    uplink = new Uplink();
    access = await getAccess(uplink);
  });

  runTest('should register access and get S3-compatible credentials', async () => {
    // Create a public shared access for edge registration
    const sharedAccess = await access.share(
      { allowDownload: true, allowUpload: false, allowList: true, allowDelete: false },
      []
    );

    const edgeConfig: EdgeConfig = {
      authServiceAddress: authService,
    };

    const credentials = await edgeRegisterAccess(
      edgeConfig,
      sharedAccess._nativeHandle,
      { isPublic: true }
    );

    expect(credentials).toBeDefined();
    expect(typeof credentials.accessKeyId).toBe('string');
    expect(credentials.accessKeyId.length).toBeGreaterThan(0);
    expect(typeof credentials.secretKey).toBe('string');
    expect(credentials.secretKey.length).toBeGreaterThan(0);
    expect(typeof credentials.endpoint).toBe('string');
    expect(credentials.endpoint.length).toBeGreaterThan(0);
  }, 60000);

  runTest('should join share URL', async () => {
    const sharedAccess = await access.share(
      { allowDownload: true, allowList: true },
      []
    );

    const edgeConfig: EdgeConfig = {
      authServiceAddress: authService,
    };

    const credentials = await edgeRegisterAccess(
      edgeConfig,
      sharedAccess._nativeHandle,
      { isPublic: true }
    );

    const shareUrl = await edgeJoinShareUrl(
      linkshareBase,
      credentials.accessKeyId,
      'test-bucket',
      'test-object.txt'
    );

    expect(typeof shareUrl).toBe('string');
    expect(shareUrl.length).toBeGreaterThan(0);
    expect(shareUrl).toContain(linkshareBase);
    expect(shareUrl).toContain(credentials.accessKeyId);
  }, 60000);

  runTest('should join share URL with raw option', async () => {
    const sharedAccess = await access.share(
      { allowDownload: true, allowList: true },
      []
    );

    const edgeConfig: EdgeConfig = {
      authServiceAddress: authService,
    };

    const credentials = await edgeRegisterAccess(
      edgeConfig,
      sharedAccess._nativeHandle,
      { isPublic: true }
    );

    const rawUrl = await edgeJoinShareUrl(
      linkshareBase,
      credentials.accessKeyId,
      'test-bucket',
      'test-file.txt',
      { raw: true }
    );

    expect(typeof rawUrl).toBe('string');
    expect(rawUrl.length).toBeGreaterThan(0);
    expect(rawUrl).toContain(linkshareBase);

    // Raw URL should differ from non-raw URL
    const normalUrl = await edgeJoinShareUrl(
      linkshareBase,
      credentials.accessKeyId,
      'test-bucket',
      'test-file.txt'
    );

    // Both should be valid but different
    expect(rawUrl).not.toBe(normalUrl);
  }, 60000);
});
