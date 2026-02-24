#!/usr/bin/env node
/**
 * @file scripts/cppcheck.js
 * @brief Run cppcheck with auto-detected system and Node.js include paths.
 *
 * Solves the "Include file not found" warnings by detecting:
 *   - Node.js N-API headers (node_api.h)
 *   - macOS SDK system headers (stdlib.h, string.h, etc.)
 *   - Linux system headers (/usr/include)
 *   - Project-local headers (native/include)
 *
 * Usage:
 *   node scripts/cppcheck.js          # normal run (error-exitcode=1)
 *   node scripts/cppcheck.js --xml    # XML output for reports
 */

const { execSync, spawnSync } = require('child_process');
const fs = require('fs');
const path = require('path');
const os = require('os');

const ROOT = path.resolve(__dirname, '..');

/**
 * Try to find the Node.js header directory containing node_api.h
 */
function findNodeInclude() {
    const candidates = [];

    // 1. node-gyp cache: ~/.node-gyp/<version>/include/node
    const nodeGypDir = path.join(
        os.homedir(),
        '.node-gyp',
        process.version.slice(1), // strip the 'v'
        'include',
        'node'
    );
    candidates.push(nodeGypDir);

    // 2. node_prefix/include/node (works on most installs)
    try {
        const prefix = process.config.variables.node_prefix;
        if (prefix) {
            candidates.push(path.join(prefix, 'include', 'node'));
        }
    } catch (_) { /* ignore */ }

    // 3. Common system locations
    candidates.push('/usr/local/include/node');
    candidates.push('/usr/include/node');

    for (const dir of candidates) {
        if (fs.existsSync(path.join(dir, 'node_api.h'))) {
            return dir;
        }
    }
    return null;
}

/**
 * Try to find the system SDK include path (macOS needs xcrun)
 */
function findSystemInclude() {
    // Linux: /usr/include usually works out of the box
    if (fs.existsSync('/usr/include/stdlib.h')) {
        return '/usr/include';
    }

    // macOS: headers are inside the SDK
    if (process.platform === 'darwin') {
        try {
            const sdkPath = execSync('xcrun --show-sdk-path', { encoding: 'utf8' }).trim();
            const inc = path.join(sdkPath, 'usr', 'include');
            if (fs.existsSync(path.join(inc, 'stdlib.h'))) {
                return inc;
            }
        } catch (_) { /* xcrun not available */ }
    }

    return null;
}

function main() {
    const isXml = process.argv.includes('--xml');

    // Build include path flags
    const includePaths = [];

    // Project-local headers (uplink.h, uplink_definitions.h)
    const localInclude = path.join(ROOT, 'native', 'include');
    if (fs.existsSync(localInclude)) {
        includePaths.push(localInclude);
    }

    // Node.js headers (node_api.h)
    const nodeInclude = findNodeInclude();
    if (nodeInclude) {
        includePaths.push(nodeInclude);
        console.log(`[cppcheck] Node.js headers: ${nodeInclude}`);
    } else {
        console.warn('[cppcheck] Warning: node_api.h not found — run "node-gyp install" to download headers');
    }

    // System headers (stdlib.h, string.h, etc.)
    const sysInclude = findSystemInclude();
    if (sysInclude) {
        includePaths.push(sysInclude);
        console.log(`[cppcheck] System headers:  ${sysInclude}`);
    } else {
        console.warn('[cppcheck] Warning: system headers not found');
    }

    // Build the cppcheck command
    const args = [
        '--enable=all',
        '--inline-suppr',
        '--suppress=unusedFunction',        // N-API exports are called from JS, not C
        '--suppress=constVariablePointer',  // Suggests `type* const p` (const on the pointer
        '--suppress=constParameterPointer', // itself, not what it points to) — uncommon in C
                                            // and adds clutter with minimal safety benefit
        '--suppress=unmatchedSuppression',  // Guard against cppcheck version drift: apt on
                                            // ubuntu-latest ships 2.13.0 which lacks some checks
                                            // (e.g. staticFunction added in 2.17). Unmatched
                                            // suppressions would otherwise exit 1 via checkersReport.
    ];

    if (isXml) {
        args.push('--xml', '--xml-version=2');
    } else {
        args.push('--error-exitcode=1');
    }

    for (const inc of includePaths) {
        args.push('-I', inc);
    }

    args.push(path.join(ROOT, 'native', 'src') + '/');

    console.log(`[cppcheck] Running: cppcheck ${args.join(' ')}\n`);

    const result = spawnSync('cppcheck', args, {
        cwd: ROOT,
        stdio: isXml ? ['inherit', 'inherit', 'pipe'] : 'inherit',
        encoding: 'utf8',
    });

    // For XML mode, write stderr to stdout (cppcheck outputs XML on stderr)
    if (isXml && result.stderr) {
        process.stderr.write(result.stderr);
    }

    process.exit(result.status || 0);
}

main();
