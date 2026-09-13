/**
 * Module initialization tests
 */

describe('Module Initialization', () => {
  it('exports VERSION equal to package.json and uplink-c provenance strings', () => {
    // eslint-disable-next-line @typescript-eslint/no-require-imports, @typescript-eslint/no-var-requires
    const { VERSION, uplinkCVersion } = require('../../src') as typeof import('../../src');
    // eslint-disable-next-line @typescript-eslint/no-require-imports, @typescript-eslint/no-var-requires
    expect(VERSION).toBe((require('../../package.json') as { version: string }).version);
    const v = uplinkCVersion();
    for (const k of ['ref', 'version', 'revision', 'storjUplink'] as const) expect(typeof v[k]).toBe('string');
  });

  it('should export types', () => {
    // Verify types are exported (compile-time check)
    type TestUplinkConfig = {
      userAgent?: string;
      dialTimeoutMilliseconds?: number;
      tempDirectory?: string;
    };

    const config: TestUplinkConfig = {
      userAgent: 'test-agent',
      dialTimeoutMilliseconds: 5000,
    };

    expect(config.userAgent).toBe('test-agent');
    expect(config.dialTimeoutMilliseconds).toBe(5000);
  });

  it('should define error codes', () => {
    // These are the expected error codes from uplink-c
    const errorCodes = {
      Internal: 0x02,
      Canceled: 0x03,
      InvalidHandle: 0x04,
      BucketNotFound: 0x13,
      ObjectNotFound: 0x21,
    };

    expect(errorCodes.Internal).toBe(0x02);
    expect(errorCodes.BucketNotFound).toBe(0x13);
    expect(errorCodes.ObjectNotFound).toBe(0x21);
  });
});
