/**
 * @file test/unit/edge.test.ts
 * @brief Unit tests for edge/linkshare operations
 */

import { 
  edgeRegisterAccess, 
  edgeJoinShareUrl, 
  EdgeRegions 
} from '../../src/edge';
import type {
  EdgeConfig,
  EdgeCredentials,
  EdgeRegisterAccessOptions,
  EdgeShareURLOptions
} from '../../src/types';

describe('Edge Operations', () => {
  describe('edgeRegisterAccess function', () => {
    it('should be a function', () => {
      expect(typeof edgeRegisterAccess).toBe('function');
    });

    it('should throw TypeError for invalid config', async () => {
      await expect(edgeRegisterAccess(null as unknown as EdgeConfig, {}))
        .rejects.toThrow(TypeError);
      
      await expect(edgeRegisterAccess('not-object' as unknown as EdgeConfig, {}))
        .rejects.toThrow(TypeError);
    });

    it('should throw TypeError for missing authServiceAddress', async () => {
      await expect(edgeRegisterAccess({} as EdgeConfig, {}))
        .rejects.toThrow(TypeError);
      
      await expect(edgeRegisterAccess({ authServiceAddress: '' } as EdgeConfig, {}))
        .rejects.toThrow(TypeError);
    });

    it('should throw TypeError for missing access handle', async () => {
      const config: EdgeConfig = {
        authServiceAddress: 'auth.us1.storjshare.io:7777'
      };
      
      await expect(edgeRegisterAccess(config, null as unknown))
        .rejects.toThrow(TypeError);
    });
  });

  describe('edgeJoinShareUrl function', () => {
    it('should be a function', () => {
      expect(typeof edgeJoinShareUrl).toBe('function');
    });

    it('should throw TypeError for invalid baseUrl', async () => {
      await expect(edgeJoinShareUrl('', 'access-key', 'bucket', 'key'))
        .rejects.toThrow(TypeError);
      
      await expect(edgeJoinShareUrl(null as unknown as string, 'access-key', 'bucket', 'key'))
        .rejects.toThrow(TypeError);
    });

    it('should throw TypeError for invalid accessKeyId', async () => {
      await expect(edgeJoinShareUrl('https://link.storj.io', '', 'bucket', 'key'))
        .rejects.toThrow(TypeError);
      
      await expect(edgeJoinShareUrl('https://link.storj.io', null as unknown as string, 'bucket', 'key'))
        .rejects.toThrow(TypeError);
    });

    it('should throw TypeError for invalid bucket type', async () => {
      await expect(edgeJoinShareUrl('https://link.storj.io', 'access-key', 123 as unknown as string, 'key'))
        .rejects.toThrow(TypeError);
    });

    it('should throw TypeError for invalid key type', async () => {
      await expect(edgeJoinShareUrl('https://link.storj.io', 'access-key', 'bucket', 123 as unknown as string))
        .rejects.toThrow(TypeError);
    });
  });

  describe('EdgeRegions constant', () => {
    it('should export region configurations', () => {
      expect(EdgeRegions).toBeDefined();
      expect(typeof EdgeRegions).toBe('object');
    });

    it('should have US1 region', () => {
      expect(EdgeRegions.US1).toBeDefined();
      expect(EdgeRegions.US1.authService).toBe('auth.us1.storjshare.io:7777');
      expect(EdgeRegions.US1.linkshare).toBe('https://link.us1.storjshare.io');
    });

    it('should have EU1 region', () => {
      expect(EdgeRegions.EU1).toBeDefined();
      expect(EdgeRegions.EU1.authService).toBe('auth.eu1.storjshare.io:7777');
      expect(EdgeRegions.EU1.linkshare).toBe('https://link.eu1.storjshare.io');
    });

    it('should have AP1 region', () => {
      expect(EdgeRegions.AP1).toBeDefined();
      expect(EdgeRegions.AP1.authService).toBe('auth.ap1.storjshare.io:7777');
      expect(EdgeRegions.AP1.linkshare).toBe('https://link.ap1.storjshare.io');
    });
  });
});

describe('Edge Types', () => {
  describe('EdgeConfig interface', () => {
    it('should define expected structure', () => {
      const config: EdgeConfig = {
        authServiceAddress: 'auth.us1.storjshare.io:7777',
        certificatePem: '-----BEGIN CERTIFICATE-----\n...',
        insecureUnencryptedConnection: false
      };

      expect(config.authServiceAddress).toBe('auth.us1.storjshare.io:7777');
      expect(config.certificatePem).toBeDefined();
      expect(config.insecureUnencryptedConnection).toBe(false);
    });

    it('should allow minimal config', () => {
      const config: EdgeConfig = {
        authServiceAddress: 'auth.us1.storjshare.io:7777'
      };

      expect(config.authServiceAddress).toBe('auth.us1.storjshare.io:7777');
      expect(config.certificatePem).toBeUndefined();
      expect(config.insecureUnencryptedConnection).toBeUndefined();
    });
  });

  describe('EdgeCredentials interface', () => {
    it('should define expected structure', () => {
      const creds: EdgeCredentials = {
        accessKeyId: 'access-key-id-123',
        secretKey: 'secret-key-456',
        endpoint: 'https://gateway.storjshare.io'
      };

      expect(creds.accessKeyId).toBe('access-key-id-123');
      expect(creds.secretKey).toBe('secret-key-456');
      expect(creds.endpoint).toBe('https://gateway.storjshare.io');
    });
  });

  describe('EdgeRegisterAccessOptions interface', () => {
    it('should define expected structure', () => {
      const options: EdgeRegisterAccessOptions = {
        isPublic: true
      };

      expect(options.isPublic).toBe(true);
    });

    it('should allow empty options', () => {
      const options: EdgeRegisterAccessOptions = {};
      expect(options.isPublic).toBeUndefined();
    });
  });

  describe('EdgeShareURLOptions interface', () => {
    it('should define expected structure', () => {
      const options: EdgeShareURLOptions = {
        raw: true
      };

      expect(options.raw).toBe(true);
    });

    it('should allow empty options', () => {
      const options: EdgeShareURLOptions = {};
      expect(options.raw).toBeUndefined();
    });
  });
});

describe('Edge Usage Patterns', () => {
  describe('S3 compatibility workflow', () => {
    it('should document the S3 credential workflow', () => {
      // This test documents the expected workflow
      const workflow = `
        1. Get an access grant
        2. Configure edge service: { authServiceAddress: 'auth.us1.storjshare.io:7777' }
        3. Register access: edgeRegisterAccess(config, access, { isPublic: false })
        4. Get S3 credentials: { accessKeyId, secretKey, endpoint }
        5. Use with AWS SDK or S3-compatible tools
      `;
      expect(workflow).toBeDefined();
    });
  });

  describe('Linkshare workflow', () => {
    it('should document the linkshare URL workflow', () => {
      // This test documents the expected workflow
      const workflow = `
        1. Get an access grant
        2. Share access with download permission
        3. Register with isPublic: true
        4. Create share URL: edgeJoinShareUrl(baseUrl, accessKeyId, bucket, key)
        5. Share the URL publicly
      `;
      expect(workflow).toBeDefined();
    });
  });
});
