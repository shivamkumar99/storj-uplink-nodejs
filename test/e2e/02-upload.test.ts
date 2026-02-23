/**
 * @file test/e2e/02-upload.test.ts
 * @brief E2E: Upload operations — simple, chunked, metadata, expiration, abort
 *
 * Uploads test objects that will be used by subsequent download/list tests.
 */

import { UploadResultStruct } from '../../src';
import { hasCredentials } from '../helpers/credentials';
import {
  getE2eContext,
  uploadTestObject,
  trackKey,
  type E2eContext,
} from './helpers/e2e-context';

describe('E2E: Upload Operations', () => {
  const runTest = hasCredentials ? it : it.skip;
  let ctx: E2eContext;

  beforeAll(async () => {
    if (!hasCredentials) {
      console.log('⏭️  Skipping E2E upload tests: No valid credentials configured');
      return;
    }
    ctx = await getE2eContext();
  });

  describe('Simple Upload', () => {
    runTest('should upload a small text object', async () => {
      const key = 'e2e-simple.txt';
      const content = Buffer.from('Hello, Storj! E2E simple upload test.');

      const upload = await ctx.project.uploadObject(ctx.bucketName, key);
      expect(upload).toBeInstanceOf(UploadResultStruct);

      const bytesWritten = await upload.write(content, content.length);
      expect(bytesWritten).toBe(content.length);

      await upload.commit();
      trackKey(key);

      // Verify via statObject
      const stat = await ctx.project.statObject(ctx.bucketName, key);
      expect(stat.key).toBe(key);
      expect(stat.system.contentLength).toBe(content.length);
    });
  });

  describe('Chunked Upload', () => {
    runTest('should upload content in multiple chunks', async () => {
      const key = 'e2e-chunked.txt';
      const fullContent = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789';
      const chunkSize = 10;

      const upload = await ctx.project.uploadObject(ctx.bucketName, key);

      let offset = 0;
      while (offset < fullContent.length) {
        const slice = fullContent.slice(offset, offset + chunkSize);
        const chunk = Buffer.from(slice);
        const written = await upload.write(chunk, chunk.length);
        expect(written).toBe(chunk.length);
        offset += chunkSize;
      }

      await upload.commit();
      trackKey(key);

      const stat = await ctx.project.statObject(ctx.bucketName, key);
      expect(stat.system.contentLength).toBe(fullContent.length);
    });
  });

  describe('Upload with Custom Metadata', () => {
    runTest('should upload with custom metadata', async () => {
      const key = 'e2e-metadata.txt';
      const content = Buffer.from('Content with custom metadata');
      const metadata = {
        'content-type': 'text/plain',
        'x-author': 'e2e-test',
        'x-version': '1.0',
      };

      await uploadTestObject(ctx, key, content, metadata);

      const stat = await ctx.project.statObject(ctx.bucketName, key);
      expect(stat.custom['content-type']).toBe('text/plain');
      expect(stat.custom['x-author']).toBe('e2e-test');
      expect(stat.custom['x-version']).toBe('1.0');
    });
  });

  describe('Upload with Expiration', () => {
    runTest('should upload with expiration date', async () => {
      const key = 'e2e-expires.txt';
      const content = Buffer.from('Expiring content');
      const expiresDate = new Date(Date.now() + 24 * 60 * 60 * 1000); // +24h

      const upload = await ctx.project.uploadObject(ctx.bucketName, key, {
        expires: expiresDate,
      });
      await upload.write(content, content.length);
      await upload.commit();
      trackKey(key);

      const stat = await ctx.project.statObject(ctx.bucketName, key);
      expect(stat.system.expires).toBeDefined();
      expect(stat.system.expires).not.toBeNull();

      if (stat.system.expires) {
        const diffSeconds = stat.system.expires - Math.floor(Date.now() / 1000);
        expect(diffSeconds).toBeGreaterThan(23 * 60 * 60);
        expect(diffSeconds).toBeLessThan(25 * 60 * 60);
      }
    });
  });

  describe('Upload Abort', () => {
    runTest('should abort an upload and verify object does not exist', async () => {
      const key = `e2e-abort-${Date.now()}.txt`;
      const content = Buffer.from('This upload will be aborted');

      const upload = await ctx.project.uploadObject(ctx.bucketName, key);
      await upload.write(content, content.length);
      await upload.abort();

      expect(upload.isActive).toBe(false);

      await expect(ctx.project.statObject(ctx.bucketName, key)).rejects.toThrow();
    });
  });

  describe('Upload Info', () => {
    runTest('should return object info before and after commit', async () => {
      const key = `e2e-upload-info-${Date.now()}.txt`;
      const content = Buffer.from('Info test content');

      const upload = await ctx.project.uploadObject(ctx.bucketName, key);

      // Info before commit
      const infoBefore = await upload.info();
      expect(infoBefore).toBeDefined();
      expect(infoBefore.key).toBe(key);

      await upload.write(content, content.length);
      await upload.commit();
      trackKey(key);

      // Verify via statObject
      const stat = await ctx.project.statObject(ctx.bucketName, key);
      expect(stat.key).toBe(key);
      expect(stat.system.contentLength).toBe(content.length);
    });
  });

  describe('Multiple File Upload', () => {
    runTest('should upload multiple files simulating a backup workflow', async () => {
      const files = [
        { name: 'e2e-docs/report.txt', content: 'Annual Report 2024' },
        { name: 'e2e-docs/notes.txt', content: 'Meeting notes' },
        { name: 'e2e-docs/sub/readme.txt', content: 'Nested file content' },
      ];

      for (const file of files) {
        await uploadTestObject(ctx, file.name, Buffer.from(file.content));
      }

      // Verify all exist
      for (const file of files) {
        const stat = await ctx.project.statObject(ctx.bucketName, file.name);
        expect(stat.key).toBe(file.name);
        expect(stat.system.contentLength).toBe(file.content.length);
      }
    });
  });
});
