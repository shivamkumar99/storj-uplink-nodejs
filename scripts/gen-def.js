#!/usr/bin/env node
/**
 * @file scripts/gen-def.js
 * @brief Generate a Windows .def file from the uplink-c header.
 *
 * Reads native/include/uplink.h, extracts all "extern ... function_name("
 * declarations, and writes a .def file that can be used with dlltool or
 * lib.exe to create an import library (.lib) for MSVC linking.
 *
 * Usage: node scripts/gen-def.js <header-path> <output-def-path>
 */
'use strict';

const fs = require('fs');
const path = require('path');

const headerPath = process.argv[2] || path.join(__dirname, '..', 'native', 'include', 'uplink.h');
const defPath = process.argv[3] || path.join(__dirname, '..', 'native', 'prebuilds', 'win32-x64', 'uplink.def');

if (!fs.existsSync(headerPath)) {
  console.error(`Header not found: ${headerPath}`);
  process.exit(1);
}

const header = fs.readFileSync(headerPath, 'utf8');

// Match "extern [__declspec(dllexport)] <return-type> <function_name>(" patterns from CGO header.
// CGO exports look like: extern __declspec(dllexport) TYPE uplink_something(...)
// or plain: extern TYPE uplink_something(...)
// Use [ \t] instead of \s to prevent matching across newlines (e.g. from "extern "C" {").
const funcRegex = /^extern[ \t]+(?:__declspec\(dllexport\)[ \t]+)?(?:\S+[ \t]+)*?(\w+)[ \t]*\(/gm;
const symbols = new Set();
let match;
while ((match = funcRegex.exec(header)) !== null) {
  const name = match[1];
  // Skip CGO internal helpers
  if (name.startsWith('_')) continue;
  symbols.add(name);
}

if (symbols.size === 0) {
  console.error('No exported symbols found in header');
  process.exit(1);
}

const sorted = Array.from(symbols).sort();
const defContent = 'LIBRARY libuplink.dll\nEXPORTS\n' + sorted.map(s => `    ${s}`).join('\n') + '\n';

// Ensure output directory exists
const outDir = path.dirname(defPath);
if (!fs.existsSync(outDir)) {
  fs.mkdirSync(outDir, { recursive: true });
}

fs.writeFileSync(defPath, defContent);
console.log(`Generated ${defPath} with ${sorted.length} exports`);
