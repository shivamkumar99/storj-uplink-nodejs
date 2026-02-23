# CI / Continuous Integration

The project uses a multi-stage GitHub Actions pipeline covering linting, static analysis, build, tests, and security.

---

## Workflows

| Workflow | Trigger | Purpose |
| --- | --- | --- |
| `ci.yml` | Push / PR to `main`, `develop` | Lint, build, unit + integration tests across all platforms |
| `nightly.yml` | Nightly schedule | Full test suite including memory and E2E tests |
| `prebuilds.yml` | Release tag | Build and publish prebuilt native binaries |
| `release.yml` | Release tag | Publish to npm registry |
| `security.yml` | Push / PR | SAST, audit, secret scanning, license check |
| `npm-install-verify.yml` | Post-release / manual | Verify `npm install storj-uplink-nodejs` works from the published package |

---

## CI Pipeline (`ci.yml`)

The main CI workflow runs on every push and pull request to `main` and `develop`.

### Stages

#### 1. Lint & Format

Runs on: `ubuntu-latest`

```sh
npm ci --ignore-scripts   # install without triggering native build
npm run lint              # ESLint
npx prettier --check "src/**/*.ts"  # format check
npx tsc --noEmit          # TypeScript type check
```

#### 2. C Static Analysis

Runs on: `ubuntu-latest`

```sh
sudo apt-get install -y cppcheck
npx node-gyp install      # download Node.js headers
node scripts/cppcheck.js  # run C static analysis
```

#### 3. Build Matrix

Builds the native addon from source on all platforms after lint and analysis pass.

| OS | Platform |
| --- | --- |
| `ubuntu-latest` | `linux-x64` |
| `macos-latest` | `darwin-arm64` |
| `windows-latest` | `win32-x64` |

```sh
npm ci --ignore-scripts
make install-source VERBOSE=1   # build from source
make verify-full                # verify the built binary
```

#### 4. Test Matrix

Tests run on all platforms and Node.js versions:

| OS | Node versions |
| --- | --- |
| Ubuntu (Linux x64) | 18, 20, 22 |
| macOS (Apple Silicon) | 18, 20, 22 |
| Windows (x64) | 18, 20, 22 |

```sh
npm run build:native    # build native addon
npm run build:ts        # compile TypeScript
npm run test:c          # C native unit tests
npm run test:unit       # Jest unit tests (no credentials needed)
npm run test:integration # Integration tests (requires Storj credentials)
```

#### 5. Install Persistence Test

Verifies that the install process works correctly and the `.uplinkrc` state persists across sessions.

#### 6. CI Success Gate

A final job that passes only if all required jobs succeeded. Used as a required status check for pull requests.

---

## Test Commands Reference

```sh
# All tests (unit + integration)
npm test

# Unit tests only (fast, no credentials)
npm run test:unit

# Integration tests (requires TEST_SATELLITE, TEST_API_KEY, TEST_PASSPHRASE)
npm run test:integration

# C native unit tests
npm run test:c
npm run test:c:helpers
npm run test:c:string
npm run test:c:handle

# All tests including C
npm run test:all

# Memory leak tests (long-running)
npm run test:memory

# End-to-end tests
npm run test:e2e

# Test coverage report
npm run test:coverage

# Watch mode (development)
npm run test:watch
```

---

## Security Pipeline (`security.yml`)

Runs on every push and pull request:

| Check | Tool |
| --- | --- |
| ESLint security rules | `eslint-plugin-security`, `eslint-plugin-sonarjs` |
| npm audit | `npm audit --audit-level=high` |
| SAST (TypeScript + Node.js) | `semgrep` |
| C code analysis | `cppcheck` |
| Secret scanning | `gitleaks` |
| Dependency vulnerabilities | `snyk` |
| License compliance | `license-checker` |

Run security checks locally:

```sh
# All security checks
npm run security:all

# Individual checks
npm run security:lint      # ESLint security rules
npm run security:audit     # npm audit
npm run security:sast      # Semgrep SAST
npm run security:c         # C code cppcheck
npm run security:secrets   # Gitleaks secret scan
npm run security:licenses  # License checker
```

---

## Nightly Pipeline (`nightly.yml`)

Runs nightly on a schedule. Includes:

* Full build matrix (all platforms + Node versions)
* Unit tests
* Integration tests (with Storj test network credentials)
* Memory leak tests
* End-to-end scenario tests

---

## Release Pipeline (`release.yml`)

Triggered on version tags (`v*`). Steps:

1. Build prebuilt binaries for all platforms
2. Run full test suite
3. Publish to npm with `npm publish`
4. Create GitHub Release with prebuilt binary artifacts

---

## Prebuilds Pipeline (`prebuilds.yml`)

Builds and stores prebuilt native binaries for:

| Platform | Architecture |
| --- | --- |
| Linux | x64 |
| macOS | arm64 (Apple Silicon) |
| macOS | x64 (Intel) |
| Windows | x64 |

Prebuilts are attached to GitHub releases and downloaded automatically on `npm install`.

---

## Build Matrix Details

The project supports Node.js `>=18` with native N-API bindings. Because N-API provides a stable ABI, a binary built for one Node.js version works on all Node.js versions >= the build target (Node-API version 8, which is Node.js 16+).

| Variable | Value |
| --- | --- |
| `NODE_VERSION` | 20 (default CI) |
| `GO_VERSION` | 1.21 (required for source builds) |
| Platforms | linux-x64, darwin-arm64, win32-x64 |
| Node test versions | 18, 20, 22 |

---

## Running CI Locally

You can replicate CI steps locally:

```sh
# 1. Lint
npm run lint
npx prettier --check "src/**/*.ts"
npx tsc --noEmit

# 2. C analysis (requires cppcheck installed)
node scripts/cppcheck.js

# 3. Build from source (requires Go)
make install-source

# 4. Run tests
npm run test:c
npm run test:unit
npm run test:integration

# 5. Security
npm run security:audit
npm run security:lint
```

> NOTE: Integration tests require `TEST_SATELLITE`, `TEST_API_KEY`, and `TEST_PASSPHRASE` environment variables or a `.env.test` file.
