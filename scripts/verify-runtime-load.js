#!/usr/bin/env node
/**
 * @file scripts/verify-runtime-load.js
 * @brief Prove that the built addon finds libuplink at runtime regardless of
 *        the process working directory, and (Windows) regardless of where the
 *        DLL lives when UPLINK_LIBRARY_PATH is set.
 *
 * Every check runs in a child process so a crash or a bad exit is caught.
 * The check exercises a real uplink-c call (parseAccess on a bogus grant),
 * which is what triggers the Windows delay-load of libuplink.dll.
 *
 * Usage: node scripts/verify-runtime-load.js [--published-addon]
 *   (after `npm run build:ts` and a native install)
 *
 * --published-addon: the addon binary was downloaded from a GitHub release and
 *   may predate the current loader, so only the layout-independent checks run
 *   (package root and foreign cwd). Drop the flag once the release that ships
 *   this loader is the published one.
 */
'use strict';

const { spawnSync } = require('node:child_process');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');

const publishedAddon = process.argv.includes('--published-addon');
const root = path.resolve(__dirname, '..');
const dist = path.join(root, 'dist');
const platform = `${process.platform}-${os.arch()}`;
const prebuildDir = path.join(root, 'native', 'prebuilds', platform);
const libName = { win32: 'libuplink.dll', darwin: 'libuplink.dylib' }[process.platform] ?? 'libuplink.so';
const libPath = path.join(prebuildDir, libName);
// node-gyp also drops a copy of the library next to the freshly built addon.
const libCopies = [libPath, path.join(root, 'build', 'Release', libName)].filter((p) => fs.existsSync(p));

// Calls into uplink-c and prints a marker only when the call reached the library.
const probe = `
  const { Uplink } = require(${JSON.stringify(dist)});
  new Uplink().parseAccess('not-a-real-access-grant')
    .then(() => { console.log('PROBE:unexpected-success'); })
    .catch((err) => { console.log('PROBE:error:' + (err && err.message)); });
`;

let failures = 0;
function check(name, { cwd, env, expectLoad }) {
  const res = spawnSync(process.execPath, ['-e', probe], {
    cwd,
    env: { ...process.env, UPLINK_LOG_LEVEL: 'error', ...env },
    encoding: 'utf8',
    timeout: 60_000,
  });
  const out = `${res.stdout}${res.stderr}`;
  const reachedLibrary = /PROBE:error:/.test(out) && !/PROBE:unexpected-success/.test(out);
  const cleanFailure = res.status !== 0 && /Failed to load uplink native module|libuplink/.test(out);
  const ok = expectLoad ? res.status === 0 && reachedLibrary : cleanFailure && res.signal === null;
  console.log(`${ok ? 'PASS' : 'FAIL'}  ${name}`);
  if (!ok) {
    failures += 1;
    console.log(`      exit=${res.status} signal=${res.signal}\n${out.split('\n').map((l) => '      | ' + l).join('\n')}`);
  }
}

if (!fs.existsSync(libPath)) {
  console.error(`missing ${libPath}: run a native install first`);
  process.exit(2);
}

// 1. Load from the package root (the layout every CI job already uses).
check('loads from the package root', { cwd: root, expectLoad: true });

// 2. Load from an unrelated working directory (how a consuming app runs).
const elsewhere = fs.mkdtempSync(path.join(os.tmpdir(), 'uplink-cwd-'));
check(`loads from an unrelated cwd (${elsewhere})`, { cwd: elsewhere, expectLoad: true });

if (publishedAddon) {
  console.log('--published-addon: skipping checks that need the current loader');
  fs.rmSync(elsewhere, { recursive: true, force: true });
  console.log(failures === 0 ? '\nruntime load verification: all checks passed' : `\n${failures} check(s) failed`);
  process.exit(failures === 0 ? 0 : 1);
}

// 3. UPLINK_LIBRARY_PATH pointing at the library file and at its directory.
check('UPLINK_LIBRARY_PATH=<file> is honoured', { cwd: elsewhere, env: { UPLINK_LIBRARY_PATH: libPath }, expectLoad: true });
check('UPLINK_LIBRARY_PATH=<dir> is honoured', { cwd: elsewhere, env: { UPLINK_LIBRARY_PATH: prebuildDir }, expectLoad: true });

// 4. Windows only: the DLL is not linked into the addon, so it can live anywhere.
//    On macOS/Linux the dynamic linker binds libuplink through rpath at .node load
//    time, so relocating the library there is not a supported layout.
if (process.platform === 'win32') {
  const moved = fs.mkdtempSync(path.join(os.tmpdir(), 'uplink-dll-'));
  const movedLib = path.join(moved, libName);
  // copy + delete rather than rename: the temp dir may be on another drive (EXDEV).
  // Every copy has to go, otherwise the PATH layer legitimately finds the other one.
  fs.copyFileSync(libPath, movedLib);
  for (const copy of libCopies) fs.rmSync(copy);
  try {
    check('relocated DLL: fails cleanly (no crash) without UPLINK_LIBRARY_PATH', { cwd: elsewhere, expectLoad: false });
    check('relocated DLL: UPLINK_LIBRARY_PATH=<file> loads it', { cwd: elsewhere, env: { UPLINK_LIBRARY_PATH: movedLib }, expectLoad: true });
    check('relocated DLL: UPLINK_LIBRARY_PATH=<dir> loads it', { cwd: elsewhere, env: { UPLINK_LIBRARY_PATH: moved }, expectLoad: true });
    check('relocated DLL: directory on PATH loads it', {
      cwd: elsewhere,
      env: { PATH: `${moved}${path.delimiter}${process.env.PATH ?? ''}` },
      expectLoad: true,
    });
  } finally {
    for (const copy of libCopies) fs.copyFileSync(movedLib, copy);
    fs.rmSync(moved, { recursive: true, force: true });
  }
}

fs.rmSync(elsewhere, { recursive: true, force: true });
console.log(failures === 0 ? '\nruntime load verification: all checks passed' : `\n${failures} check(s) failed`);
process.exit(failures === 0 ? 0 : 1);
