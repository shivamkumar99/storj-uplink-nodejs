/**
 * @file test/e2e/00-installation.test.ts
 * @brief E2E: Verify native module loads and basic initialization works
 *
 * This is the first E2E test — it verifies that the native addon loads,
 * the Uplink class can be instantiated, and access can be parsed/requested.
 *
 * Requires either:
 * - TEST_ACCESS_GRANT environment variable, OR
 * - TEST_SATELLITE, TEST_API_KEY, and TEST_PASSPHRASE environment variables
 */

import {
  Uplink,
  AccessResultStruct,
  ProjectResultStruct,
} from '../../src';
import { native } from '../../src/native';
import {
  hasAnyCredentials,
  hasCredentials,
  getAccess,
  testSatellite,
  testApiKey,
  testPassphrase,
} from '../helpers/credentials';

describe('E2E: Installation & Initialization', () => {
  const runTest = hasAnyCredentials ? it : it.skip;

  beforeAll(() => {
    if (!hasAnyCredentials) {
      console.log('⏭️  Skipping E2E installation tests: No valid credentials configured');
    }
  });

  describe('Native Module', () => {
    it('should load the native addon', () => {
      expect(native).toBeDefined();
      expect(typeof native).toBe('object');
    });

    it('should expose expected native functions', () => {
      expect(typeof native.parseAccess).toBe('function');
      expect(typeof native.openProject).toBe('function');
    });
  });

  describe('Uplink Initialization', () => {
    it('should create an Uplink instance', () => {
      const uplink = new Uplink();
      expect(uplink).toBeInstanceOf(Uplink);
    });

    runTest('should parse access grant successfully', async () => {
      const uplink = new Uplink();
      const access = await getAccess(uplink);

      expect(access).toBeInstanceOf(AccessResultStruct);
    });

    runTest('should serialize and re-parse access', async () => {
      const uplink = new Uplink();
      const access = await getAccess(uplink);
      const serialized = await access.serialize();

      expect(typeof serialized).toBe('string');
      expect(serialized.length).toBeGreaterThan(0);

      const reparsed = await uplink.parseAccess(serialized);
      expect(reparsed).toBeInstanceOf(AccessResultStruct);
    });

    runTest('should open a project from access', async () => {
      const uplink = new Uplink();
      const access = await getAccess(uplink);
      const project = await access.openProject();

      expect(project).toBeInstanceOf(ProjectResultStruct);
      await project.close();
    });
  });

  describe('Request Access (with credentials)', () => {
    const runCredTest = hasCredentials ? it : it.skip;

    runCredTest('should request access with passphrase', async () => {
      const uplink = new Uplink();
      const access = await uplink.requestAccessWithPassphrase(
        testSatellite!,
        testApiKey!,
        testPassphrase!,
      );

      expect(access).toBeInstanceOf(AccessResultStruct);
    });

    runCredTest('should get satellite address', async () => {
      const uplink = new Uplink();
      const access = await uplink.requestAccessWithPassphrase(
        testSatellite!,
        testApiKey!,
        testPassphrase!,
      );

      const addr = await access.satelliteAddress();
      expect(typeof addr).toBe('string');
      expect(addr.length).toBeGreaterThan(0);
    });
  });
});
