#!/usr/bin/env node
/**
 * Fallback postinstall for systems without Make.
 * Primarily for Windows users without MinGW/MSYS2.
 *
 * This script attempts to:
 *  1. Check if prebuilt binaries already exist (from npm package)
 *  2. If not, build the native addon with node-gyp
 *  3. Copy the built addon to native/prebuilds/<platform>/
 */

'use strict';

const { execSync } = require('child_process');
const fs = require('fs');
const path = require('path');

const projectDir = path.resolve(__dirname, '..');
const platform = `${process.platform}-${process.arch}`;
const prebuildDir = path.join(projectDir, 'native', 'prebuilds', platform);
const addonName = 'uplink_native.node';

// ─── .uplinkrc persistence ──────────────────────────────────────────────────
// Read/write saved install method so it survives rm -rf node_modules.
// INIT_CWD is set by npm/yarn/pnpm to the directory where the user ran install.
const RC_FILENAME = '.uplinkrc';
const rcDir = process.env.INIT_CWD || path.resolve(projectDir, '..', '..');
const rcPath = path.join(rcDir, RC_FILENAME);
const VALID_METHODS = ['prebuilt', 'hybrid', 'source', 'skip'];

function readRc() {
  try {
    const val = fs.readFileSync(rcPath, 'utf8').trim();
    return VALID_METHODS.includes(val) ? val : null;
  } catch { return null; }
}

function writeRc(method) {
  if (!VALID_METHODS.includes(method)) return;
  try {
    fs.writeFileSync(rcPath, method + '\n', 'utf8');
    console.log(`[uplink-nodejs] ✔ Saved install method "${method}" to ${rcPath}`);
  } catch { /* non-critical */ }
}

// Resolve UPLINK_INSTALL: env var > .uplinkrc > (none = auto)
let uplinkInstall = (process.env.UPLINK_INSTALL || '').trim().toLowerCase();
const fromEnv = !!uplinkInstall;
if (!uplinkInstall) {
  const saved = readRc();
  if (saved) {
    uplinkInstall = saved;
    console.log(`[uplink-nodejs] Using saved install method "${saved}" from ${rcPath}`);
  }
}

console.log('[uplink-nodejs] Fallback installer (Make not available)');
console.log(`  Platform: ${platform}`);
console.log(`  Method:   ${uplinkInstall || 'auto'}`);
console.log(`  Output:   ${prebuildDir}`);
console.log('');

// Handle skip method
if (uplinkInstall === 'skip') {
  console.log('[uplink-nodejs] Method: skip — skipping native build');
  if (fromEnv) writeRc('skip');
  process.exit(0);
}

// Step 1: Check if prebuilt addon already exists (shipped with npm package or already built)
const prebuiltAddon = path.join(prebuildDir, addonName);
if (fs.existsSync(prebuiltAddon)) {
  console.log(`✓ Prebuilt addon found at ${prebuiltAddon}`);
  console.log('  Skipping build — ready to use.');
  if (fromEnv) writeRc(uplinkInstall);
  process.exit(0);
}

// Step 2: Check if locally built addon exists from node-gyp
const buildAddon = path.join(projectDir, 'build', 'Release', addonName);
if (fs.existsSync(buildAddon)) {
  console.log(`✓ Built addon found at ${buildAddon}`);
  console.log('  Skipping build — ready to use.');
  process.exit(0);
}

// Step 3: Check if the uplink-c library exists (required for node-gyp build)
const libNames = {
  darwin: 'libuplink.dylib',
  linux: 'libuplink.so',
  win32: 'libuplink.dll',
};
const libName = libNames[process.platform] || 'libuplink.so';
const libPath = path.join(prebuildDir, libName);

if (!fs.existsSync(libPath)) {
  console.log(`⚠ uplink-c library not found at ${libPath}`);
  console.log('');
  console.log('The native uplink-c library is required to build the Node.js addon.');
  console.log('');
  console.log('To install, choose one of:');
  console.log('');
  console.log('  Option 1 — Source build (requires Go + C compiler):');
  console.log('    make install-source');
  console.log('');
  console.log('  Option 2 — Hybrid build (requires C compiler):');
  console.log('    make install-hybrid');
  console.log('');
  console.log('  Option 3 — Prebuilt (no compiler needed):');
  console.log('    make install-prebuilt');
  console.log('');
  console.log('See README.md for details.');
  // Don't exit(1) — allow npm install to succeed; the user can build later
  process.exit(0);
}

// Step 4: Try building with node-gyp
console.log('Building native addon with node-gyp...');
try {
  fs.mkdirSync(prebuildDir, { recursive: true });
  execSync('npx node-gyp rebuild', { cwd: projectDir, stdio: 'inherit' });

  // Copy built addon to prebuilds
  if (fs.existsSync(buildAddon)) {
    fs.copyFileSync(buildAddon, prebuiltAddon);
    console.log(`✓ Addon built and copied to ${prebuildDir}`);
  } else {
    console.log('✓ Addon built at build/Release/');
  }
} catch (err) {
  console.error('');
  console.error('⚠ node-gyp build failed:', err.message);
  console.error('');
  console.error('Try installing with Make:');
  console.error('  make install-hybrid   (requires C compiler)');
  console.error('  make install-prebuilt  (no compiler needed)');
  // Don't exit(1) — allow npm install to succeed
}
