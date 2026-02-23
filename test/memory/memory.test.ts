/**
 * @file test/memory/memory.test.ts
 * @brief Comprehensive memory leak detection tests
 *
 * Each function is tested INDIVIDUALLY with configurable iterations to ensure
 * no memory growth. Functions are NOT tested collectively to produce
 * accurate, isolated results per function.
 *
 * Run with: npm run test:memory
 * Requires --expose-gc flag for accurate GC (configured in package.json)
 *
 * Requires either:
 * - TEST_ACCESS_GRANT environment variable, OR
 * - TEST_SATELLITE, TEST_API_KEY, and TEST_PASSPHRASE environment variables
 */

import * as fs from 'fs';
import * as path from 'path';
import { Uplink, AccessResultStruct } from '../../src';

// ============================================================================
// Configuration
// ============================================================================

const ITERATIONS = 3000;
const ALLOWED_GROWTH_MB = 50; // Max allowed heap growth across 3000 iterations
const GC_SETTLE_MS = 200; // Time to allow GC to settle
const TEST_DESC = `should not leak memory after ${ITERATIONS} iterations`;

// Directory for memory profiling CSV files
const RESULTS_DIR = path.join(__dirname, 'results');

// Ensure the results directory exists
if (!fs.existsSync(RESULTS_DIR)) {
  fs.mkdirSync(RESULTS_DIR, { recursive: true });
}

function _unused(_value: unknown): void {
  /* intentionally empty - suppresses unused variable warnings */
}

// ============================================================================
// Helpers
// ============================================================================

function hasValidCredentials(): boolean {
  const testGrant = process.env.TEST_ACCESS_GRANT;
  const hasValidGrant = !!(
    testGrant &&
    testGrant !== 'your_access_grant_here' &&
    testGrant.length > 50
  );

  const satellite = process.env.TEST_SATELLITE;
  const apiKey = process.env.TEST_API_KEY;
  const passphrase = process.env.TEST_PASSPHRASE;
  const hasValidSatelliteCredentials = !!(
    satellite &&
    satellite.includes('@') &&
    apiKey &&
    apiKey.length > 20 &&
    passphrase &&
    passphrase !== 'your_passphrase_here'
  );

  return hasValidGrant || hasValidSatelliteCredentials;
}

async function getAccess(uplink: Uplink): Promise<AccessResultStruct> {
  const testGrant = process.env.TEST_ACCESS_GRANT;
  const hasValidGrant = !!(
    testGrant &&
    testGrant !== 'your_access_grant_here' &&
    testGrant.length > 50
  );

  if (hasValidGrant && testGrant) {
    return uplink.parseAccess(testGrant);
  }

  const satellite = process.env.TEST_SATELLITE || '';
  const apiKey = process.env.TEST_API_KEY || '';
  const passphrase = process.env.TEST_PASSPHRASE || '';

  return uplink.requestAccessWithPassphrase(satellite, apiKey, passphrase);
}

function forceGC(): void {
  if (global.gc) {
    global.gc();
    global.gc(); // Double GC to collect weak refs
  }
}

function getHeapUsedMB(): number {
  forceGC();
  return process.memoryUsage().heapUsed / (1024 * 1024);
}

async function settleMemory(): Promise<void> {
  forceGC();
  await new Promise((resolve) => setTimeout(resolve, GC_SETTLE_MS));
  forceGC();
}

/**
 * Sanitize a function name into a safe filename.
 */
function toFileName(name: string): string {
  return name.replace(/[^a-zA-Z0-9_-]/g, '_').replace(/_+/g, '_');
}

/**
 * Write per-iteration memory snapshots to a CSV file.
 */
function writeMemoryCSV(
  name: string,
  baselineMB: number,
  snapshots: Array<{ iteration: number; heapUsedMB: number }>
): void {
  const fileName = `${toFileName(name)}.csv`;
  const filePath = path.join(RESULTS_DIR, fileName);
  const lines = [
    'iteration,heapUsedMB,growthFromBaselineMB',
    `0,${baselineMB.toFixed(4)},0.0000`,
    ...snapshots.map(
      (s) => `${s.iteration},${s.heapUsedMB.toFixed(4)},${(s.heapUsedMB - baselineMB).toFixed(4)}`
    ),
  ];
  fs.writeFileSync(filePath, lines.join('\n') + '\n', 'utf-8');
}

/**
 * Run a memory test for a single function in isolation.
 * Warmup phase + measured phase with 100 iterations.
 * Writes per-iteration heap snapshots to CSV for graphing.
 */
async function measureMemoryGrowth(
  name: string,
  fn: (iteration: number) => Promise<void>,
  iterations: number = ITERATIONS
): Promise<{ baselineMB: number; finalMB: number; growthMB: number }> {
  // Warmup: run a few iterations to stabilize JIT and caches
  for (let i = 0; i < 10; i++) {
    await fn(i);
  }

  await settleMemory();
  const baselineMB = getHeapUsedMB();

  // Collect per-iteration memory snapshots
  const snapshots: Array<{ iteration: number; heapUsedMB: number }> = [];

  // Measured phase
  for (let i = 0; i < iterations; i++) {
    await fn(i);

    // Record heap at every 10th iteration
    if ((i + 1) % 10 === 0) {
      const heapUsedMB = process.memoryUsage().heapUsed / (1024 * 1024);
      snapshots.push({ iteration: i + 1, heapUsedMB });
    }

    // Periodic GC every 25 iterations to avoid false positives from GC deferral
    if (i > 0 && i % 25 === 0) {
      forceGC();
    }
  }

  await settleMemory();
  const finalMB = getHeapUsedMB();
  const growthMB = finalMB - baselineMB;

  // Write CSV for graphing
  writeMemoryCSV(name, baselineMB, snapshots);

  console.log(
    `    ${name}: baseline=${baselineMB.toFixed(2)}MB, ` +
      `final=${finalMB.toFixed(2)}MB, growth=${growthMB.toFixed(2)}MB ` +
      `(${iterations} iterations) → ${toFileName(name)}.csv`
  );

  return { baselineMB, finalMB, growthMB };
}

// ============================================================================
// Test Suite
// ============================================================================

describe(`Memory Leak Tests - Individual Function Isolation (${ITERATIONS} iterations)`, () => {
  const validCredentials = hasValidCredentials();
  const runTest = validCredentials ? it : it.skip;

  let uplink: Uplink;
  let access: AccessResultStruct;
  let serializedGrant: string;

  beforeAll(async () => {
    if (!validCredentials) {
      console.log('⏭️  Skipping memory tests: No valid credentials configured');
      console.log('   Set TEST_ACCESS_GRANT or (TEST_SATELLITE + TEST_API_KEY + TEST_PASSPHRASE)');
      return;
    }

    // Pre-setup shared resources
    uplink = new Uplink();
    access = await getAccess(uplink);
    serializedGrant = await access.serialize();

    console.log(`\n  Running memory tests with ${ITERATIONS} iterations each...`);
    console.log(`  Allowed growth: ${ALLOWED_GROWTH_MB}MB per function\n`);
  });

  // ==========================================================================
  // 1. Uplink.parseAccess
  // ==========================================================================
  describe('Uplink.parseAccess', () => {
    runTest(
      TEST_DESC,
      async () => {
        const { growthMB } = await measureMemoryGrowth('parseAccess', async () => {
          const a = await uplink.parseAccess(serializedGrant);
          // Let access go out of scope
          _unused(a);
        });

        expect(growthMB).toBeLessThan(ALLOWED_GROWTH_MB);
      },
      300000
    );
  });

  // ==========================================================================
  // 2. Uplink.requestAccessWithPassphrase
  // ==========================================================================
  describe('Uplink.requestAccessWithPassphrase', () => {
    runTest(
      TEST_DESC,
      async () => {
        const satellite = process.env.TEST_SATELLITE;
        const apiKey = process.env.TEST_API_KEY;
        const passphrase = process.env.TEST_PASSPHRASE;

        // This test only runs if satellite credentials are available
        // (parseAccess test covers the grant-based path)
        if (!satellite || !apiKey || !passphrase) {
          console.log('    Skipping: requires TEST_SATELLITE credentials');
          return;
        }

        const { growthMB } = await measureMemoryGrowth('requestAccessWithPassphrase', async () => {
          const a = await uplink.requestAccessWithPassphrase(satellite, apiKey, passphrase);
          _unused(a);
        });

        expect(growthMB).toBeLessThan(ALLOWED_GROWTH_MB);
      },
      600000
    );
  });

  // ==========================================================================
  // 3. Access.serialize
  // ==========================================================================
  describe('Access.serialize', () => {
    runTest(
      TEST_DESC,
      async () => {
        const { growthMB } = await measureMemoryGrowth('access.serialize', async () => {
          const s = await access.serialize();
          _unused(s);
        });

        expect(growthMB).toBeLessThan(ALLOWED_GROWTH_MB);
      },
      300000
    );
  });

  // ==========================================================================
  // 4. Access.satelliteAddress
  // ==========================================================================
  describe('Access.satelliteAddress', () => {
    runTest(
      TEST_DESC,
      async () => {
        const { growthMB } = await measureMemoryGrowth('access.satelliteAddress', async () => {
          const addr = await access.satelliteAddress();
          _unused(addr);
        });

        expect(growthMB).toBeLessThan(ALLOWED_GROWTH_MB);
      },
      300000
    );
  });

  // ==========================================================================
  // 5. Access.share
  // ==========================================================================
  describe('Access.share', () => {
    runTest(
      TEST_DESC,
      async () => {
        const { growthMB } = await measureMemoryGrowth('access.share', async () => {
          const shared = await access.share({ allowDownload: true, allowList: true }, [
            { bucket: 'test-bucket' },
          ]);
          _unused(shared);
        });

        expect(growthMB).toBeLessThan(ALLOWED_GROWTH_MB);
      },
      300000
    );
  });

  // ==========================================================================
  // 6. Uplink.deriveEncryptionKey
  // ==========================================================================
  describe('Uplink.deriveEncryptionKey', () => {
    runTest(
      TEST_DESC,
      async () => {
        const salt = Buffer.from('test-salt-for-memory-testing');

        const { growthMB } = await measureMemoryGrowth('uplinkDeriveEncryptionKey', async () => {
          const key = await uplink.uplinkDeriveEncryptionKey('test-passphrase', salt);
          _unused(key);
        });

        expect(growthMB).toBeLessThan(ALLOWED_GROWTH_MB);
      },
      300000
    );
  });

  // ==========================================================================
  // 7. Access.openProject + Project.close
  // ==========================================================================
  describe('Access.openProject + Project.close', () => {
    runTest(
      TEST_DESC,
      async () => {
        const { growthMB } = await measureMemoryGrowth('openProject/close', async () => {
          const project = await access.openProject();
          await project.close();
        });

        expect(growthMB).toBeLessThan(ALLOWED_GROWTH_MB);
      },
      600000
    );
  });

  // ==========================================================================
  // 8. Project.ensureBucket (idempotent - safe to repeat)
  // ==========================================================================
  describe('Project.ensureBucket', () => {
    runTest(
      TEST_DESC,
      async () => {
        const project = await access.openProject();
        const bucketName = `mem-test-ensure-${Date.now()}`;

        try {
          const { growthMB } = await measureMemoryGrowth('ensureBucket', async () => {
            const result = await project.ensureBucket(bucketName);
            _unused(result);
          });

          expect(growthMB).toBeLessThan(ALLOWED_GROWTH_MB);
        } finally {
          try {
            await project.deleteBucket(bucketName);
          } catch {
            /* cleanup best-effort */
          }
          await project.close();
        }
      },
      600000
    );
  });

  // ==========================================================================
  // 9. Project.statBucket
  // ==========================================================================
  describe('Project.statBucket', () => {
    runTest(
      TEST_DESC,
      async () => {
        const project = await access.openProject();
        const bucketName = `mem-test-stat-${Date.now()}`;

        try {
          await project.ensureBucket(bucketName);

          const { growthMB } = await measureMemoryGrowth('statBucket', async () => {
            const result = await project.statBucket(bucketName);
            _unused(result);
          });

          expect(growthMB).toBeLessThan(ALLOWED_GROWTH_MB);
        } finally {
          try {
            await project.deleteBucket(bucketName);
          } catch {
            /* cleanup best-effort */
          }
          await project.close();
        }
      },
      600000
    );
  });

  // ==========================================================================
  // 10. Project.listBuckets
  // ==========================================================================
  describe('Project.listBuckets', () => {
    runTest(
      TEST_DESC,
      async () => {
        const project = await access.openProject();

        try {
          const { growthMB } = await measureMemoryGrowth('listBuckets', async () => {
            const buckets = await project.listBuckets();
            _unused(buckets);
          });

          expect(growthMB).toBeLessThan(ALLOWED_GROWTH_MB);
        } finally {
          await project.close();
        }
      },
      600000
    );
  });

  // ==========================================================================
  // 11. Project.createBucket + deleteBucket cycle
  // ==========================================================================
  describe('Project.createBucket + deleteBucket', () => {
    runTest(
      TEST_DESC,
      async () => {
        const project = await access.openProject();

        try {
          const { growthMB } = await measureMemoryGrowth('createBucket/deleteBucket', async (i) => {
            const name = `mem-test-cd-${Date.now()}-${i}`;
            await project.createBucket(name);
            await project.deleteBucket(name);
          });

          expect(growthMB).toBeLessThan(ALLOWED_GROWTH_MB);
        } finally {
          await project.close();
        }
      },
      600000
    );
  });

  // ==========================================================================
  // 12. Upload: uploadObject + write + commit
  // ==========================================================================
  describe('Upload: uploadObject + write + commit', () => {
    runTest(
      TEST_DESC,
      async () => {
        const project = await access.openProject();
        const bucketName = `mem-test-upload-${Date.now()}`;

        try {
          await project.ensureBucket(bucketName);
          const testData = Buffer.from('memory-test-data-payload-for-upload');

          const { growthMB } = await measureMemoryGrowth('upload/write/commit', async (i) => {
            const upload = await project.uploadObject(bucketName, `mem-obj-${i}`);
            await upload.write(testData, testData.length);
            await upload.commit();
          });

          expect(growthMB).toBeLessThan(ALLOWED_GROWTH_MB);
        } finally {
          try {
            await project.deleteBucketWithObjects(bucketName);
          } catch {
            /* cleanup best-effort */
          }
          await project.close();
        }
      },
      600000
    );
  });

  // ==========================================================================
  // 13. Upload: uploadObject + abort
  // ==========================================================================
  describe('Upload: uploadObject + abort', () => {
    runTest(
      TEST_DESC,
      async () => {
        const project = await access.openProject();
        const bucketName = `mem-test-abort-${Date.now()}`;

        try {
          await project.ensureBucket(bucketName);
          const testData = Buffer.from('abort-test-data');

          const { growthMB } = await measureMemoryGrowth('upload/write/abort', async (i) => {
            const upload = await project.uploadObject(bucketName, `mem-abort-${i}`);
            await upload.write(testData, testData.length);
            await upload.abort();
          });

          expect(growthMB).toBeLessThan(ALLOWED_GROWTH_MB);
        } finally {
          try {
            await project.deleteBucketWithObjects(bucketName);
          } catch {
            /* cleanup best-effort */
          }
          await project.close();
        }
      },
      600000
    );
  });

  // ==========================================================================
  // 14. Download: downloadObject + read + close
  // ==========================================================================
  describe('Download: downloadObject + read + close', () => {
    runTest(
      TEST_DESC,
      async () => {
        const project = await access.openProject();
        const bucketName = `mem-test-download-${Date.now()}`;
        const objectKey = 'mem-download-obj';

        try {
          await project.ensureBucket(bucketName);

          // Upload a test object once
          const testData = Buffer.from('download-memory-test-data-payload');
          const upload = await project.uploadObject(bucketName, objectKey);
          await upload.write(testData, testData.length);
          await upload.commit();

          const readBuffer = Buffer.alloc(1024);

          const { growthMB } = await measureMemoryGrowth('download/read/close', async () => {
            const download = await project.downloadObject(bucketName, objectKey);
            try {
              await download.read(readBuffer, readBuffer.length);
            } catch {
              // EOF expected — buffer is larger than data
            }
            await download.close();
          });

          expect(growthMB).toBeLessThan(ALLOWED_GROWTH_MB);
        } finally {
          try {
            await project.deleteBucketWithObjects(bucketName);
          } catch {
            /* cleanup best-effort */
          }
          await project.close();
        }
      },
      600000
    );
  });

  // ==========================================================================
  // 15. Download.info
  // ==========================================================================
  describe('Download.info', () => {
    runTest(
      TEST_DESC,
      async () => {
        const project = await access.openProject();
        const bucketName = `mem-test-dlinfo-${Date.now()}`;
        const objectKey = 'mem-dlinfo-obj';

        try {
          await project.ensureBucket(bucketName);

          const testData = Buffer.from('dlinfo-test-data');
          const upload = await project.uploadObject(bucketName, objectKey);
          await upload.write(testData, testData.length);
          await upload.commit();

          const { growthMB } = await measureMemoryGrowth('download.info', async () => {
            const download = await project.downloadObject(bucketName, objectKey);
            const info = await download.info();
            _unused(info);
            await download.close();
          });

          expect(growthMB).toBeLessThan(ALLOWED_GROWTH_MB);
        } finally {
          try {
            await project.deleteBucketWithObjects(bucketName);
          } catch {
            /* cleanup best-effort */
          }
          await project.close();
        }
      },
      600000
    );
  });

  // ==========================================================================
  // 16. Project.statObject
  // ==========================================================================
  describe('Project.statObject', () => {
    runTest(
      TEST_DESC,
      async () => {
        const project = await access.openProject();
        const bucketName = `mem-test-statobj-${Date.now()}`;
        const objectKey = 'mem-stat-obj';

        try {
          await project.ensureBucket(bucketName);

          const testData = Buffer.from('statobj-test-data');
          const upload = await project.uploadObject(bucketName, objectKey);
          await upload.write(testData, testData.length);
          await upload.commit();

          const { growthMB } = await measureMemoryGrowth('statObject', async () => {
            const info = await project.statObject(bucketName, objectKey);
            _unused(info);
          });

          expect(growthMB).toBeLessThan(ALLOWED_GROWTH_MB);
        } finally {
          try {
            await project.deleteBucketWithObjects(bucketName);
          } catch {
            /* cleanup best-effort */
          }
          await project.close();
        }
      },
      600000
    );
  });

  // ==========================================================================
  // 17. Project.listObjects
  // ==========================================================================
  describe('Project.listObjects', () => {
    runTest(
      TEST_DESC,
      async () => {
        const project = await access.openProject();
        const bucketName = `mem-test-listobj-${Date.now()}`;

        try {
          await project.ensureBucket(bucketName);

          // Upload a few objects to list
          for (let i = 0; i < 5; i++) {
            const data = Buffer.from(`list-obj-data-${i}`);
            const upload = await project.uploadObject(bucketName, `list-obj-${i}`);
            await upload.write(data, data.length);
            await upload.commit();
          }

          const { growthMB } = await measureMemoryGrowth('listObjects', async () => {
            const objects = await project.listObjects(bucketName);
            _unused(objects);
          });

          expect(growthMB).toBeLessThan(ALLOWED_GROWTH_MB);
        } finally {
          try {
            await project.deleteBucketWithObjects(bucketName);
          } catch {
            /* cleanup best-effort */
          }
          await project.close();
        }
      },
      600000
    );
  });

  // ==========================================================================
  // 18. Project.deleteObject
  // ==========================================================================
  describe('Project.deleteObject', () => {
    runTest(
      TEST_DESC,
      async () => {
        const project = await access.openProject();
        const bucketName = `mem-test-delobj-${Date.now()}`;

        try {
          await project.ensureBucket(bucketName);

          const { growthMB } = await measureMemoryGrowth('upload+deleteObject', async (i) => {
            // Upload then delete in each iteration
            const data = Buffer.from('del-test');
            const upload = await project.uploadObject(bucketName, `del-obj-${i}`);
            await upload.write(data, data.length);
            await upload.commit();
            await project.deleteObject(bucketName, `del-obj-${i}`);
          });

          expect(growthMB).toBeLessThan(ALLOWED_GROWTH_MB);
        } finally {
          try {
            await project.deleteBucketWithObjects(bucketName);
          } catch {
            /* cleanup best-effort */
          }
          await project.close();
        }
      },
      600000
    );
  });

  // ==========================================================================
  // 19. Project.copyObject
  // ==========================================================================
  describe('Project.copyObject', () => {
    runTest(
      TEST_DESC,
      async () => {
        const project = await access.openProject();
        const bucketName = `mem-test-copy-${Date.now()}`;
        const srcKey = 'mem-copy-src';

        try {
          await project.ensureBucket(bucketName);

          // Upload source object once
          const data = Buffer.from('copy-source-data');
          const upload = await project.uploadObject(bucketName, srcKey);
          await upload.write(data, data.length);
          await upload.commit();

          const { growthMB } = await measureMemoryGrowth('copyObject', async (i) => {
            const destKey = `mem-copy-dest-${i}`;
            const result = await project.copyObject(bucketName, srcKey, bucketName, destKey);
            _unused(result);
          });

          expect(growthMB).toBeLessThan(ALLOWED_GROWTH_MB);
        } finally {
          try {
            await project.deleteBucketWithObjects(bucketName);
          } catch {
            /* cleanup best-effort */
          }
          await project.close();
        }
      },
      600000
    );
  });

  // ==========================================================================
  // 20. Project.moveObject
  // ==========================================================================
  describe('Project.moveObject', () => {
    runTest(
      TEST_DESC,
      async () => {
        const project = await access.openProject();
        const bucketName = `mem-test-move-${Date.now()}`;

        try {
          await project.ensureBucket(bucketName);

          const { growthMB } = await measureMemoryGrowth('moveObject', async (i) => {
            // Upload then move in each iteration
            const data = Buffer.from('move-test-data');
            const upload = await project.uploadObject(bucketName, `move-src-${i}`);
            await upload.write(data, data.length);
            await upload.commit();
            await project.moveObject(bucketName, `move-src-${i}`, bucketName, `move-dest-${i}`);
          });

          expect(growthMB).toBeLessThan(ALLOWED_GROWTH_MB);
        } finally {
          try {
            await project.deleteBucketWithObjects(bucketName);
          } catch {
            /* cleanup best-effort */
          }
          await project.close();
        }
      },
      600000
    );
  });

  // ==========================================================================
  // 21. Project.updateObjectMetadata
  // ==========================================================================
  describe('Project.updateObjectMetadata', () => {
    runTest(
      TEST_DESC,
      async () => {
        const project = await access.openProject();
        const bucketName = `mem-test-meta-${Date.now()}`;
        const objectKey = 'mem-meta-obj';

        try {
          await project.ensureBucket(bucketName);

          const data = Buffer.from('metadata-test-data');
          const upload = await project.uploadObject(bucketName, objectKey);
          await upload.write(data, data.length);
          await upload.commit();

          const { growthMB } = await measureMemoryGrowth('updateObjectMetadata', async (i) => {
            await project.updateObjectMetadata(bucketName, objectKey, {
              'x-test-key': `value-${i}`,
              'x-iteration': String(i),
            });
          });

          expect(growthMB).toBeLessThan(ALLOWED_GROWTH_MB);
        } finally {
          try {
            await project.deleteBucketWithObjects(bucketName);
          } catch {
            /* cleanup best-effort */
          }
          await project.close();
        }
      },
      600000
    );
  });

  // ==========================================================================
  // 22. Upload.info
  // ==========================================================================
  describe('Upload.info', () => {
    runTest(
      TEST_DESC,
      async () => {
        const project = await access.openProject();
        const bucketName = `mem-test-upinfo-${Date.now()}`;

        try {
          await project.ensureBucket(bucketName);

          const { growthMB } = await measureMemoryGrowth('upload.info', async (i) => {
            const upload = await project.uploadObject(bucketName, `upinfo-${i}`);
            const data = Buffer.from('info-test');
            await upload.write(data, data.length);
            const info = await upload.info();
            _unused(info);
            await upload.commit();
          });

          expect(growthMB).toBeLessThan(ALLOWED_GROWTH_MB);
        } finally {
          try {
            await project.deleteBucketWithObjects(bucketName);
          } catch {
            /* cleanup best-effort */
          }
          await project.close();
        }
      },
      600000
    );
  });

  // ==========================================================================
  // 23. Upload.setCustomMetadata
  // ==========================================================================
  describe('Upload.setCustomMetadata', () => {
    runTest(
      TEST_DESC,
      async () => {
        const project = await access.openProject();
        const bucketName = `mem-test-custmeta-${Date.now()}`;

        try {
          await project.ensureBucket(bucketName);

          const { growthMB } = await measureMemoryGrowth('upload.setCustomMetadata', async (i) => {
            const upload = await project.uploadObject(bucketName, `custmeta-${i}`);
            await upload.setCustomMetadata({
              'Content-Type': 'application/octet-stream',
              'X-Test': `iteration-${i}`,
            });
            const data = Buffer.from('custom-meta-data');
            await upload.write(data, data.length);
            await upload.commit();
          });

          expect(growthMB).toBeLessThan(ALLOWED_GROWTH_MB);
        } finally {
          try {
            await project.deleteBucketWithObjects(bucketName);
          } catch {
            /* cleanup best-effort */
          }
          await project.close();
        }
      },
      600000
    );
  });

  // ==========================================================================
  // 24. Project.deleteBucketWithObjects
  // ==========================================================================
  describe('Project.deleteBucketWithObjects', () => {
    runTest(
      TEST_DESC,
      async () => {
        const project = await access.openProject();

        try {
          const { growthMB } = await measureMemoryGrowth('deleteBucketWithObjects', async (i) => {
            const name = `mem-test-dbwo-${Date.now()}-${i}`;
            await project.ensureBucket(name);
            // Upload a small object to make it non-empty
            const data = Buffer.from('data');
            const upload = await project.uploadObject(name, 'obj');
            await upload.write(data, data.length);
            await upload.commit();
            await project.deleteBucketWithObjects(name);
          });

          expect(growthMB).toBeLessThan(ALLOWED_GROWTH_MB);
        } finally {
          await project.close();
        }
      },
      600000
    );
  });

  // ==========================================================================
  // 25. Buffer operations (no credentials needed)
  // ==========================================================================
  describe('Buffer operations', () => {
    it(`should not leak memory on repeated 1KB buffer allocations over ${ITERATIONS} iterations`, async () => {
      const bufferSize = 1024; // 1KB

      const { growthMB } = await measureMemoryGrowth('Buffer.alloc(1KB)', async (i) => {
        const buffer = Buffer.alloc(bufferSize);
        buffer.fill(i % 256);
        _unused(buffer);
      });

      expect(growthMB).toBeLessThan(ALLOWED_GROWTH_MB);
    }, 60000);

    it(`should not leak memory on repeated 1MB buffer allocations over ${ITERATIONS} iterations`, async () => {
      const bufferSize = 1024 * 1024; // 1MB

      const { growthMB } = await measureMemoryGrowth('Buffer.alloc(1MB)', async (i) => {
        const buffer = Buffer.alloc(bufferSize);
        buffer.fill(i % 256);
        _unused(buffer);
      });

      expect(growthMB).toBeLessThan(ALLOWED_GROWTH_MB);
    }, 120000);
  });

  // ==========================================================================
  // 26. Access.overrideEncryptionKey
  // ==========================================================================
  describe('Access.overrideEncryptionKey', () => {
    runTest(
      TEST_DESC,
      async () => {
        const salt = Buffer.from('override-enc-key-salt');

        const { growthMB } = await measureMemoryGrowth('overrideEncryptionKey', async (i) => {
          // Create fresh access each time since override mutates state
          const freshAccess = await uplink.parseAccess(serializedGrant);
          const encKey = await uplink.uplinkDeriveEncryptionKey(`passphrase-${i}`, salt);
          await freshAccess.overrideEncryptionKey('test-bucket', `prefix-${i}/`, encKey);
          _unused(freshAccess);
        });

        expect(growthMB).toBeLessThan(ALLOWED_GROWTH_MB);
      },
      600000
    );
  });

  // ==========================================================================
  // 27. configOpenProject (with custom config)
  // ==========================================================================
  describe('Access.configOpenProject', () => {
    runTest(
      TEST_DESC,
      async () => {
        const { growthMB } = await measureMemoryGrowth('configOpenProject/close', async () => {
          const project = await access.configOpenProject({
            dialTimeoutMilliseconds: 30000,
          });
          await project.close();
        });

        expect(growthMB).toBeLessThan(ALLOWED_GROWTH_MB);
      },
      600000
    );
  });
});

// Note: Run with --expose-gc flag for accurate GC:
// node --expose-gc node_modules/.bin/jest test/memory/memory.test.ts --runInBand
