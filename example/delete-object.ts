import { Uplink } from 'storj-uplink-nodejs';
import * as dotenv from 'dotenv';
import * as path from 'path';

dotenv.config({ path: path.resolve(__dirname, '../.env.test') });

const SATELLITE = process.env.TEST_SATELLITE;
const API_KEY = process.env.TEST_API_KEY;
const PASSPHRASE = process.env.TEST_PASSPHRASE;
const BUCKET = process.env.TEST_BUCKET || 'example-chunk-bucket';
const OBJECT_KEY = process.env.UPLOAD_OBJECT_KEY || 'chunked-upload.txt';

async function main() {
  if (!SATELLITE || !API_KEY || !PASSPHRASE) {
    throw new Error('Missing Storj credentials in environment');
  }
  const uplink = new Uplink();
  const access = await uplink.requestAccessWithPassphrase(SATELLITE, API_KEY, PASSPHRASE);
  const project = await access.openProject();

  await project.deleteObject(BUCKET, OBJECT_KEY);
  console.log(`Deleted object: ${OBJECT_KEY} from bucket: ${BUCKET}`);
  await project.close();
}

main().catch(err => {
  console.error('Object deletion failed:', err);
  process.exit(1);
});