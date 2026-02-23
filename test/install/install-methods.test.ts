/**
 * @file Tests for Sprint 13 Makefile Installation Methods
 * 
 * Tests the three installation methods:
 * 1. install-source - Build everything from source (requires Go)
 * 2. install-hybrid - Download lib, build addon (requires C compiler) - SKIPPED until prebuilts available
 * 3. install-prebuilt - Download everything (Node.js only) - SKIPPED until prebuilts available
 * 
 * Note: These tests actually run make commands to verify the installation methods work.
 * They are slow tests and should only run in CI or when explicitly requested.
 */

import { execSync, ExecSyncOptions } from 'child_process';
import * as fs from 'fs';
import * as path from 'path';

const PROJECT_ROOT = path.resolve(__dirname, '../..');

// Detect platform
const PLATFORM = (() => {
  const os = process.platform;
  const arch = process.arch === 'x64' ? 'x64' : process.arch;
  
  if (os === 'darwin') return `darwin-${arch}`;
  if (os === 'linux') return `linux-${arch}`;
  if (os === 'win32') return 'win32-x64';
  return `${os}-${arch}`;
})();

// Library and addon names
const LIB_NAME = process.platform === 'darwin' 
  ? 'libuplink.dylib' 
  : process.platform === 'win32' 
    ? 'libuplink.dll' 
    : 'libuplink.so';

const ADDON_NAME = 'uplink_native.node';

// Paths
const PREBUILDS_DIR = path.join(PROJECT_ROOT, 'native', 'prebuilds', PLATFORM);

// Helper to run make commands
function runMake(target: string, options: { verbose?: boolean; timeout?: number } = {}): string {
  const execOptions: ExecSyncOptions = {
    cwd: PROJECT_ROOT,
    encoding: 'utf-8',
    timeout: options.timeout || 300000, // 5 minutes default
    stdio: 'pipe',
  };
  
  const verboseFlag = options.verbose ? 'VERBOSE=1' : '';
  const command = `make ${target} ${verboseFlag}`.trim();
  
  try {
    return execSync(command, execOptions) as string;
  } catch (error: unknown) {
    const execError = error as { stdout?: string; stderr?: string; message?: string };
    throw new Error(
      `Make command failed: ${command}\n` +
      `stdout: ${execError.stdout || ''}\n` +
      `stderr: ${execError.stderr || ''}\n` +
      `error: ${execError.message || ''}`
    );
  }
}

// Helper to check if a prerequisite is available
function hasPrerequisite(name: string): boolean {
  try {
    runMake(`check-${name}`, { timeout: 10000 });
    return true;
  } catch {
    return false;
  }
}

// Helper to check if file exists
function fileExists(filePath: string): boolean {
  return fs.existsSync(filePath);
}

// Helper to clean prebuilds
function cleanPrebuilds(): void {
  runMake('clean-prebuilds', { timeout: 30000 });
}

describe('Sprint 13: Makefile Installation Methods', () => {
  // Check prerequisites before running tests
  const hasGo = hasPrerequisite('go');
  const hasCompiler = hasPrerequisite('compiler');
  const hasNode = hasPrerequisite('node');
  const hasCurl = hasPrerequisite('curl');
  
  beforeAll(() => {
    console.log('\n=== Installation Methods Test Info ===');
    console.log(`Platform: ${PLATFORM}`);
    console.log(`Project: ${PROJECT_ROOT}`);
    console.log(`Prebuilds: ${PREBUILDS_DIR}`);
    console.log(`Library: ${LIB_NAME}`);
    console.log(`Prerequisites:`);
    console.log(`  - Node.js: ${hasNode ? '✓' : '✗'}`);
    console.log(`  - Go: ${hasGo ? '✓' : '✗'}`);
    console.log(`  - C Compiler: ${hasCompiler ? '✓' : '✗'}`);
    console.log(`  - curl: ${hasCurl ? '✓' : '✗'}`);
    console.log('=====================================\n');
  });
  
  describe('Prerequisites Checks', () => {
    test('check-node verifies Node.js is available', () => {
      const output = runMake('check-node');
      expect(output).toContain('Node.js');
    });
    
    test('check-npm verifies npm is available', () => {
      const output = runMake('check-npm');
      expect(output).toContain('npm');
    });
    
    test('check-source-prereqs verifies all source build prerequisites', () => {
      if (!hasGo || !hasCompiler) {
        console.log('Skipping: Go or C compiler not available');
        return;
      }
      const output = runMake('check-source-prereqs');
      expect(output).toContain('All prerequisites for source build satisfied');
    });
    
    test('check-hybrid-prereqs verifies all hybrid build prerequisites', () => {
      if (!hasCompiler) {
        console.log('Skipping: C compiler not available');
        return;
      }
      const output = runMake('check-hybrid-prereqs');
      expect(output).toContain('All prerequisites for hybrid build satisfied');
    });
    
    test('check-prebuilt-prereqs verifies prebuilt prerequisites (Node.js only)', () => {
      const output = runMake('check-prebuilt-prereqs');
      expect(output).toContain('All prerequisites for prebuilt download satisfied');
    });
  });
  
  describe('Option 1: install-source (Full Source Build)', () => {
    // This test actually builds from source - takes time
    const timeout = 600000; // 10 minutes
    
    beforeAll(() => {
      // Skip if prerequisites not available
      if (!hasGo || !hasCompiler) {
        console.log('Skipping install-source tests: Go or C compiler not available');
        return;
      }
    });
    
    test('install-source builds library and addon from source', () => {
      if (!hasGo || !hasCompiler) {
        console.log('Skipping: Go or C compiler not available');
        return;
      }
      
      // Clean first
      cleanPrebuilds();
      expect(fileExists(path.join(PREBUILDS_DIR, LIB_NAME))).toBe(false);
      
      // Run install-source
      const output = runMake('install-source', { verbose: true, timeout });
      
      // Verify output messages
      expect(output).toContain('OPTION 1: Full Source Build');
      expect(output).toContain('Installation complete (Source Build)');
      
      // Verify files exist
      expect(fileExists(path.join(PREBUILDS_DIR, LIB_NAME))).toBe(true);
      expect(fileExists(path.join(PREBUILDS_DIR, ADDON_NAME))).toBe(true);
    }, timeout);
    
    test('verify-full succeeds after install-source', () => {
      if (!hasGo || !hasCompiler) {
        console.log('Skipping: Go or C compiler not available');
        return;
      }
      
      // Verify should pass if install-source succeeded
      if (!fileExists(path.join(PREBUILDS_DIR, LIB_NAME))) {
        console.log('Skipping: Library not built');
        return;
      }
      
      const output = runMake('verify-full');
      expect(output).toContain('Verification passed!');
      expect(output).toContain(`✓ ${LIB_NAME}`);
      expect(output).toContain(`✓ ${ADDON_NAME}`);
    });
    
    test('native module can be loaded after install-source', () => {
      if (!hasGo || !hasCompiler) {
        console.log('Skipping: Go or C compiler not available');
        return;
      }
      
      if (!fileExists(path.join(PREBUILDS_DIR, ADDON_NAME))) {
        console.log('Skipping: Addon not built');
        return;
      }
      
      // The native module should be loadable through the normal dist path
      // Direct loading from prebuilds requires proper library path setup
      // This test verifies the files exist and the module structure is correct
      const addonPath = path.join(PREBUILDS_DIR, ADDON_NAME);
      const libPath = path.join(PREBUILDS_DIR, LIB_NAME);
      
      // Verify both files exist
      expect(fileExists(addonPath)).toBe(true);
      expect(fileExists(libPath)).toBe(true);
      
      // Verify addon is a valid file with expected size
      const stats = fs.statSync(addonPath);
      expect(stats.size).toBeGreaterThan(10000); // Should be > 10KB
      
      // On macOS, verify the addon has correct @rpath reference
      if (process.platform === 'darwin') {
        try {
          const output = execSync(`otool -L "${addonPath}"`, { encoding: 'utf-8' });
          expect(output).toContain('@rpath/libuplink.dylib');
        } catch {
          console.log('Warning: otool not available to verify rpath');
        }
      }
    });
  });
  
  describe('Option 2: install-hybrid (Download Lib, Build Addon)', () => {
    // Note: Set PREBUILT_LIB_AVAILABLE = true when prebuilts are published
    // Prebuilts are hosted at: https://github.com/GITHUB_OWNER/uplink-nodejs/releases
    // Override the owner with: make install-hybrid GITHUB_OWNER=your-username
    
    const PREBUILT_LIB_AVAILABLE = false; // Set to true when prebuilts are published
    
    test.skip('install-hybrid downloads library and builds addon', () => {
      if (!PREBUILT_LIB_AVAILABLE) {
        console.log('Skipping: Prebuilt uplink-c library not available for download');
        console.log('Publish prebuilts to your GitHub repo releases');
        console.log('Use install-source instead');
        return;
      }
      
      if (!hasCompiler) {
        console.log('Skipping: C compiler not available');
        return;
      }
      
      // Clean first
      cleanPrebuilds();
      
      // Run install-hybrid
      const output = runMake('install-hybrid', { verbose: true, timeout: 300000 });
      
      expect(output).toContain('OPTION 2: Hybrid Build');
      expect(output).toContain('Installation complete (Hybrid Build)');
      expect(fileExists(path.join(PREBUILDS_DIR, LIB_NAME))).toBe(true);
      expect(fileExists(path.join(PREBUILDS_DIR, ADDON_NAME))).toBe(true);
    });
    
    test('install-hybrid fails gracefully when prebuilt not available', () => {
      if (!hasCompiler || !hasCurl) {
        console.log('Skipping: C compiler or curl not available');
        return;
      }
      
      // Skip if prebuilts are available (test won't fail)
      if (PREBUILT_LIB_AVAILABLE) {
        console.log('Skipping: Prebuilts are available');
        return;
      }
      
      // This should fail with a helpful message
      cleanPrebuilds();
      
      expect(() => {
        runMake('download-lib', { timeout: 60000 });
      }).toThrow(/Download failed|404|not available/);
    });
  });
  
  describe('Option 3: install-prebuilt (Download Everything)', () => {
    // Note: Set PREBUILT_AVAILABLE = true when prebuilts are published
    // Prebuilts are hosted at: https://github.com/GITHUB_OWNER/uplink-nodejs/releases
    // Override the owner with: make install-prebuilt GITHUB_OWNER=your-username
    
    const PREBUILT_AVAILABLE = false; // Set to true when prebuilts are published
    
    test.skip('install-prebuilt downloads library and addon', () => {
      if (!PREBUILT_AVAILABLE) {
        console.log('Skipping: Prebuilt binaries not available for download');
        console.log('Publish prebuilts to your GitHub repo releases');
        console.log('Use install-source for now');
        return;
      }
      
      // Clean first
      cleanPrebuilds();
      
      // Run install-prebuilt
      const output = runMake('install-prebuilt', { verbose: true, timeout: 180000 });
      
      expect(output).toContain('OPTION 3: Full Prebuilt');
      expect(output).toContain('Installation complete (Prebuilt');
      expect(fileExists(path.join(PREBUILDS_DIR, LIB_NAME))).toBe(true);
      expect(fileExists(path.join(PREBUILDS_DIR, ADDON_NAME))).toBe(true);
    });
    
    test('install-prebuilt fails gracefully when prebuilt not available', () => {
      if (!hasCurl) {
        console.log('Skipping: curl not available');
        return;
      }
      
      // Skip if prebuilts are available (test won't fail)
      if (PREBUILT_AVAILABLE) {
        console.log('Skipping: Prebuilts are available');
        return;
      }
      
      // This should fail with a helpful message
      cleanPrebuilds();
      
      expect(() => {
        runMake('download-addon', { timeout: 60000 });
      }).toThrow(/Download failed|404|not available/);
    });
  });
  
  describe('Auto-detection (install-auto)', () => {
    // install-auto tries hybrid first, falls back to source
    
    test('install-auto falls back to source when hybrid fails', () => {
      if (!hasGo || !hasCompiler) {
        console.log('Skipping: Go or C compiler not available');
        return;
      }
      
      // Clean first
      cleanPrebuilds();
      
      // Run install-auto - should fall back to install-source
      // since hybrid download will fail
      const output = runMake('install-auto', { verbose: true, timeout: 600000 });
      
      // Should either succeed with hybrid (if prebuilts exist) or fall back to source
      expect(
        output.includes('Installation complete') ||
        output.includes('Source Build')
      ).toBe(true);
    }, 600000);
  });
  
  describe('Verification Targets', () => {
    test('verify checks library exists', () => {
      // May fail if library not built
      try {
        const output = runMake('verify');
        expect(output).toContain(LIB_NAME);
      } catch (error) {
        // Expected to fail if library not built
        expect(String(error)).toContain('NOT FOUND');
      }
    });
    
    test('verify-full checks both library and addon', () => {
      try {
        const output = runMake('verify-full');
        expect(output).toContain(LIB_NAME);
        expect(output).toContain(ADDON_NAME);
        expect(output).toContain('Verification passed!');
      } catch (error) {
        // Expected to fail if not built
        expect(String(error)).toContain('NOT FOUND');
      }
    });
  });
  
  describe('.uplinkrc Persistence (install method memory)', () => {
    /**
     * Tests that the chosen install method is saved to .uplinkrc and
     * automatically reused on subsequent `make install` calls.
     *
     * Real-world scenario:
     *   1. Developer runs: UPLINK_INSTALL=prebuilt npm install <pkg>
     *   2. .uplinkrc is created with "prebuilt"
     *   3. Developer deletes node_modules and runs: npm install
     *   4. postinstall → make install reads .uplinkrc → uses "prebuilt" again
     */

    const UPLINKRC_PATH = path.join(PROJECT_ROOT, '.uplinkrc');

    // Helper to read .uplinkrc content
    function readRc(): string | null {
      try {
        return fs.readFileSync(UPLINKRC_PATH, 'utf8').trim();
      } catch {
        return null;
      }
    }

    // Helper to delete .uplinkrc
    function deleteRc(): void {
      try { fs.unlinkSync(UPLINKRC_PATH); } catch { /* ignore */ }
    }

    // Helper to run make install with UPLINK_INSTALL and capture output
    function runInstallWithMethod(method: string, timeout = 600000): string {
      const execOptions: ExecSyncOptions = {
        cwd: PROJECT_ROOT,
        encoding: 'utf-8',
        timeout,
        stdio: 'pipe',
        env: { ...process.env, UPLINK_INSTALL: method },
      };
      try {
        return execSync('make install', execOptions) as string;
      } catch (error: unknown) {
        const execError = error as { stdout?: string; stderr?: string; message?: string };
        throw new Error(
          `make install (UPLINK_INSTALL=${method}) failed:\n` +
          `stdout: ${execError.stdout || ''}\n` +
          `stderr: ${execError.stderr || ''}\n` +
          `error: ${execError.message || ''}`
        );
      }
    }

    // Helper to run make install WITHOUT UPLINK_INSTALL (should read .uplinkrc)
    function runInstallWithoutMethod(timeout = 600000): string {
      const execOptions: ExecSyncOptions = {
        cwd: PROJECT_ROOT,
        encoding: 'utf-8',
        timeout,
        stdio: 'pipe',
        env: { ...process.env, UPLINK_INSTALL: '' },
      };
      // Remove UPLINK_INSTALL from env entirely
      delete execOptions.env!.UPLINK_INSTALL;
      try {
        return execSync('make install', execOptions) as string;
      } catch (error: unknown) {
        const execError = error as { stdout?: string; stderr?: string; message?: string };
        throw new Error(
          `make install (no UPLINK_INSTALL) failed:\n` +
          `stdout: ${execError.stdout || ''}\n` +
          `stderr: ${execError.stderr || ''}\n` +
          `error: ${execError.message || ''}`
        );
      }
    }

    afterAll(() => {
      // Always clean up .uplinkrc after tests
      deleteRc();
    });

    test('.uplinkrc does not exist initially (clean state)', () => {
      deleteRc();
      expect(fs.existsSync(UPLINKRC_PATH)).toBe(false);
    });

    test('make install without UPLINK_INSTALL shows "auto" when no .uplinkrc', () => {
      deleteRc();

      // Use skip-like quick run: just capture the banner output
      // We set UPLINK_INSTALL=skip to avoid actually building, just to test the "no rc" path
      // Instead, run without env var and check the output mentions auto-detect
      const output = runInstallWithoutMethod();
      expect(output).toContain('auto');
    }, 600000);

    test('UPLINK_INSTALL=prebuilt saves method to .uplinkrc', () => {
      deleteRc();

      const output = runInstallWithMethod('prebuilt');

      // Should save to .uplinkrc
      expect(output).toContain('Saved install method');
      expect(fs.existsSync(UPLINKRC_PATH)).toBe(true);
      expect(readRc()).toBe('prebuilt');
    }, 600000);

    test('subsequent make install reads method from .uplinkrc', () => {
      // .uplinkrc should contain "prebuilt" from previous test
      expect(readRc()).toBe('prebuilt');

      const output = runInstallWithoutMethod();

      // Should show method was loaded from .uplinkrc
      expect(output).toContain('.uplinkrc');
      expect(output).toContain('prebuilt');
      // Banner should show: Install method : prebuilt (from .../.uplinkrc)
      expect(output).toMatch(/Install method\s*:\s*prebuilt\s*\(from/);
    }, 600000);

    test('explicit UPLINK_INSTALL overrides .uplinkrc', () => {
      // .uplinkrc says "prebuilt", but we pass skip
      fs.writeFileSync(UPLINKRC_PATH, 'prebuilt\n', 'utf8');

      const output = runInstallWithMethod('skip');

      // Should show "skip" was used, not "prebuilt"
      expect(output).toContain('Method: skip');
      // Should NOT show "(from .uplinkrc)"
      expect(output).not.toMatch(/Install method\s*:\s*prebuilt\s*\(from/);
      // .uplinkrc should now be updated to "skip"
      expect(readRc()).toBe('skip');
    }, 600000);

    test.each(['prebuilt', 'hybrid', 'source', 'skip'] as const)(
      'UPLINK_INSTALL=%s saves correctly to .uplinkrc',
      (method) => {
        deleteRc();

        // Use skip for non-skip methods too — we only care about the save logic
        // For methods that would actually build, we'd need prerequisites
        // So we test the save behavior by checking: does the Makefile write .uplinkrc?
        // For "prebuilt" and "skip" we can run directly (no build needed if binaries exist or skip)
        if (method === 'skip') {
          const output = runInstallWithMethod(method);
          expect(output).toContain('Saved install method');
          expect(readRc()).toBe(method);
        } else if (method === 'prebuilt') {
          // Only works if shipped prebuilds exist
          if (fileExists(path.join(PREBUILDS_DIR, ADDON_NAME)) &&
              fileExists(path.join(PREBUILDS_DIR, LIB_NAME))) {
            const output = runInstallWithMethod(method);
            expect(output).toContain('Saved install method');
            expect(readRc()).toBe(method);
          } else {
            console.log(`Skipping ${method}: prebuilt binaries not available`);
          }
        } else if (method === 'source') {
          if (hasGo && hasCompiler) {
            const output = runInstallWithMethod(method, 600000);
            expect(output).toContain('Saved install method');
            expect(readRc()).toBe(method);
          } else {
            console.log(`Skipping ${method}: Go or compiler not available`);
          }
        } else if (method === 'hybrid') {
          // Hybrid requires download — skip unless prebuilts are published
          console.log(`Skipping ${method}: requires prebuilt download`);
        }
      },
      600000
    );

    test('invalid .uplinkrc content is ignored', () => {
      // Write garbage to .uplinkrc
      fs.writeFileSync(UPLINKRC_PATH, 'invalid-method\n', 'utf8');

      const output = runInstallWithoutMethod();

      // Should ignore the invalid value and auto-detect
      expect(output).toContain('invalid');
      expect(output).toContain('auto');
    }, 600000);

    test('empty .uplinkrc is treated as no preference', () => {
      fs.writeFileSync(UPLINKRC_PATH, '\n', 'utf8');

      const output = runInstallWithoutMethod();

      // Should fall through to auto-detect
      expect(output).toContain('auto');
    }, 600000);

    test('.uplinkrc survives clean-prebuilds', () => {
      // Write a known value
      fs.writeFileSync(UPLINKRC_PATH, 'prebuilt\n', 'utf8');

      // Run clean-prebuilds
      runMake('clean-prebuilds');

      // .uplinkrc should still exist (it's in project root, not prebuilds)
      expect(fs.existsSync(UPLINKRC_PATH)).toBe(true);
      expect(readRc()).toBe('prebuilt');
    });

    test('deleting .uplinkrc resets to auto-detect', () => {
      fs.writeFileSync(UPLINKRC_PATH, 'prebuilt\n', 'utf8');
      deleteRc();

      const output = runInstallWithoutMethod();
      expect(output).toContain('auto');
    }, 600000);

    test('full persistence cycle: install → delete node_modules → reinstall', () => {
      /**
       * This is the key end-to-end test for the persistence feature.
       *
       * Scenario:
       *   1. Install with UPLINK_INSTALL=prebuilt (or skip for speed)
       *   2. Verify .uplinkrc was created
       *   3. Simulate "delete node_modules && npm install" by running
       *      make install again WITHOUT UPLINK_INSTALL
       *   4. Verify the same method is used from .uplinkrc
       */
      deleteRc();

      // Step 1: First install with explicit method
      const firstMethod = 'skip'; // Use skip for speed — tests the persistence, not the build
      const firstOutput = runInstallWithMethod(firstMethod);
      expect(firstOutput).toContain('Saved install method');
      expect(readRc()).toBe(firstMethod);

      // Step 2: Simulate reinstall (no UPLINK_INSTALL set)
      // This is what happens after: rm -rf node_modules && npm install
      const reinstallOutput = runInstallWithoutMethod();

      // Step 3: Verify same method was used
      expect(reinstallOutput).toContain(`.uplinkrc`);
      expect(reinstallOutput).toContain(firstMethod);
      expect(reinstallOutput).toMatch(
        new RegExp(`Install method\\s*:\\s*${firstMethod}\\s*\\(from`)
      );

      // Step 4: .uplinkrc should remain unchanged (not re-saved)
      expect(readRc()).toBe(firstMethod);

      // Cleanup
      deleteRc();
    }, 600000);
  });

  describe('Clean Targets', () => {
    test('clean-prebuilds removes platform prebuilds', () => {
      runMake('clean-prebuilds');
      // Directory may or may not exist after clean
      if (fs.existsSync(PREBUILDS_DIR)) {
        const files = fs.readdirSync(PREBUILDS_DIR);
        // Should be empty or not exist
        expect(files.length === 0 || !fs.existsSync(PREBUILDS_DIR)).toBeTruthy();
      }
    });
    
    test('clean-downloads removes download cache', () => {
      const downloadDir = path.join(PROJECT_ROOT, '.downloads');
      runMake('clean-downloads');
      // Should not exist after clean
      expect(fs.existsSync(downloadDir)).toBe(false);
    });
  });
  
  describe('Help Target', () => {
    test('help shows all available commands', () => {
      const output = runMake('help');
      
      // Check major sections
      expect(output).toContain('INSTALLATION OPTIONS');
      expect(output).toContain('BUILD TARGETS');
      expect(output).toContain('DOWNLOAD TARGETS');
      expect(output).toContain('TEST & VERIFY');
      expect(output).toContain('CLEAN');
      
      // Check all three install options are documented
      expect(output).toContain('install-source');
      expect(output).toContain('install-hybrid');
      expect(output).toContain('install-prebuilt');
    });
  });
});
