import { Uplink, beginMultipartUpload } from 'storj-uplink-nodejs';
import * as fs from 'fs';
import * as path from 'path';
import * as dotenv from 'dotenv';

dotenv.config({ path: path.resolve(__dirname, '../.env.test') });

const SATELLITE = process.env.TEST_SATELLITE;
const API_KEY = process.env.TEST_API_KEY;
const PASSPHRASE = process.env.TEST_PASSPHRASE;
const BUCKET = process.env.TEST_BUCKET || 'example-multipart-bucket';
const FILE_PATH = process.env.UPLOAD_FILE_PATH || path.resolve(__dirname, 'large.txt');
const OBJECT_KEY = process.env.UPLOAD_OBJECT_KEY || 'multipart-upload.txt';

async function main() {
  if (!SATELLITE || !API_KEY || !PASSPHRASE) {
    throw new Error('Missing Storj credentials in environment');
  }
  const uplink = new Uplink();
  const access = await uplink.requestAccessWithPassphrase(SATELLITE, API_KEY, PASSPHRASE);
  const project = await access.openProject();
  await project.ensureBucket(BUCKET);

  const mp = await beginMultipartUpload(project._nativeHandle, BUCKET, OBJECT_KEY);
  const fileSize = fs.statSync(FILE_PATH).size;
  const stream = fs.createReadStream(FILE_PATH, { highWaterMark: 5 * 1024 * 1024 }); // 5MB chunks
  let partNumber = 1;
  let total = 0;
  for await (const chunk of stream) {
    const part = await mp.uploadPart(partNumber);
    await part.write(chunk, chunk.length);
    await part.commit();
    console.log(`Uploaded part ${partNumber}: ${chunk.length} bytes`);
    total += chunk.length;
    partNumber++;
  }
  const info = await mp.commit();
  console.log(`Multipart upload complete: ${OBJECT_KEY} (${total} bytes)`);
  await project.close();
}

main().catch(err => {
  console.error('Multipart upload failed:', err);
  process.exit(1);
});