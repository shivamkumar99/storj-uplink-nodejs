/**
 * @file test/integration/project-config.test.ts
 * @brief Integration tests for configOpenProject
 *
 * Tests the chains:
 *   1. access → configOpenProject({dialTimeoutMilliseconds}) → listBuckets → close
 *   2. requestAccessWithPassphrase → configOpenProject({userAgent}) → listBuckets → close
 *
 * Requires environment variables:
 * - TEST_SATELLITE, TEST_API_KEY, TEST_PASSPHRASE
 * - (Optional) TEST_ACCESS_GRANT
 */

import { Uplink, ProjectResultStruct } from '../../src';
import {
  hasAnyCredentials,
  hasCredentials,
  getAccess,
  testSatellite,
  testApiKey,
  testPassphrase,
} from '../helpers/credentials';

describe('Integration: Project Config Operations', () => {
  let uplink: Uplink;

  beforeAll(() => {
    uplink = new Uplink();

    if (!hasAnyCredentials) {
      console.log('⏭️  Skipping project-config integration tests: No valid credentials configured');
    }
  });

  describe('configOpenProject via access', () => {
    const runTest = hasAnyCredentials ? it : it.skip;

    runTest('should open project with dialTimeout config', async () => {
      const access = await getAccess(uplink);
      const project = await access.configOpenProject({
        dialTimeoutMilliseconds: 30000,
      });

      expect(project).toBeInstanceOf(ProjectResultStruct);

      try {
        const buckets = await project.listBuckets();
        expect(Array.isArray(buckets)).toBe(true);
      } finally {
        await project.close();
      }
    }, 60000);
  });

  describe('configOpenProject via requestAccessWithPassphrase', () => {
    const runTest = hasCredentials ? it : it.skip;

    runTest('should open project with userAgent config', async () => {
      const access = await uplink.requestAccessWithPassphrase(
        testSatellite!,
        testApiKey!,
        testPassphrase!
      );
      const project = await access.configOpenProject({
        userAgent: 'uplink-nodejs-integration-test/1.0',
      });

      expect(project).toBeInstanceOf(ProjectResultStruct);

      try {
        const buckets = await project.listBuckets();
        expect(Array.isArray(buckets)).toBe(true);
      } finally {
        await project.close();
      }
    }, 60000);
  });
});
