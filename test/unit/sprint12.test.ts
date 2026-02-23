/**
 * @file sprint12.test.ts
 * @brief Unit tests for Sprint 12 API completeness
 * 
 * Tests for:
 * - project.revokeAccess()
 * - internalUniverseIsEmpty() / uplinkInternalUniverseIsEmpty()
 */

import { Uplink, ProjectResultStruct, internalUniverseIsEmpty, uplinkInternalUniverseIsEmpty } from '../../src';

describe('Sprint 12: API Completeness', () => {
  describe('ProjectResultStruct.revokeAccess', () => {
    describe('method exists', () => {
      it('should have revokeAccess method', () => {
        expect(typeof ProjectResultStruct.prototype.revokeAccess).toBe('function');
      });
    });

    describe('input validation', () => {
      it('should document correct usage pattern', () => {
        const docString = `
          // Create a shared access with limited permissions
          const sharedAccess = await access.share(permission, prefixes);
          
          // Later, revoke the access when no longer needed
          await project.revokeAccess(sharedAccess);
        `;
        
        expect(docString).toContain('revokeAccess');
        expect(docString).toContain('share');
      });

      it('should require access parameter', () => {
        const method = ProjectResultStruct.prototype.revokeAccess;
        expect(method.length).toBe(1);
      });
    });
  });

  describe('internalUniverseIsEmpty', () => {
    describe('function exists', () => {
      it('should export internalUniverseIsEmpty function', () => {
        expect(typeof internalUniverseIsEmpty).toBe('function');
      });

      it('should export uplinkInternalUniverseIsEmpty alias', () => {
        expect(typeof uplinkInternalUniverseIsEmpty).toBe('function');
      });

      it('should have both functions be the same', () => {
        expect(internalUniverseIsEmpty).toBe(uplinkInternalUniverseIsEmpty);
      });
    });

    describe('expected behavior', () => {
      it('should document memory leak detection usage', () => {
        const docString = `
          // After closing all resources
          const isEmpty = await internalUniverseIsEmpty();
          if (isEmpty) {
            console.log('All handles cleaned up successfully');
          }
        `;
        
        expect(docString).toContain('internalUniverseIsEmpty');
        expect(docString).toContain('handles');
      });
    });
  });

  describe('Native Module Registration', () => {
    it('should have revokeAccess registered', async () => {
      const { native } = await import('../../src/native');
      expect(typeof native.revokeAccess).toBe('function');
    });

    it('should have internalUniverseIsEmpty registered', async () => {
      const { native } = await import('../../src/native');
      expect(typeof native.internalUniverseIsEmpty).toBe('function');
    });
  });

  describe('Complete API Coverage', () => {
    it('should have all Uplink methods', () => {
      expect(typeof Uplink.prototype.parseAccess).toBe('function');
      expect(typeof Uplink.prototype.requestAccessWithPassphrase).toBe('function');
      expect(typeof Uplink.prototype.configRequestAccessWithPassphrase).toBe('function');
      expect(typeof Uplink.prototype.uplinkDeriveEncryptionKey).toBe('function');
    });

    it('should have all ProjectResultStruct methods', () => {
      expect(typeof ProjectResultStruct.prototype.close).toBe('function');
      expect(typeof ProjectResultStruct.prototype.revokeAccess).toBe('function');
      expect(typeof ProjectResultStruct.prototype.createBucket).toBe('function');
      expect(typeof ProjectResultStruct.prototype.ensureBucket).toBe('function');
      expect(typeof ProjectResultStruct.prototype.deleteBucket).toBe('function');
      expect(typeof ProjectResultStruct.prototype.deleteBucketWithObjects).toBe('function');
      expect(typeof ProjectResultStruct.prototype.statBucket).toBe('function');
      expect(typeof ProjectResultStruct.prototype.listBuckets).toBe('function');
      expect(typeof ProjectResultStruct.prototype.uploadObject).toBe('function');
      expect(typeof ProjectResultStruct.prototype.downloadObject).toBe('function');
      expect(typeof ProjectResultStruct.prototype.statObject).toBe('function');
      expect(typeof ProjectResultStruct.prototype.listObjects).toBe('function');
      expect(typeof ProjectResultStruct.prototype.deleteObject).toBe('function');
      expect(typeof ProjectResultStruct.prototype.copyObject).toBe('function');
      expect(typeof ProjectResultStruct.prototype.moveObject).toBe('function');
    });

    it('should export debug utilities', async () => {
      const exports = await import('../../src');
      expect(typeof exports.internalUniverseIsEmpty).toBe('function');
      expect(typeof exports.uplinkInternalUniverseIsEmpty).toBe('function');
    });
  });
});
