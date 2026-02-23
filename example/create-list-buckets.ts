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

  const bucketNames = [
    `example-bucket-1-${Date.now()}`,
    `example-bucket-2-${Date.now()}`,
    `example-bucket-3-${Date.now()}`,
    `example-bucket-4-${Date.now()}`,
  ];

  for (const name of bucketNames) {
    await project.ensureBucket(name);
    console.log(`Created bucket: ${name}`);
  }

  const buckets = await project.listBuckets();
  console.log('Buckets:');
  for (const bucket of buckets) {
    console.log(`- ${bucket.name}`);
  }
  await project.close();
}

main().catch(err => {
  console.error('Bucket creation/listing failed:', err);
  process.exit(1);
});