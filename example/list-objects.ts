import { Uplink } from 'storj-uplink-nodejs';
import * as dotenv from 'dotenv';
import * as path from 'path';

dotenv.config({ path: path.resolve(__dirname, '../.env.test') });

const SATELLITE = process.env.TEST_SATELLITE;
const API_KEY = process.env.TEST_API_KEY;
const PASSPHRASE = process.env.TEST_PASSPHRASE;
const BUCKET = process.env.TEST_BUCKET || 'example-chunk-bucket';

async function main() {
  if (!SATELLITE || !API_KEY || !PASSPHRASE) {
    throw new Error('Missing Storj credentials in environment');
  }
  const uplink = new Uplink();
  const access = await uplink.requestAccessWithPassphrase(SATELLITE, API_KEY, PASSPHRASE);
  const project = await access.openProject();

  const objects = await project.listObjects(BUCKET, { recursive: true });
  console.log(`Objects in bucket '${BUCKET}':`);
  for (const obj of objects) {
    console.log(`- ${obj.key} (${obj.system.contentLength} bytes)`);
  }
  await project.close();
}

main().catch(err => {
  console.error('Object listing failed:', err);
  process.exit(1);
});