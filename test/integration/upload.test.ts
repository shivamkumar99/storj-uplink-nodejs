/**
 * @file test/integration/upload.test.ts
 * @brief Integration tests for upload operations (abort, info, expires)
 *
 * Tests the chains:
 *   1. uploadObject → write → abort → statObject(throws)
 *   2. uploadObject → write → commit → info()
 *   3. uploadObject → info() (pre-commit)
 *   4. uploadObject → setCustomMetadata → write → commit → statObject
 *   5. uploadObject({expires}) → write → commit → statObject
 *
 * Requires environment variables:
 * - TEST_ACCESS_GRANT: Pre-serialized access grant, OR
 * - TEST_SATELLITE, TEST_API_KEY, TEST_PASSPHRASE
 */

import {
  Uplink,
  AccessResultStruct,
  ProjectResultStruct,
  UploadResultStruct,
} from '../../src';
import * as fs from 'fs';
import * as path from 'path';
import { generatePng, generateAvi } from '../helpers/media-generator';
import { hasAnyCredentials, getAccess } from '../helpers/credentials';

describe('Integration: Upload Operations', () => {
  const validCredentials = hasAnyCredentials;
  const runTest = validCredentials ? it : it.skip;
  const bucketName = `int-up-${Date.now()}`;

  let uplink: Uplink;
  let access: AccessResultStruct;
  let project: ProjectResultStruct;

  beforeAll(async () => {
    if (!validCredentials) {
      console.log('⏭️  Skipping upload integration tests: No valid credentials configured');
      return;
    }

    const passphrase = process.env.TEST_PASSPHRASE;
    console.log(`🔑 Encryption passphrase: "${passphrase ?? '(using access grant)'}"`);

    uplink = new Uplink();
    access = await getAccess(uplink);
    project = await access.openProject();
    await project.ensureBucket(bucketName);
  });

  afterAll(async () => {
    if (!validCredentials) return;

    try {
      const objects = await project.listObjects(bucketName, { recursive: true });
      for (const obj of objects) {
        await project.deleteObject(bucketName, obj.key);
      }
      await project.deleteBucket(bucketName);
    } catch (_) {
      // bucket may not exist
    }
    console.log(`🪣 Bucket "${bucketName}" cleaned up`);
    await project.close();
  });

  runTest('should abort an upload and verify object does not exist', async () => {
    const key = `abort-test-${Date.now()}.txt`;
    const content = Buffer.from('This upload will be aborted');

    const upload = await project.uploadObject(bucketName, key);
    expect(upload).toBeInstanceOf(UploadResultStruct);

    await upload.write(content, content.length);
    await upload.abort();

    expect(upload.isActive).toBe(false);

    // Object should not exist after abort
    await expect(project.statObject(bucketName, key)).rejects.toThrow();
  });

  runTest('should return object info after commit via upload.info()', async () => {
    const key = `info-post-commit-${Date.now()}.txt`;
    const content = Buffer.from('Content for info test after commit');

    const upload = await project.uploadObject(bucketName, key);
    await upload.write(content, content.length);

    // info() before commit should work
    const infoBefore = await upload.info();
    console.log('📄 upload.info() before commit:', JSON.stringify(infoBefore, null, 2));
    expect(infoBefore).toBeDefined();
    expect(infoBefore.key).toBe(key);

    await upload.commit();

    // Verify via statObject
    const stat = await project.statObject(bucketName, key);
    console.log('📄 statObject() after commit:', JSON.stringify(stat, null, 2));
    expect(stat.key).toBe(key);
    expect(stat.system.contentLength).toBe(content.length);

    await project.deleteObject(bucketName, key);
  });

  runTest('should return object info before commit via upload.info()', async () => {
    const key = `info-pre-commit-${Date.now()}.txt`;

    const upload = await project.uploadObject(bucketName, key);

    const info = await upload.info();
    expect(info).toBeDefined();
    expect(info.key).toBe(key);

    // Abort since we never intend to commit
    await upload.abort();
  });

  runTest('should set metadata before write then verify via statObject', async () => {
    const key = `meta-order-${Date.now()}.txt`;
    const content = Buffer.from('Content with pre-set metadata');
    const metadata = {
      'x-test-key': 'test-value',
      'x-author': 'integration-test',
    };

    const upload = await project.uploadObject(bucketName, key);

    // Set metadata before write
    await upload.setCustomMetadata(metadata);
    await upload.write(content, content.length);
    await upload.commit();

    const stat = await project.statObject(bucketName, key);
    expect(stat.custom).toBeDefined();
    expect(stat.custom['x-test-key']).toBe('test-value');
    expect(stat.custom['x-author']).toBe('integration-test');

    await project.deleteObject(bucketName, key);
  });

  runTest('should upload with expiration option', async () => {
    const key = `expires-${Date.now()}.txt`;
    const content = Buffer.from('Content with expiration');
    const expiresDate = new Date(Date.now() + 24 * 60 * 60 * 1000); // +24 hours

    const upload = await project.uploadObject(bucketName, key, {
      expires: expiresDate,
    });
    await upload.write(content, content.length);
    await upload.commit();

    const stat = await project.statObject(bucketName, key);
    expect(stat.key).toBe(key);
    expect(stat.system.expires).toBeDefined();
    expect(stat.system.expires).not.toBeNull();

    if (stat.system.expires) {
      // Expires should be approximately 24h from now (within 5 min tolerance)
      // stat.system.expires is a Unix timestamp in seconds
      const diffSeconds = stat.system.expires - Math.floor(Date.now() / 1000);
      expect(diffSeconds).toBeGreaterThan(23 * 60 * 60);
      expect(diffSeconds).toBeLessThan(25 * 60 * 60);
    }

    await project.deleteObject(bucketName, key);
  }, 60000);

  runTest('should upload content in multiple chunks and verify reassembled data', async () => {
    const key = `chunked-upload-${Date.now()}.txt`;
    const fullContent = 'ABCDEFGHIJKLMNOPQRSTUVWXY'; // 25 characters
    const chunkSize = 10;

    const upload = await project.uploadObject(bucketName, key);

    // Write in chunks of 10: "ABCDEFGHIJ", "KLMNOPQRST", "UVWXY"
    let offset = 0;
    let chunkNum = 0;
    while (offset < fullContent.length) {
      const slice = fullContent.slice(offset, offset + chunkSize);
      const chunk = Buffer.from(slice);
      const bytesWritten = await upload.write(chunk, chunk.length);
      chunkNum++;
      console.log(
        `[Chunked] Chunk #${chunkNum}: "${slice}" (${chunk.length} bytes, written=${bytesWritten})`
      );
      expect(bytesWritten).toBe(chunk.length);
      offset += chunkSize;
    }

    expect(chunkNum).toBe(3); // 10 + 10 + 5

    await upload.commit();

    // Verify via statObject
    const stat = await project.statObject(bucketName, key);
    expect(stat.key).toBe(key);
    expect(stat.system.contentLength).toBe(fullContent.length);

    // Download and verify full content in chunks of 10
    const download = await project.downloadObject(bucketName, key);
    const receivedBuf = Buffer.alloc(fullContent.length);
    let totalRead = 0;
    let readChunkNum = 0;

    while (totalRead < fullContent.length) {
      const remaining = fullContent.length - totalRead;
      const toRead = Math.min(chunkSize, remaining);
      const readBuf = Buffer.alloc(toRead);
      try {
        const result = await download.read(readBuf, toRead);
        readChunkNum++;
        const text = readBuf.slice(0, result.bytesRead).toString('utf-8');
        console.log(
          `[Chunked-Read] Chunk #${readChunkNum}: "${text}" (${result.bytesRead} bytes, total=${totalRead + result.bytesRead}/${fullContent.length})`
        );
        readBuf.copy(receivedBuf, totalRead, 0, result.bytesRead);
        totalRead += result.bytesRead;
      } catch (err: unknown) {
        const errObj = err as Record<string, unknown>;
        const partialBytes = typeof errObj.bytesRead === 'number' ? errObj.bytesRead : 0;
        if (partialBytes > 0) {
          readBuf.copy(receivedBuf, totalRead, 0, partialBytes);
          totalRead += partialBytes;
        }
        console.log(`[Chunked-Read] EOF at total=${totalRead}`);
        break;
      }
    }
    await download.close();

    expect(totalRead).toBe(fullContent.length);
    expect(receivedBuf.toString('utf-8')).toBe(fullContent);

    await project.deleteObject(bucketName, key);
  }, 60000);

  runTest('should upload and download a 1MB PNG image in chunks', async () => {
    const key = `image-chunked-${Date.now()}.png`;
    const chunkSize = 64 * 1024; // 64KB chunks

    // Generate a valid, viewable PNG using the helper
    const imageBuffer = generatePng({ width: 512, height: 512 });

    console.log(`[PNG] Generated ${imageBuffer.length} byte valid PNG`);

    // Save uploaded image to disk
    const outputDir = path.join(__dirname, '..', 'output');
    fs.mkdirSync(outputDir, { recursive: true });
    const uploadedPath = path.join(outputDir, 'uploaded.png');
    fs.writeFileSync(uploadedPath, imageBuffer);
    console.log(`[PNG] Saved uploaded image → ${uploadedPath}`);

    // Verify PNG signature
    expect(imageBuffer[0]).toBe(137);
    expect(imageBuffer.slice(1, 4).toString('ascii')).toBe('PNG');

    // ---- Upload in chunks ----
    const upload = await project.uploadObject(bucketName, key);
    await upload.setCustomMetadata({ 'content-type': 'image/png' });

    let uploadOffset = 0;
    let uploadChunkNum = 0;
    while (uploadOffset < imageBuffer.length) {
      const remaining = imageBuffer.length - uploadOffset;
      const toWrite = Math.min(chunkSize, remaining);
      const chunk = imageBuffer.slice(uploadOffset, uploadOffset + toWrite);
      const bytesWritten = await upload.write(chunk, chunk.length);
      uploadChunkNum++;
      if (uploadChunkNum <= 3 || uploadChunkNum % 5 === 0) {
        console.log(
          `[PNG-Upload] Chunk #${uploadChunkNum}: ${bytesWritten} bytes (total=${uploadOffset + bytesWritten}/${imageBuffer.length})`
        );
      }
      expect(bytesWritten).toBe(toWrite);
      uploadOffset += bytesWritten;
    }

    console.log(`[PNG-Upload] Done: ${uploadChunkNum} chunks, ${uploadOffset} bytes total`);
    await upload.commit();

    // Verify via statObject
    const stat = await project.statObject(bucketName, key);
    expect(stat.key).toBe(key);
    expect(stat.system.contentLength).toBe(imageBuffer.length);
    expect(stat.custom['content-type']).toBe('image/png');
    console.log(`[PNG] statObject: contentLength=${stat.system.contentLength}`);

    // ---- Download in chunks ----
    const download = await project.downloadObject(bucketName, key);
    const downloadSize = imageBuffer.length;
    const receivedBuf = Buffer.alloc(downloadSize);
    let downloadOffset = 0;
    let downloadChunkNum = 0;
    let zeroRetries = 0;
    const MAX_ZERO_RETRIES = 100;

    while (downloadOffset < downloadSize) {
      const remaining = downloadSize - downloadOffset;
      const toRead = Math.min(chunkSize, remaining);
      const readBuf = Buffer.alloc(toRead);
      try {
        const result = await download.read(readBuf, toRead);
        if (result.bytesRead > 0) {
          readBuf.copy(receivedBuf, downloadOffset, 0, result.bytesRead);
          downloadOffset += result.bytesRead;
          downloadChunkNum++;
          zeroRetries = 0;
          if (downloadChunkNum <= 3 || downloadChunkNum % 10 === 0) {
            console.log(
              `[PNG-Download] Chunk #${downloadChunkNum}: ${result.bytesRead} bytes (total=${downloadOffset}/${downloadSize})`
            );
          }
        } else {
          zeroRetries++;
          if (zeroRetries >= MAX_ZERO_RETRIES) break;
        }
      } catch (err: unknown) {
        const errObj = err as Record<string, unknown>;
        const partialBytes = typeof errObj.bytesRead === 'number' ? errObj.bytesRead : 0;
        if (partialBytes > 0) {
          readBuf.copy(receivedBuf, downloadOffset, 0, partialBytes);
          downloadOffset += partialBytes;
        }
        console.log(`[PNG-Download] EOF at total=${downloadOffset}`);
        break;
      }
    }
    await download.close();

    console.log(`[PNG-Download] Done: ${downloadChunkNum} chunks, ${downloadOffset} bytes total`);

    // Save downloaded image to disk
    const downloadedPath = path.join(outputDir, 'downloaded.png');
    fs.writeFileSync(downloadedPath, receivedBuf.slice(0, downloadOffset));
    console.log(`[PNG] Saved downloaded image → ${downloadedPath}`);

    // Verify size
    expect(downloadOffset).toBe(downloadSize);

    // Verify data integrity: downloaded bytes must match uploaded bytes exactly
    expect(receivedBuf.equals(imageBuffer)).toBe(true);

    // Verify PNG signature is intact after round-trip
    expect(receivedBuf[0]).toBe(137);
    expect(receivedBuf.slice(1, 4).toString('ascii')).toBe('PNG');

    console.log(`\n🖼️  Open the images to compare:`);
    console.log(`   Uploaded:   open "${uploadedPath}"`);
    console.log(`   Downloaded: open "${downloadedPath}"`);

    await project.deleteObject(bucketName, key);
  }, 120000);

  /**
   * Test 8: Upload and download a ~5MB AVI video file in chunks.
   *
   * Generates a valid, playable AVI video (uncompressed RGB24) with an
   * animated colour-cycling background and a bouncing white rectangle.
   * Plays natively in QuickTime Player, VLC, mpv, and ffplay.
   *
   * The file is saved to disk before upload, read from disk in chunks
   * for upload, then downloaded in chunks and written to a separate file.
   */
  runTest('should upload and download a ~5MB AVI video file in chunks', async () => {
    const key = `video-${Date.now()}.avi`;
    const chunkSize = 256 * 1024; // 256 KB chunks

    // Generate a valid, playable AVI targeting ~5 MB
    const videoBuffer = generateAvi({
      width: 320,
      height: 240,
      fps: 10,
      targetSizeBytes: 5 * 1024 * 1024,
    });

    console.log(`[AVI] Generated ${videoBuffer.length} bytes (${(videoBuffer.length / 1024 / 1024).toFixed(2)} MB)`);

    // Verify RIFF/AVI signature
    expect(videoBuffer.slice(0, 4).toString('ascii')).toBe('RIFF');
    expect(videoBuffer.slice(8, 12).toString('ascii')).toBe('AVI ');

    // Save original video to disk
    const outputDir = path.join(__dirname, '..', 'output');
    fs.mkdirSync(outputDir, { recursive: true });
    const uploadedPath = path.join(outputDir, 'uploaded.avi');
    fs.writeFileSync(uploadedPath, videoBuffer);
    console.log(`[AVI] Saved original → ${uploadedPath}`);

    // ---- Upload in chunks (read file from disk in chunks) ----
    const upload = await project.uploadObject(bucketName, key);
    await upload.setCustomMetadata({ 'content-type': 'video/avi' });

    const fd = fs.openSync(uploadedPath, 'r');
    let uploadOffset = 0;
    let uploadChunkNum = 0;
    const readChunkBuf = Buffer.alloc(chunkSize);

    while (uploadOffset < videoBuffer.length) {
      const bytesReadFromFile = fs.readSync(fd, readChunkBuf, 0, chunkSize, uploadOffset);
      if (bytesReadFromFile === 0) break;
      const chunk = readChunkBuf.slice(0, bytesReadFromFile);
      const bytesWritten = await upload.write(chunk, chunk.length);
      uploadChunkNum++;
      if (uploadChunkNum <= 3 || uploadChunkNum % 5 === 0) {
        console.log(
          `[AVI-Upload] Chunk #${uploadChunkNum}: ${bytesWritten} bytes ` +
          `(total=${uploadOffset + bytesWritten}/${videoBuffer.length})`
        );
      }
      expect(bytesWritten).toBe(bytesReadFromFile);
      uploadOffset += bytesWritten;
    }
    fs.closeSync(fd);

    console.log(`[AVI-Upload] Done: ${uploadChunkNum} chunks, ${uploadOffset} bytes total`);
    await upload.commit();

    // Verify via statObject
    const stat = await project.statObject(bucketName, key);
    expect(stat.key).toBe(key);
    expect(stat.system.contentLength).toBe(videoBuffer.length);
    expect(stat.custom['content-type']).toBe('video/avi');
    console.log(`[AVI] statObject: contentLength=${stat.system.contentLength}`);

    // ---- Download in chunks and write to disk ----
    const download = await project.downloadObject(bucketName, key);
    const downloadSize = videoBuffer.length;
    const downloadedPath = path.join(outputDir, 'downloaded.avi');
    const wfd = fs.openSync(downloadedPath, 'w');
    let downloadOffset = 0;
    let downloadChunkNum = 0;
    let zeroRetries = 0;
    const MAX_ZERO_RETRIES = 100;

    while (downloadOffset < downloadSize) {
      const remaining = downloadSize - downloadOffset;
      const toRead = Math.min(chunkSize, remaining);
      const readBuf = Buffer.alloc(toRead);
      try {
        const result = await download.read(readBuf, toRead);
        if (result.bytesRead > 0) {
          fs.writeSync(wfd, readBuf, 0, result.bytesRead);
          downloadOffset += result.bytesRead;
          downloadChunkNum++;
          zeroRetries = 0;
          if (downloadChunkNum <= 3 || downloadChunkNum % 10 === 0) {
            console.log(
              `[AVI-Download] Chunk #${downloadChunkNum}: ${result.bytesRead} bytes ` +
              `(total=${downloadOffset}/${downloadSize})`
            );
          }
        } else {
          zeroRetries++;
          if (zeroRetries >= MAX_ZERO_RETRIES) break;
        }
      } catch (err: unknown) {
        const errObj = err as Record<string, unknown>;
        const partialBytes = typeof errObj.bytesRead === 'number' ? errObj.bytesRead : 0;
        if (partialBytes > 0) {
          fs.writeSync(wfd, readBuf, 0, partialBytes);
          downloadOffset += partialBytes;
        }
        console.log(`[AVI-Download] EOF at total=${downloadOffset}`);
        break;
      }
    }
    fs.closeSync(wfd);
    await download.close();

    console.log(`[AVI-Download] Done: ${downloadChunkNum} chunks, ${downloadOffset} bytes total`);

    // Verify size
    expect(downloadOffset).toBe(downloadSize);

    // Read back both files and compare byte-for-byte
    const originalFile = fs.readFileSync(uploadedPath);
    const downloadedFile = fs.readFileSync(downloadedPath);
    expect(downloadedFile.length).toBe(originalFile.length);
    expect(downloadedFile.equals(originalFile)).toBe(true);

    // Verify RIFF/AVI signature is intact after round-trip
    expect(downloadedFile.slice(0, 4).toString('ascii')).toBe('RIFF');
    expect(downloadedFile.slice(8, 12).toString('ascii')).toBe('AVI ');

    console.log(`\n🎬  Open the videos to compare:`);
    console.log(`   Uploaded:   open "${uploadedPath}"`);
    console.log(`   Downloaded: open "${downloadedPath}"`);

    await project.deleteObject(bucketName, key);
  }, 300000); // 5 min timeout for ~5MB upload+download
});
