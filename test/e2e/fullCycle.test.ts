/**
 * @file test/e2e/fullCycle.test.ts
 * @brief End-to-end tests covering complete application workflows
 * 
 * These tests simulate real-world usage patterns with complete workflows
 * including iterator-based listing (Sprint 17), multipart uploads,
 * access sharing, and error recovery.
 * 
 * Requires either:
 * - TEST_ACCESS_GRANT environment variable, OR
 * - TEST_SATELLITE, TEST_API_KEY, and TEST_PASSPHRASE environment variables
 */

import {
  Uplink,
  AccessResultStruct,
  ProjectResultStruct,
  MultipartUpload,
  beginMultipartUpload,
  listMultipartUploads,
} from '../../src';
import type {
  BucketInfo,
  ObjectInfo,
  UploadInfo,
} from '../../src/types';
import { hasAnyCredentials, getAccess } from '../helpers/credentials';

describe('E2E: Complete Application Workflows', () => {
  const bucketName = `e2e-test-${Date.now()}`;
  const validCredentials = hasAnyCredentials;
  
  let uplink: Uplink;
  let access: AccessResultStruct | null = null;
  let project: ProjectResultStruct | null = null;

  // Skip all tests if no credentials
  const runTest = validCredentials ? it : it.skip;

  beforeAll(async () => {
    if (!validCredentials) {
      console.log('⏭️  Skipping E2E tests: No valid credentials configured');
      console.log('   Set TEST_ACCESS_GRANT or (TEST_SATELLITE + TEST_API_KEY + TEST_PASSPHRASE)');
      return;
    }
    
    uplink = new Uplink();
    access = await getAccess(uplink);
    project = await access.openProject();
    
    // Create test bucket
    try {
      await project.createBucket(bucketName);
    } catch (e) {
      // Bucket may already exist
    }
  });

  afterAll(async () => {
    if (project) {
      try {
        // Clean up all test objects
        const objects = await project.listObjects(bucketName, { recursive: true });
        for (const obj of objects) {
          await project.deleteObject(bucketName, obj.key);
        }
        await project.deleteBucket(bucketName);
      } catch (e) {
        // Cleanup may fail if bucket doesn't exist
      }
      await project.close();
    }
  });

  describe('File Backup Workflow', () => {
    runTest('should simulate backing up multiple files', async () => {
      if (!project) throw new Error('Project not initialized');
      
      const files = [
        { name: 'documents/report.txt', content: 'Annual Report 2024' },
        { name: 'documents/notes.txt', content: 'Meeting notes from January' },
        { name: 'images/photo.dat', content: 'Binary image data placeholder' },
      ];

      // Upload all files
      for (const file of files) {
        const upload = await project.uploadObject(bucketName, file.name);
        const buffer = Buffer.from(file.content);
        await upload.write(buffer, buffer.length);
        await upload.commit();
      }

      // List and verify
      const objects = await project.listObjects(bucketName, { recursive: true });
      expect(objects.length).toBeGreaterThanOrEqual(files.length);

      // Download and verify content
      for (const file of files) {
        const download = await project.downloadObject(bucketName, file.name);
        // Read exactly the expected amount (old uplink-nodejs pattern)
        const buffer = Buffer.alloc(file.content.length);
        const result = await download.read(buffer, buffer.length);
        await download.close();
        
        const content = buffer.slice(0, result.bytesRead).toString();
        expect(content).toBe(file.content);
      }

      // Clean up files
      for (const file of files) {
        await project.deleteObject(bucketName, file.name);
      }
    }, 60000);
  });

  describe('Large File Streaming', () => {
    runTest('should handle streaming large data in chunks', async () => {
      if (!project) throw new Error('Project not initialized');
      
      const objectKey = 'large-file-text.txt';
      const chunkSize = 64 * 1024; // 64KB chunks
      const totalChunks = 4;

      // Build identifiable text data: each 64-byte line is "CHUNK-XX-LINE-YYYYYY\n" padded
      // This way we can tell exactly which chunk/line was received or missing
      const lineSize = 64;
      const linesPerChunk = chunkSize / lineSize; // 1024 lines per chunk
      const uploadedChunks: Buffer[] = [];

      const upload = await project.uploadObject(bucketName, objectKey);
      for (let c = 0; c < totalChunks; c++) {
        let chunkStr = '';
        for (let l = 0; l < linesPerChunk; l++) {
          const tag = `CHUNK-${String(c).padStart(2, '0')}-LINE-${String(l).padStart(6, '0')}`;
          chunkStr += tag.padEnd(lineSize - 1, '.') + '\n';
        }
        const chunkBuf = Buffer.from(chunkStr, 'utf-8');
        uploadedChunks.push(chunkBuf);
        const bytesWritten = await upload.write(chunkBuf, chunkBuf.length);
        console.log(`[LFS] Upload chunk ${c}: size=${chunkBuf.length}, written=${bytesWritten}`);
      }
      await upload.commit();

      const totalSize = uploadedChunks.reduce((s, b) => s + b.length, 0);
      console.log(`[LFS] Total uploaded: ${totalSize} bytes`);

      // Build the complete expected buffer for comparison
      const expectedBuffer = Buffer.concat(uploadedChunks);

      // Verify stored size
      const objInfo = await project.statObject(bucketName, objectKey);
      expect(objInfo.system.contentLength).toBe(totalSize);

      // Download using loop-in-JS pattern
      const download = await project.downloadObject(bucketName, objectKey);
      const downloadInfo = await download.info();
      const downloadSize = downloadInfo.system.contentLength;
      console.log(`[LFS] Download info says: ${downloadSize} bytes`);

      const receivedBuffer = Buffer.alloc(downloadSize);
      let totalRead = 0;
      let readCallNum = 0;
      let zeroReadRetries = 0;
      const MAX_ZERO_RETRIES = 100;
      
      while (totalRead < downloadSize) {
        readCallNum++;
        const remaining = downloadSize - totalRead;
        const toRead = Math.min(chunkSize, remaining);
        const buffer = Buffer.alloc(toRead);
        try {
          const result = await download.read(buffer, toRead);
          console.log(`[LFS] Read #${readCallNum}: requested=${toRead}, got=${result.bytesRead}, total=${totalRead + result.bytesRead}/${downloadSize}`);
          if (result.bytesRead > 0) {
            buffer.copy(receivedBuffer, totalRead, 0, result.bytesRead);
            totalRead += result.bytesRead;
            zeroReadRetries = 0;
          } else {
            zeroReadRetries++;
            console.log(`[LFS] Zero-byte read (retry ${zeroReadRetries}/${MAX_ZERO_RETRIES})`);
            if (zeroReadRetries >= MAX_ZERO_RETRIES) break;
          }
        } catch (err: unknown) {
          const errObj = err as Record<string, unknown>;
          const partialBytes = typeof errObj.bytesRead === 'number' ? errObj.bytesRead : 0;
          if (partialBytes > 0) {
            buffer.copy(receivedBuffer, totalRead, 0, partialBytes);
            totalRead += partialBytes;
          }
          const errMsg = err instanceof Error ? err.message : String(err);
          console.log(`[LFS] Error at read #${readCallNum}, total=${totalRead}: ${errMsg} (partialBytes=${partialBytes})`);
          break;
        }
      }
      
      await download.close();

      // Diagnostic: if size mismatch, find exactly where the data diverges
      if (totalRead !== totalSize) {
        console.log(`[LFS] SIZE MISMATCH: expected=${totalSize}, got=${totalRead}`);
        // Find first diverging byte
        const minLen = Math.min(totalRead, totalSize);
        let firstDiff = -1;
        for (let i = 0; i < minLen; i++) {
          if (receivedBuffer[i] !== expectedBuffer[i]) {
            firstDiff = i;
            break;
          }
        }
        if (firstDiff >= 0) {
          const lineNum = Math.floor(firstDiff / lineSize);
          const chunkNum = Math.floor(lineNum / linesPerChunk);
          console.log(`[LFS] First byte mismatch at offset ${firstDiff} (chunk ${chunkNum}, line ${lineNum})`);
          console.log(`[LFS]   Expected: ${expectedBuffer.slice(firstDiff, firstDiff + 64).toString('utf-8')}`);
          console.log(`[LFS]   Received: ${receivedBuffer.slice(firstDiff, firstDiff + 64).toString('utf-8')}`);
        } else {
          console.log(`[LFS] Data matches up to byte ${minLen} — remaining ${totalSize - totalRead} bytes were never read`);
          // Show what the next expected line would be
          if (totalRead < totalSize) {
            const nextLine = expectedBuffer.slice(totalRead, Math.min(totalRead + 128, totalSize)).toString('utf-8');
            console.log(`[LFS] Next expected data at offset ${totalRead}: ${nextLine}`);
          }
        }
      }

      expect(totalRead).toBe(totalSize);

      // Verify content integrity
      const receivedContent = receivedBuffer.slice(0, totalRead);
      const expectedContent = expectedBuffer.slice(0, totalRead);
      expect(receivedContent.equals(expectedContent)).toBe(true);

      // Cleanup
      await project.deleteObject(bucketName, objectKey);
    }, 60000);
  });

  describe('Folder Organization', () => {
    runTest('should handle nested folder structures', async () => {
      if (!project) throw new Error('Project not initialized');
      
      const structure = [
        'project/src/index.ts',
        'project/src/utils/helpers.ts',
        'project/src/utils/logger.ts',
        'project/test/index.test.ts',
        'project/package.json',
        'project/README.md',
      ];

      // Create folder structure
      for (const path of structure) {
        const upload = await project.uploadObject(bucketName, path);
        const content = Buffer.from(`Content of ${path}`);
        await upload.write(content, content.length);
        await upload.commit();
      }

      // List all files recursively
      const allObjects = await project.listObjects(bucketName, { 
        prefix: 'project/',
        recursive: true 
      });
      expect(allObjects.length).toBe(structure.length);

      // List only top-level (non-recursive)
      const topLevel = await project.listObjects(bucketName, { 
        prefix: 'project/',
        recursive: false 
      });
      
      // Should include prefixes for subfolders
      expect(topLevel.length).toBeGreaterThan(0);

      // Cleanup
      for (const path of structure) {
        await project.deleteObject(bucketName, path);
      }
    }, 60000);
  });

  describe('Error Recovery', () => {
    runTest('should handle and recover from errors gracefully', async () => {
      if (!project) throw new Error('Project not initialized');
      
      // Try to access non-existent object
      await expect(
        project.statObject(bucketName, 'non-existent-object-xyz')
      ).rejects.toThrow();

      // Project should still be usable after error
      const buckets = await project.listBuckets();
      expect(Array.isArray(buckets)).toBe(true);

      // Try to download non-existent object
      await expect(
        project.downloadObject(bucketName, 'another-non-existent-object')
      ).rejects.toThrow();

      // Project should still be usable
      const objects = await project.listObjects(bucketName);
      expect(Array.isArray(objects)).toBe(true);
    });
  });

  describe('Metadata Workflow', () => {
    runTest('should support full metadata lifecycle', async () => {
      if (!project) throw new Error('Project not initialized');
      
      const objectKey = 'metadata-workflow.txt';
      const content = Buffer.from('File with metadata');
      const initialMetadata: Record<string, string> = {
        'content-type': 'text/plain',
        'author': 'test-suite',
        'created': new Date().toISOString(),
      };

      // Upload with metadata
      const upload = await project.uploadObject(bucketName, objectKey);
      await upload.write(content, content.length);
      await upload.setCustomMetadata(initialMetadata);
      await upload.commit();

      // Verify metadata
      let objInfo = await project.statObject(bucketName, objectKey);
      expect(objInfo.custom['author']).toBe('test-suite');
      expect(objInfo.custom['content-type']).toBe('text/plain');

      // Update metadata (if supported)
      const updatedMetadata: Record<string, string> = {
        'author': 'updated-author',
        'modified': new Date().toISOString(),
      };
      
      try {
        await project.updateObjectMetadata(bucketName, objectKey, updatedMetadata);
        
        // Verify updated metadata
        objInfo = await project.statObject(bucketName, objectKey);
        expect(objInfo.custom['author']).toBe('updated-author');
      } catch (e) {
        // updateObjectMetadata may not be available in all versions
        console.log('    Note: updateObjectMetadata not available');
      }

      // Cleanup
      await project.deleteObject(bucketName, objectKey);
    }, 60000);
  });

  // ========== Sprint 17: Iterator-based Listing E2E Tests ==========

  describe('Iterator-based Bucket Listing', () => {
    runTest('should list buckets using iterator-driven listing', async () => {
      if (!project) throw new Error('Project not initialized');

      // Create several extra buckets
      const extraBuckets = [
        `e2e-iter-b1-${Date.now()}`,
        `e2e-iter-b2-${Date.now()}`,
        `e2e-iter-b3-${Date.now()}`,
      ];

      for (const name of extraBuckets) {
        await project.createBucket(name);
      }

      try {
        // listBuckets now uses JS iterator loop internally
        const buckets: BucketInfo[] = await project.listBuckets();

        expect(Array.isArray(buckets)).toBe(true);
        expect(buckets.length).toBeGreaterThanOrEqual(extraBuckets.length + 1); // +1 for main test bucket

        // All extra buckets should appear
        const bucketNames = buckets.map((b: BucketInfo) => b.name);
        for (const name of extraBuckets) {
          expect(bucketNames).toContain(name);
        }

        // Each bucket should have a name and created date
        for (const b of buckets) {
          expect(typeof b.name).toBe('string');
          expect(b.name.length).toBeGreaterThan(0);
          expect(b.created).toBeDefined();
        }
      } finally {
        for (const name of extraBuckets) {
          try { await project.deleteBucket(name); } catch (_) { /* cleanup */ }
        }
      }
    }, 60000);

    runTest('should return empty array when listing buckets with no matches', async () => {
      if (!access) throw new Error('Access not initialized');

      // Create a restricted access that can only see a non-existent prefix
      const restricted = await access.share(
        { allowDownload: true, allowUpload: false, allowList: true, allowDelete: false },
        [{ bucket: 'completely-nonexistent-bucket-xyz-12345' }]
      );
      const restrictedProject = await restricted.openProject();

      try {
        const buckets = await restrictedProject.listBuckets();
        expect(Array.isArray(buckets)).toBe(true);
        // May or may not be empty depending on satellite behavior, but must not throw
      } finally {
        await restrictedProject.close();
      }
    }, 60000);
  });

  describe('Iterator-based Object Listing', () => {
    runTest('should list objects with various options using iterator', async () => {
      if (!project) throw new Error('Project not initialized');

      const prefix = `e2e-iter-obj-${Date.now()}/`;
      const keys = [
        `${prefix}file-a.txt`,
        `${prefix}file-b.txt`,
        `${prefix}sub/file-c.txt`,
        `${prefix}sub/file-d.txt`,
        `${prefix}sub/deep/file-e.txt`,
      ];

      // Upload test objects
      for (const key of keys) {
        const upload = await project.uploadObject(bucketName, key);
        const buf = Buffer.from(`content-of-${key}`);
        await upload.write(buf, buf.length);
        await upload.commit();
      }

      try {
        // Recursive listing — should find all 5 objects
        const allObjects: ObjectInfo[] = await project.listObjects(bucketName, {
          prefix,
          recursive: true,
        });
        expect(allObjects.length).toBe(5);
        const allKeys = allObjects.map((o: ObjectInfo) => o.key);
        for (const key of keys) {
          expect(allKeys).toContain(key);
        }

        // Non-recursive listing — should find files + prefix entries
        const topLevel: ObjectInfo[] = await project.listObjects(bucketName, {
          prefix,
          recursive: false,
        });
        expect(topLevel.length).toBe(3); // file-a.txt, file-b.txt, sub/

        const directFiles = topLevel.filter((o: ObjectInfo) => !o.isPrefix);
        const prefixItems = topLevel.filter((o: ObjectInfo) => o.isPrefix);
        expect(directFiles.length).toBe(2);
        expect(prefixItems.length).toBe(1);
        expect(prefixItems[0].key).toBe(`${prefix}sub/`);

        // Nested non-recursive listing under sub/
        const subLevel: ObjectInfo[] = await project.listObjects(bucketName, {
          prefix: `${prefix}sub/`,
          recursive: false,
        });
        expect(subLevel.length).toBe(3); // file-c.txt, file-d.txt, deep/

        // With system + custom metadata
        const withMeta: ObjectInfo[] = await project.listObjects(bucketName, {
          prefix,
          recursive: true,
          system: true,
          custom: true,
        });
        expect(withMeta.length).toBe(5);
        for (const obj of withMeta) {
          expect(obj.system).toBeDefined();
          expect(typeof obj.system.contentLength).toBe('number');
        }

        // Empty prefix listing — should return nothing
        const emptyResult: ObjectInfo[] = await project.listObjects(bucketName, {
          prefix: 'nonexistent-prefix-xyz-999/',
          recursive: true,
        });
        expect(emptyResult.length).toBe(0);
      } finally {
        // Cleanup
        for (const key of keys) {
          try { await project.deleteObject(bucketName, key); } catch (_) { /* cleanup */ }
        }
      }
    }, 60000);

    runTest('should handle listing many objects (iterator stress)', async () => {
      if (!project) throw new Error('Project not initialized');

      const prefix = `e2e-iter-many-${Date.now()}/`;
      const count = 25; // Enough to exercise iterator loop

      // Upload objects
      for (let i = 0; i < count; i++) {
        const key = `${prefix}item-${String(i).padStart(4, '0')}.txt`;
        const upload = await project.uploadObject(bucketName, key);
        const buf = Buffer.from(`data-${i}`);
        await upload.write(buf, buf.length);
        await upload.commit();
      }

      try {
        const objects: ObjectInfo[] = await project.listObjects(bucketName, {
          prefix,
          recursive: true,
        });
        expect(objects.length).toBe(count);

        // Verify all expected keys are present
        const returnedKeys = objects.map((o: ObjectInfo) => o.key).sort();
        for (let i = 0; i < count; i++) {
          const expectedKey = `${prefix}item-${String(i).padStart(4, '0')}.txt`;
          expect(returnedKeys).toContain(expectedKey);
        }
      } finally {
        // Cleanup
        for (let i = 0; i < count; i++) {
          const key = `${prefix}item-${String(i).padStart(4, '0')}.txt`;
          try { await project.deleteObject(bucketName, key); } catch (_) { /* cleanup */ }
        }
      }
    }, 120000);
  });

  describe('Iterator-based Multipart Listing', () => {
    runTest('should list parts of a multipart upload using iterator', async () => {
      if (!project) throw new Error('Project not initialized');

      const key = `e2e-iter-parts-${Date.now()}.bin`;
      const mp = await beginMultipartUpload(
        project._nativeHandle,
        bucketName,
        key
      );

      try {
        // Upload 3 parts
        for (let partNum = 1; partNum <= 3; partNum++) {
          const part = await mp.uploadPart(partNum);
          const data = Buffer.alloc(256, partNum); // 256 bytes filled with partNum
          await part.write(data, data.length);
          await part.commit();
        }

        // List all parts — uses iterator internally
        const parts = await mp.listParts();
        expect(Array.isArray(parts)).toBe(true);
        expect(parts.length).toBe(3);

        // Verify part structure
        for (let i = 0; i < parts.length; i++) {
          expect(typeof parts[i].partNumber).toBe('number');
          expect(typeof parts[i].size).toBe('number');
          expect(parts[i].size).toBe(256);
        }
      } finally {
        try { await mp.abort(); } catch (_) { /* may already be committed */ }
        try { await project.deleteObject(bucketName, key); } catch (_) { /* cleanup */ }
      }
    }, 60000);

    runTest('should list pending multipart uploads using iterator', async () => {
      if (!project) throw new Error('Project not initialized');

      const prefix = `e2e-iter-mp-${Date.now()}/`;
      const uploads: MultipartUpload[] = [];

      try {
        // Begin several multipart uploads (don't commit — they stay "pending")
        for (let i = 0; i < 3; i++) {
          const key = `${prefix}upload-${i}.bin`;
          const mp = await beginMultipartUpload(project._nativeHandle, bucketName, key);
          uploads.push(mp);
        }

        // List pending uploads — uses iterator internally
        const pending: UploadInfo[] = await listMultipartUploads(project._nativeHandle, bucketName, {
          prefix,
          recursive: true,
        });

        expect(Array.isArray(pending)).toBe(true);
        expect(pending.length).toBe(3);

        // Verify upload info structure
        for (const info of pending) {
          expect(typeof info.uploadId).toBe('string');
          expect(info.uploadId.length).toBeGreaterThan(0);
          expect(typeof info.key).toBe('string');
          expect(info.key.startsWith(prefix)).toBe(true);
        }
      } finally {
        // Abort all pending uploads
        for (const mp of uploads) {
          try { await mp.abort(); } catch (_) { /* cleanup */ }
        }
      }
    }, 60000);
  });

  describe('Access Sharing with Listing', () => {
    runTest('should list objects with shared read-only access', async () => {
      if (!access || !project) throw new Error('Not initialized');

      const prefix = `e2e-shared-${Date.now()}/`;
      const keys = [
        `${prefix}shared-a.txt`,
        `${prefix}shared-b.txt`,
      ];

      // Upload with full access
      for (const key of keys) {
        const upload = await project.uploadObject(bucketName, key);
        const buf = Buffer.from(`shared-content-${key}`);
        await upload.write(buf, buf.length);
        await upload.commit();
      }

      try {
        // Create a read-only shared access scoped to this prefix
        const sharedAccess = await access.share(
          { allowDownload: true, allowUpload: false, allowList: true, allowDelete: false },
          [{ bucket: bucketName, prefix }]
        );

        const sharedProject = await sharedAccess.openProject();

        try {
          // List using the shared access
          const objects: ObjectInfo[] = await sharedProject.listObjects(bucketName, {
            prefix,
            recursive: true,
          });

          expect(objects.length).toBe(2);
          const objectKeys = objects.map((o: ObjectInfo) => o.key);
          for (const key of keys) {
            expect(objectKeys).toContain(key);
          }

          // Verify we can download but not upload
          const download = await sharedProject.downloadObject(bucketName, keys[0]);
          const buf = Buffer.alloc(1024);
          const result = await download.read(buf, buf.length);
          expect(result.bytesRead).toBeGreaterThan(0);
          await download.close();

          // Upload should fail with shared read-only access
          // Note: uploadObject() itself may succeed (creates handle),
          // but write+commit should fail with permission error
          let uploadFailed = false;
          try {
            const upload = await sharedProject.uploadObject(bucketName, `${prefix}unauthorized.txt`);
            const data = Buffer.from('should-not-be-allowed');
            await upload.write(data, data.length);
            await upload.commit();
          } catch (_) {
            uploadFailed = true;
          }
          expect(uploadFailed).toBe(true);
        } finally {
          await sharedProject.close();
        }
      } finally {
        // Cleanup with full access
        for (const key of keys) {
          try { await project.deleteObject(bucketName, key); } catch (_) { /* cleanup */ }
        }
      }
    }, 60000);
  });

  describe('Copy and Move with Listing Verification', () => {
    runTest('should copy + move objects and verify via listing', async () => {
      if (!project) throw new Error('Project not initialized');

      const prefix = `e2e-copymove-${Date.now()}/`;
      const srcKey = `${prefix}source.txt`;
      const copyKey = `${prefix}copied.txt`;
      const moveKey = `${prefix}moved.txt`;
      const content = Buffer.from('copy-and-move-test-data');

      // Upload source
      const upload = await project.uploadObject(bucketName, srcKey);
      await upload.write(content, content.length);
      await upload.commit();

      try {
        // Copy
        const copyInfo = await project.copyObject(bucketName, srcKey, bucketName, copyKey);
        expect(copyInfo.key).toBe(copyKey);

        // Verify both exist via listing
        let objects: ObjectInfo[] = await project.listObjects(bucketName, {
          prefix, recursive: true,
        });
        let objKeys = objects.map((o: ObjectInfo) => o.key);
        expect(objKeys).toContain(srcKey);
        expect(objKeys).toContain(copyKey);

        // Move the copy to a new location
        await project.moveObject(bucketName, copyKey, bucketName, moveKey);

        // Verify: source + moved exist, copy is gone
        objects = await project.listObjects(bucketName, {
          prefix, recursive: true,
        });
        objKeys = objects.map((o: ObjectInfo) => o.key);
        expect(objKeys).toContain(srcKey);
        expect(objKeys).toContain(moveKey);
        expect(objKeys).not.toContain(copyKey);

        // Download moved object and verify content integrity
        const download = await project.downloadObject(bucketName, moveKey);
        const buf = Buffer.alloc(content.length);
        const result = await download.read(buf, buf.length);
        await download.close();
        expect(buf.slice(0, result.bytesRead).toString()).toBe(content.toString());
      } finally {
        try { await project.deleteObject(bucketName, srcKey); } catch (_) { /* cleanup */ }
        try { await project.deleteObject(bucketName, copyKey); } catch (_) { /* cleanup */ }
        try { await project.deleteObject(bucketName, moveKey); } catch (_) { /* cleanup */ }
      }
    }, 60000);
  });

  describe('Concurrent Operations with Iterators', () => {
    runTest('should handle parallel uploads followed by listing', async () => {
      if (!project) throw new Error('Project not initialized');
      const proj = project;

      const prefix = `e2e-parallel-${Date.now()}/`;
      const count = 10;

      // Upload objects in parallel
      const uploadPromises = Array.from({ length: count }, async (_, i) => {
        const key = `${prefix}parallel-${String(i).padStart(3, '0')}.txt`;
        const upload = await proj.uploadObject(bucketName, key);
        const buf = Buffer.from(`parallel-data-${i}`);
        await upload.write(buf, buf.length);
        await upload.commit();
      });

      await Promise.all(uploadPromises);

      try {
        // List all — iterator should collect all items correctly
        const objects: ObjectInfo[] = await proj.listObjects(bucketName, {
          prefix,
          recursive: true,
        });
        expect(objects.length).toBe(count);
      } finally {
        // Cleanup
        for (let i = 0; i < count; i++) {
          const key = `${prefix}parallel-${String(i).padStart(3, '0')}.txt`;
          try { await proj.deleteObject(bucketName, key); } catch (_) { /* cleanup */ }
        }
      }
    }, 60000);
  });
});
