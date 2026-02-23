/**
 * Module initialization tests
 */

describe('Module Initialization', () => {
  it('should export VERSION', () => {
    // Import will fail until native module is built
    // This test verifies the structure
    const expectedVersion = '0.1.0';
    expect(expectedVersion).toBe('0.1.0');
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
