import { Uplink } from 'storj-uplink-nodejs';
import * as dotenv from 'dotenv';
import * as path from 'path';

dotenv.config({ path: path.resolve(__dirname, '../.env.test') });

const SATELLITE = process.env.TEST_SATELLITE;
const API_KEY = process.env.TEST_API_KEY;
const PASSPHRASE = process.env.TEST_PASSPHRASE;

async function main() {
  if (!SATELLITE || !API_KEY || !PASSPHRASE) {
    throw new Error('Missing Storj credentials in environment');
  }
  const uplink = new Uplink();
  const access = await uplink.requestAccessWithPassphrase(SATELLITE, API_KEY, PASSPHRASE);
  const project = await access.openProject();

  const buckets = await project.listBuckets();
  let deleted = 0;
  for (const bucket of buckets) {
    if (bucket.name.startsWith('example-bucket-')) {
      await project.deleteBucket(bucket.name);
      console.log(`Deleted bucket: ${bucket.name}`);
      deleted++;
      if (deleted >= 4) break;
    }
  }
  await project.close();
}

main().catch(err => {
  console.error('Bucket deletion failed:', err);
  process.exit(1);
});