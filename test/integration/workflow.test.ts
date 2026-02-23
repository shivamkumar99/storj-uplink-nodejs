/**
 * @file test/integration/workflow.test.ts
 * @brief Full workflow integration tests for upload/download cycle
 * 
 * These tests require either:
 * - TEST_ACCESS_GRANT environment variable, OR
 * - TEST_SATELLITE, TEST_API_KEY, and TEST_PASSPHRASE environment variables
 * 
 * Skip gracefully if credentials are not available.
 */

import { Uplink, AccessResultStruct, ProjectResultStruct, UploadResultStruct, DownloadResultStruct } from '../../src';
import { hasAnyCredentials, getAccess } from '../helpers/credentials';

// Extend globalThis for test bucket name
declare const global: typeof globalThis & { testBucketName?: string };

describe('Integration: Full Workflow', () => {
  const bucketName = global.testBucketName || `test-${Date.now()}`;
  const validCredentials = hasAnyCredentials;
  
  let uplink: Uplink;
  let access: AccessResultStruct | null = null;
  let project: ProjectResultStruct | null = null;
  let bucketCreated = false;

  // Skip all tests if no credentials
  const runTest = validCredentials ? it : it.skip;

  beforeAll(async () => {
    if (!validCredentials) {
      console.log('⏭️  Skipping integration tests: No valid credentials configured');
      console.log('   Set TEST_ACCESS_GRANT or (TEST_SATELLITE + TEST_API_KEY + TEST_PASSPHRASE)');
      return;
    }
    
    uplink = new Uplink();
    try {
      access = await getAccess(uplink);
      project = await access.openProject();
    } catch (error) {
      console.error('Failed to initialize test environment:', error);
    }
  });

  afterAll(async () => {
    if (project) {
      try {
        // Clean up test bucket if created
        if (bucketCreated) {
          // Delete all objects in bucket first
          const objects = await project.listObjects(bucketName, { recursive: true });
          for (const obj of objects) {
            await project.deleteObject(bucketName, obj.key);
          }
          await project.deleteBucket(bucketName);
        }
      } catch (e) {
        // Bucket may not exist
      }
      await project.close();
    }
  });

  describe('Bucket Operations', () => {
    runTest('should create a bucket', async () => {
      if (!project) throw new Error('Project not initialized');
      
      const bucket = await project.createBucket(bucketName);
      bucketCreated = true;
      
      expect(bucket).toBeDefined();
      expect(bucket.name).toBe(bucketName);
    });

    runTest('should stat bucket', async () => {
      if (!project) throw new Error('Project not initialized');
      
      const bucket = await project.statBucket(bucketName);
      
      expect(bucket).toBeDefined();
      expect(bucket.name).toBe(bucketName);
    });

    runTest('should list buckets', async () => {
      if (!project) throw new Error('Project not initialized');
      
      const buckets = await project.listBuckets();
      
      expect(Array.isArray(buckets)).toBe(true);
      expect(buckets.some(b => b.name === bucketName)).toBe(true);
    });
  });

  describe('Object Upload/Download Cycle', () => {
    const testKey = 'test-file.txt';
    const testContent = Buffer.from('Hello, Storj! This is a test file content.');

    runTest('should upload object', async () => {
      if (!project) throw new Error('Project not initialized');
      
      const upload = await project.uploadObject(bucketName, testKey);
      
      expect(upload).toBeInstanceOf(UploadResultStruct);
      
      const bytesWritten = await upload.write(testContent, testContent.length);
      expect(bytesWritten).toBe(testContent.length);
      
      await upload.commit();
    });

    runTest('should stat uploaded object', async () => {
      if (!project) throw new Error('Project not initialized');
      
      const objInfo = await project.statObject(bucketName, testKey);
      
      expect(objInfo).toBeDefined();
      expect(objInfo.key).toBe(testKey);
      expect(objInfo.system.contentLength).toBe(testContent.length);
    });

    runTest('should list objects', async () => {
      if (!project) throw new Error('Project not initialized');
      
      const objects = await project.listObjects(bucketName);
      
      expect(Array.isArray(objects)).toBe(true);
      expect(objects.some(o => o.key === testKey)).toBe(true);
    });

    runTest('should download object', async () => {
      if (!project) throw new Error('Project not initialized');
      
      const download = await project.downloadObject(bucketName, testKey);
      
      expect(download).toBeInstanceOf(DownloadResultStruct);
      
      // Read exactly the expected amount to avoid EOF (old uplink-nodejs pattern)
      const downloadBuffer = Buffer.alloc(testContent.length);
      const result = await download.read(downloadBuffer, testContent.length);
      
      expect(result.bytesRead).toBe(testContent.length);
      expect(downloadBuffer.slice(0, result.bytesRead).toString()).toBe(testContent.toString());
      
      await download.close();
    });

    runTest('should delete object', async () => {
      if (!project) throw new Error('Project not initialized');
      
      await project.deleteObject(bucketName, testKey);
      
      // Verify deletion
      const objects = await project.listObjects(bucketName);
      expect(objects.some(o => o.key === testKey)).toBe(false);
    });
  });

  describe('Copy and Move Operations', () => {
    const srcKey = 'copy-test-source.txt';
    const copyKey = 'copy-test-destination.txt';
    const moveKey = 'move-test-destination.txt';
    const content = Buffer.from('Content for copy/move test');

    runTest('should copy object', async () => {
      if (!project) throw new Error('Project not initialized');
      
      // Create source object
      const upload = await project.uploadObject(bucketName, srcKey);
      await upload.write(content, content.length);
      await upload.commit();
      
      // Copy object
      const copiedObj = await project.copyObject(bucketName, srcKey, bucketName, copyKey);
      
      expect(copiedObj).toBeDefined();
      expect(copiedObj.key).toBe(copyKey);
      
      // Verify both exist
      const srcInfo = await project.statObject(bucketName, srcKey);
      const copyInfo = await project.statObject(bucketName, copyKey);
      
      expect(srcInfo.key).toBe(srcKey);
      expect(copyInfo.key).toBe(copyKey);
      
      // Cleanup
      await project.deleteObject(bucketName, copyKey);
    });

    runTest('should move object', async () => {
      if (!project) throw new Error('Project not initialized');
      
      // Move the source object
      await project.moveObject(bucketName, srcKey, bucketName, moveKey);
      
      // Verify move
      const movedInfo = await project.statObject(bucketName, moveKey);
      expect(movedInfo.key).toBe(moveKey);
      
      // Source should no longer exist
      await expect(project.statObject(bucketName, srcKey)).rejects.toThrow();
      
      // Cleanup
      await project.deleteObject(bucketName, moveKey);
    });
  });

  describe('Custom Metadata', () => {
    const metaKey = 'metadata-test.txt';
    const content = Buffer.from('Content with metadata');
    const customMetadata: Record<string, string> = {
      'custom-key': 'custom-value',
      'author': 'test-suite',
      'version': '1.0'
    };

    runTest('should upload with custom metadata', async () => {
      if (!project) throw new Error('Project not initialized');
      
      const upload = await project.uploadObject(bucketName, metaKey);
      await upload.write(content, content.length);
      await upload.setCustomMetadata(customMetadata);
      await upload.commit();
      
      // Verify metadata
      const objInfo = await project.statObject(bucketName, metaKey);
      expect(objInfo.custom).toBeDefined();
      expect(objInfo.custom['custom-key']).toBe('custom-value');
      expect(objInfo.custom['author']).toBe('test-suite');
      
      // Cleanup
      await project.deleteObject(bucketName, metaKey);
    });
  });
});
