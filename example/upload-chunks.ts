import { Uplink } from 'storj-uplink-nodejs';
import * as fs from 'fs';
import * as path from 'path';
import * as dotenv from 'dotenv';

dotenv.config({ path: path.resolve(__dirname, '../.env.test') });

const SATELLITE = process.env.TEST_SATELLITE;
const API_KEY = process.env.TEST_API_KEY;
const PASSPHRASE = process.env.TEST_PASSPHRASE;
const BUCKET = process.env.TEST_BUCKET || 'example-chunk-bucket';
const FILE_PATH = process.env.UPLOAD_FILE_PATH || path.resolve(__dirname, 'sample.txt');
const OBJECT_KEY = process.env.UPLOAD_OBJECT_KEY || 'chunked-upload.txt';

async function main() {
  if (!SATELLITE || !API_KEY || !PASSPHRASE) {
    throw new Error('Missing Storj credentials in environment');
  }
  const uplink = new Uplink();
  const access = await uplink.requestAccessWithPassphrase(SATELLITE, API_KEY, PASSPHRASE);
  const project = await access.openProject();
  await project.ensureBucket(BUCKET);

  const upload = await project.uploadObject(BUCKET, OBJECT_KEY);
  const stream = fs.createReadStream(FILE_PATH, { highWaterMark: 1024 * 1024 }); // 1MB chunks
  let total = 0;
  for await (const chunk of stream) {
    const written = await upload.write(chunk, chunk.length);
    total += written;
    console.log(`Uploaded chunk: ${written} bytes`);
  }
  await upload.commit();
  console.log(`Upload complete: ${OBJECT_KEY} (${total} bytes)`);
  await project.close();
}

main().catch(err => {
  console.error('Upload failed:', err);
  process.exit(1);
});