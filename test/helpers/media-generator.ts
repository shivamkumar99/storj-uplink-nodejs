/**
 * @file test/helpers/media-generator.ts
 * @brief Generates real, playable media files (PNG images and AVI videos)
 *        for integration testing of chunked upload/download operations.
 *
 * All generated files are fully valid and can be opened in native players:
 *   - PNG: viewable in Preview, browsers, any image viewer
 *   - AVI: playable in QuickTime, VLC, mpv, ffplay (uncompressed RGB24)
 */

import * as zlib from 'zlib';

// ---------------------------------------------------------------------------
// PNG Generation
// ---------------------------------------------------------------------------

export interface PngOptions {
  /** Image width in pixels (default: 512) */
  width?: number;
  /** Image height in pixels (default: 512) */
  height?: number;
}

/**
 * Generates a valid, viewable PNG image (RGBA gradient).
 *
 * The image shows:
 *   - Red channel: left→right gradient
 *   - Green channel: top→bottom gradient
 *   - Blue channel: diagonal gradient
 *   - Alpha: fully opaque
 *
 * @returns A Buffer containing a complete, valid PNG file.
 */
export function generatePng(options: PngOptions = {}): Buffer {
  const width = options.width ?? 512;
  const height = options.height ?? 512;

  // Raw pixel data: each row has a filter byte (0 = None) + RGBA pixels
  const rawRowSize = 1 + width * 4;
  const rawData = Buffer.alloc(rawRowSize * height);

  for (let y = 0; y < height; y++) {
    const rowOffset = y * rawRowSize;
    rawData[rowOffset] = 0; // filter: None
    for (let x = 0; x < width; x++) {
      const px = rowOffset + 1 + x * 4;
      rawData[px + 0] = Math.floor((x / width) * 255);                        // R
      rawData[px + 1] = Math.floor((y / height) * 255);                       // G
      rawData[px + 2] = Math.floor(((x + y) / (width + height)) * 255);       // B
      rawData[px + 3] = 255;                                                   // A
    }
  }

  // Helper: build a PNG chunk  [length][type][data][CRC32]
  function pngChunk(type: string, data: Buffer): Buffer {
    const len = Buffer.alloc(4);
    len.writeUInt32BE(data.length, 0);
    const typeB = Buffer.from(type, 'ascii');
    const crcInput = Buffer.concat([typeB, data]);
    const crc = Buffer.alloc(4);
    crc.writeUInt32BE(zlib.crc32(crcInput) >>> 0, 0);
    return Buffer.concat([len, typeB, data, crc]);
  }

  // PNG Signature
  const signature = Buffer.from([137, 80, 78, 71, 13, 10, 26, 10]);

  // IHDR
  const ihdr = Buffer.alloc(13);
  ihdr.writeUInt32BE(width, 0);
  ihdr.writeUInt32BE(height, 4);
  ihdr.writeUInt8(8, 8);   // bit depth
  ihdr.writeUInt8(6, 9);   // color type: RGBA
  ihdr.writeUInt8(0, 10);  // compression
  ihdr.writeUInt8(0, 11);  // filter
  ihdr.writeUInt8(0, 12);  // interlace

  // IDAT
  const compressed = zlib.deflateSync(rawData);

  // Assemble
  const png = Buffer.concat([
    signature,
    pngChunk('IHDR', ihdr),
    pngChunk('IDAT', compressed),
    pngChunk('IEND', Buffer.alloc(0)),
  ]);

  console.log(`[media-generator] PNG: ${width}×${height} RGBA, ${png.length} bytes`);
  return png;
}

// ---------------------------------------------------------------------------
// AVI Generation  (uncompressed RGB24 — plays everywhere)
// ---------------------------------------------------------------------------

export interface AviOptions {
  /** Video width in pixels (default: 320) */
  width?: number;
  /** Video height in pixels (default: 240) */
  height?: number;
  /** Frames per second (default: 10) */
  fps?: number;
  /** Duration in seconds (default: 3) */
  durationSec?: number;
  /** Approximate target file size in bytes. If set, durationSec is
   *  computed automatically to reach this size. */
  targetSizeBytes?: number;
}

/**
 * Generates a valid, playable AVI video file (uncompressed RGB24).
 *
 * Each frame renders an animated colour pattern:
 *   - Background colour cycles through hues over time
 *   - A white rectangle "bounces" across the frame
 *
 * The AVI uses the simplest possible format:
 *   - RIFF/AVI container
 *   - Single video stream, codec = DIB (uncompressed)
 *   - AVI 1.0 index (idx1)
 *
 * This plays natively in QuickTime Player, VLC, mpv, and ffplay.
 *
 * @returns A Buffer containing a complete, playable AVI file.
 */
export function generateAvi(options: AviOptions = {}): Buffer {
  const width = options.width ?? 320;
  const height = options.height ?? 240;
  const fps = options.fps ?? 10;

  // Row stride must be padded to 4-byte boundary (BMP/AVI requirement)
  const rowStride = Math.ceil(width * 3 / 4) * 4;
  const frameDataSize = rowStride * height;

  // Calculate frame count
  let frameCount: number;
  if (options.targetSizeBytes) {
    // Estimate: total ≈ headers (~12KB) + frameCount*(8+frameDataSize) + idx1
    const overhead = 12000; // generous estimate for headers + idx1 per frame
    const perFrame = 8 + frameDataSize; // chunk header + pixel data
    frameCount = Math.max(1, Math.floor((options.targetSizeBytes - overhead) / (perFrame + 16)));
    // +16 per frame for idx1 entry
    console.log(
      `[media-generator] AVI: targeting ~${(options.targetSizeBytes / 1024 / 1024).toFixed(1)} MB → ` +
      `${frameCount} frames (${(frameCount / fps).toFixed(1)}s at ${fps} fps)`
    );
  } else {
    const durationSec = options.durationSec ?? 3;
    frameCount = fps * durationSec;
  }

  // ---- Generate frames ----
  const frames: Buffer[] = [];

  for (let f = 0; f < frameCount; f++) {
    const frame = Buffer.alloc(frameDataSize);
    const t = f / frameCount; // 0..1 progress

    // Background: cycling hue
    const bgR = Math.floor(127 + 127 * Math.sin(2 * Math.PI * t));
    const bgG = Math.floor(127 + 127 * Math.sin(2 * Math.PI * t + 2.094));
    const bgB = Math.floor(127 + 127 * Math.sin(2 * Math.PI * t + 4.189));

    // Bouncing rectangle position
    const rectW = Math.floor(width * 0.2);
    const rectH = Math.floor(height * 0.15);
    const rectX = Math.floor((width - rectW) * (0.5 + 0.5 * Math.sin(2 * Math.PI * t * 2)));
    const rectY = Math.floor((height - rectH) * (0.5 + 0.5 * Math.cos(2 * Math.PI * t * 3)));

    // BMP pixel data is stored bottom-up
    for (let y = 0; y < height; y++) {
      const bmpY = height - 1 - y; // flip vertically
      const rowOff = bmpY * rowStride;
      for (let x = 0; x < width; x++) {
        const inRect = x >= rectX && x < rectX + rectW && y >= rectY && y < rectY + rectH;
        const px = rowOff + x * 3;
        if (inRect) {
          frame[px + 0] = 255; // B
          frame[px + 1] = 255; // G
          frame[px + 2] = 255; // R
        } else {
          frame[px + 0] = bgB; // B (BMP is BGR)
          frame[px + 1] = bgG; // G
          frame[px + 2] = bgR; // R
        }
      }
    }
    frames.push(frame);
  }

  // ---- Build AVI file structure ----

  /** Write a 32-bit little-endian int at offset */
  function w32(buf: Buffer, offset: number, val: number): void {
    buf.writeUInt32LE(val, offset);
  }
  /** Write a 16-bit little-endian int at offset */
  function w16(buf: Buffer, offset: number, val: number): void {
    buf.writeUInt16LE(val, offset);
  }

  // --- avih (Main AVI Header) ---
  const avih = Buffer.alloc(56);
  w32(avih, 0, Math.floor(1000000 / fps)); // dwMicroSecPerFrame
  w32(avih, 4, frameDataSize * fps);       // dwMaxBytesPerSec
  w32(avih, 8, 0);                         // dwPaddingGranularity
  w32(avih, 12, 0x0110);                   // dwFlags: AVIF_HASINDEX | AVIF_ISINTERLEAVED
  w32(avih, 16, frameCount);               // dwTotalFrames
  w32(avih, 20, 0);                        // dwInitialFrames
  w32(avih, 24, 1);                        // dwStreams
  w32(avih, 28, frameDataSize);            // dwSuggestedBufferSize
  w32(avih, 32, width);                    // dwWidth
  w32(avih, 36, height);                   // dwHeight
  // dwReserved[4] = 0

  // --- strh (Stream Header) ---
  const strh = Buffer.alloc(56);
  strh.write('vids', 0, 4, 'ascii');       // fccType
  strh.write('DIB ', 4, 4, 'ascii');       // fccHandler (uncompressed)
  w32(strh, 8, 0);                         // dwFlags
  w16(strh, 12, 0);                        // wPriority
  w16(strh, 14, 0);                        // wLanguage
  w32(strh, 16, 0);                        // dwInitialFrames
  w32(strh, 20, 1);                        // dwScale
  w32(strh, 24, fps);                      // dwRate
  w32(strh, 28, 0);                        // dwStart
  w32(strh, 32, frameCount);               // dwLength
  w32(strh, 36, frameDataSize);            // dwSuggestedBufferSize
  w32(strh, 40, 0xFFFFFFFF);               // dwQuality (-1)
  w32(strh, 44, 0);                        // dwSampleSize
  w16(strh, 48, 0);                        // rcFrame left
  w16(strh, 50, 0);                        // rcFrame top
  w16(strh, 52, width);                    // rcFrame right
  w16(strh, 54, height);                   // rcFrame bottom

  // --- strf (Stream Format — BITMAPINFOHEADER) ---
  const strf = Buffer.alloc(40);
  w32(strf, 0, 40);                        // biSize
  strf.writeInt32LE(width, 4);             // biWidth
  strf.writeInt32LE(height, 8);            // biHeight (positive = bottom-up)
  w16(strf, 12, 1);                        // biPlanes
  w16(strf, 14, 24);                       // biBitCount (RGB24)
  w32(strf, 16, 0);                        // biCompression (BI_RGB)
  w32(strf, 20, frameDataSize);            // biSizeImage
  w32(strf, 24, 0);                        // biXPelsPerMeter
  w32(strf, 28, 0);                        // biYPelsPerMeter
  w32(strf, 32, 0);                        // biClrUsed
  w32(strf, 36, 0);                        // biClrImportant

  // --- Build chunks using RIFF helper ---
  function riffChunk(fourcc: string, data: Buffer): Buffer {
    const hdr = Buffer.alloc(8);
    hdr.write(fourcc, 0, 4, 'ascii');
    hdr.writeUInt32LE(data.length, 4);
    // AVI chunks must be 2-byte aligned; pad if odd
    const pad = data.length % 2 === 1 ? Buffer.alloc(1) : Buffer.alloc(0);
    return Buffer.concat([hdr, data, pad]);
  }

  function riffList(listType: string, children: Buffer[]): Buffer {
    const body = Buffer.concat([Buffer.from(listType, 'ascii'), ...children]);
    const hdr = Buffer.alloc(8);
    hdr.write('LIST', 0, 4, 'ascii');
    hdr.writeUInt32LE(body.length, 4);
    return Buffer.concat([hdr, body]);
  }

  // hdrl LIST
  const strhChunk = riffChunk('strh', strh);
  const strfChunk = riffChunk('strf', strf);
  const strl = riffList('strl', [strhChunk, strfChunk]);
  const avihChunk = riffChunk('avih', avih);
  const hdrl = riffList('hdrl', [avihChunk, strl]);

  // movi LIST (contains the actual frame data)
  const moviChildren: Buffer[] = [];
  for (const frame of frames) {
    moviChildren.push(riffChunk('00dc', frame)); // 00dc = stream 0, compressed video
  }
  const movi = riffList('movi', moviChildren);

  // idx1 (AVI 1.0 index)
  const idx1Entries = Buffer.alloc(frameCount * 16);
  let moviDataOffset = 4; // offset from start of 'movi' data (after 'movi' fourcc)
  for (let i = 0; i < frameCount; i++) {
    const entryOff = i * 16;
    idx1Entries.write('00dc', entryOff, 4, 'ascii');       // ckid
    idx1Entries.writeUInt32LE(0x10, entryOff + 4);         // dwFlags: AVIIF_KEYFRAME
    idx1Entries.writeUInt32LE(moviDataOffset, entryOff + 8);  // dwOffset
    idx1Entries.writeUInt32LE(frameDataSize, entryOff + 12);  // dwSize
    moviDataOffset += 8 + frameDataSize; // chunk header + data
    // Add padding byte if frameDataSize is odd
    if (frameDataSize % 2 === 1) moviDataOffset += 1;
  }
  const idx1Chunk = riffChunk('idx1', idx1Entries);

  // RIFF AVI container
  const riffBody = Buffer.concat([
    Buffer.from('AVI ', 'ascii'),
    hdrl,
    movi,
    idx1Chunk,
  ]);
  const riffHeader = Buffer.alloc(8);
  riffHeader.write('RIFF', 0, 4, 'ascii');
  riffHeader.writeUInt32LE(riffBody.length, 4);

  const avi = Buffer.concat([riffHeader, riffBody]);

  console.log(
    `[media-generator] AVI: ${width}×${height}, ${frameCount} frames @ ${fps} fps ` +
    `(${(frameCount / fps).toFixed(1)}s), ${avi.length} bytes ` +
    `(${(avi.length / 1024 / 1024).toFixed(2)} MB)`
  );

  return avi;
}
