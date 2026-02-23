/**
 * @file test/e2e/helpers/e2e-context.ts
 * @brief Shared singleton context for E2E tests
 *
 * Provides a lazily-initialized context (uplink, access, project, bucket)
 * that is reused across all E2E test files when running with --runInBand.
 *
 * When running individual files, a fresh context is created per file.
 */

import {
  Uplink,
  AccessResultStruct,
  ProjectResultStruct,
} from '../../../src';
import {
  testSatellite,
  testApiKey,
  testPassphrase,
} from '../../helpers/credentials';

export interface E2eContext {
  uplink: Uplink;
  access: AccessResultStruct;
  project: ProjectResultStruct;
  bucketName: string;
  uploadedKeys: Set<string>;
}

const BUCKET_PREFIX = 'e2e';

let _ctx: E2eContext | null = null;
let _initializing: Promise<E2eContext> | null = null;

/**
 * Get or create the shared E2E context.
 *
 * The context is a singleton: the first call creates uplink → access →
 * project → bucket; subsequent calls return the cached instance.
 */
export async function getE2eContext(): Promise<E2eContext> {
  if (_ctx) return _ctx;

  if (_initializing) return _initializing;

  _initializing = (async (): Promise<E2eContext> => {
    if (!testSatellite || !testApiKey || !testPassphrase) {
      throw new Error(
        'E2E tests require TEST_SATELLITE, TEST_API_KEY, and TEST_PASSPHRASE in .env.test'
      );
    }

    const uplink = new Uplink();
    const access = await uplink.requestAccessWithPassphrase(
      testSatellite,
      testApiKey,
      testPassphrase,
    );
    const project = await access.openProject();
    const bucketName = `${BUCKET_PREFIX}-${Date.now()}`;

    await project.ensureBucket(bucketName);

    _ctx = {
      uplink,
      access,
      project,
      bucketName,
      uploadedKeys: new Set<string>(),
    };

    registerCleanupHook();

    return _ctx;
  })();

  return _initializing;
}

/**
 * Track an uploaded object key so other test files can reference it.
 */
export function trackKey(key: string): void {
  if (_ctx) {
    _ctx.uploadedKeys.add(key);
  }
}

/**
 * Remove a key from tracking (after deletion).
 */
export function untrackKey(key: string): void {
  if (_ctx) {
    _ctx.uploadedKeys.delete(key);
  }
}

/**
 * Upload a test object and track its key.
 */
export async function uploadTestObject(
  ctx: E2eContext,
  key: string,
  content: Buffer,
  metadata?: Record<string, string>,
): Promise<void> {
  const upload = await ctx.project.uploadObject(ctx.bucketName, key);
  if (metadata) {
    await upload.setCustomMetadata(metadata);
  }
  await upload.write(content, content.length);
  await upload.commit();
  trackKey(key);
}

/**
 * Delete a test object and untrack its key. Swallows errors.
 */
export async function deleteTestObject(
  ctx: E2eContext,
  key: string,
): Promise<void> {
  try {
    await ctx.project.deleteObject(ctx.bucketName, key);
  } catch (_) {
    // Object may not exist
  }
  untrackKey(key);
}

/**
 * Clean up all tracked objects, delete the bucket, and close the project.
 * Called from 09-cleanup.test.ts (or afterAll of an individual file).
 */
export async function cleanupE2eContext(): Promise<void> {
  if (!_ctx) return;

  const { project, bucketName, uploadedKeys } = _ctx;

  // Delete all tracked objects
  for (const key of uploadedKeys) {
    try {
      await project.deleteObject(bucketName, key);
    } catch (_) {
      // May already be deleted
    }
  }
  uploadedKeys.clear();

  // Also list any remaining objects (in case tracking missed some)
  try {
    const remaining = await project.listObjects(bucketName, { recursive: true });
    for (const obj of remaining) {
      try {
        await project.deleteObject(bucketName, obj.key);
      } catch (_) {
        // Ignore
      }
    }
  } catch (_) {
    // Bucket may not exist
  }

  // Delete the bucket
  try {
    await project.deleteBucket(bucketName);
  } catch (_) {
    // May already be deleted
  }

  // Close project
  await project.close();

  _ctx = null;
  _initializing = null;
}

/**
 * Register a process-level cleanup hook so that individual file runs
 * (without 09-cleanup.test.ts) still clean up the bucket and project.
 *
 * When running the full suite with --runInBand, 09-cleanup.test.ts handles
 * cleanup explicitly. This hook acts as a safety net.
 */
let _cleanupRegistered = false;

function registerCleanupHook(): void {
  if (_cleanupRegistered) return;
  _cleanupRegistered = true;

  afterAll(async () => {
    if (_ctx) {
      await cleanupE2eContext();
    }
  });
}
