import { Uplink } from 'storj-uplink-nodejs';
import * as fs from 'fs';
import * as path from 'path';
import * as dotenv from 'dotenv';

dotenv.config({ path: path.resolve(__dirname, '../.env.test') });

const SATELLITE = process.env.TEST_SATELLITE;
const API_KEY = process.env.TEST_API_KEY;
const PASSPHRASE = process.env.TEST_PASSPHRASE;
const BUCKET = process.env.TEST_BUCKET || 'example-multipart-bucket';
const OBJECT_KEY = process.env.UPLOAD_OBJECT_KEY || 'multipart-upload.txt';
const DOWNLOAD_PATH = process.env.DOWNLOAD_FILE_PATH || path.resolve(__dirname, 'downloaded-multipart.txt');

async function main() {
  if (!SATELLITE || !API_KEY || !PASSPHRASE) {
    throw new Error('Missing Storj credentials in environment');
  }
  const uplink = new Uplink();
  const access = await uplink.requestAccessWithPassphrase(SATELLITE, API_KEY, PASSPHRASE);
  const project = await access.openProject();

  const download = await project.downloadObject(BUCKET, OBJECT_KEY);
  const info = await download.info();
  const size = info.system.contentLength;
  const buffer = Buffer.alloc(size);
  const result = await download.read(buffer, size);
  await download.close();

  fs.writeFileSync(DOWNLOAD_PATH, buffer);
  console.log(`Downloaded multipart ${OBJECT_KEY} to ${DOWNLOAD_PATH} (${result.bytesRead} bytes)`);
  await project.close();
}

main().catch(err => {
  console.error('Multipart download failed:', err);
  process.exit(1);
});