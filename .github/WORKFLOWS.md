# GitHub Actions Workflows

This document describes the CI/CD pipeline for uplink-nodejs.

---

## Workflow Overview

| Workflow | File | Trigger | Purpose |
|----------|------|---------|---------|
| **CI** | `ci.yml` | Push, PR | Fast feedback on code quality |
| **Security** | `security.yml` | Push to main, PR, Weekly | Deep security analysis |
| **Prebuilds** | `prebuilds.yml` | Tags, Manual | Build binaries for all platforms |
| **Release** | `release.yml` | Tags (v*.*.*) | Build & publish GitHub Release |
| **Nightly** | `nightly.yml` | Daily 2 AM UTC | Extended testing |

### Deduplication Notes

Each workflow has a distinct responsibility with minimal overlap:

| Check | CI | Security | Release | Nightly |
|-------|----|----------|---------|---------|
| ESLint + TSC | ✅ | — | — | — |
| cppcheck | ✅ | — | — | — |
| npm audit | ✅ | — | ✅ (critical only) | ✅ |
| CodeQL | — | ✅ | — | — |
| Semgrep | — | ✅ | — | — |
| Snyk | — | ✅ | — | — |
| TruffleHog | — | ✅ | — | — |
| Gitleaks | — | — | ✅ | — |
| Cross-platform build | ✅ | — | ✅ | ✅ |
| Unit tests | ✅ | — | ✅ | ✅ |
| Memory tests | — | — | — | ✅ |
| ASan tests | — | — | — | ✅ |
| Benchmarks | — | — | — | ✅ |

---

## 1. CI Pipeline (`ci.yml`)

**Triggers:** Every push and PR to `main`/`develop`

### CI Jobs

```
┌─────────────┐  ┌──────────────┐
│    lint     │  │ c-analysis   │
└──────┬──────┘  └──────┬───────┘
       │                │
       └────────┬───────┘
                ▼
         ┌─────────────┐
         │    build    │  (ubuntu, macos, windows)
         └──────┬──────┘
                │
       ┌────────┴────────┐
       ▼                 ▼
┌─────────────┐  ┌──────────────────┐
│    test     │  │ install-         │
│(Node 18-22  │  │ persistence      │
│  × 3 OS)   │  │ (.uplinkrc test) │
└──────┬──────┘  └──────┬───────────┘
       │                │
       └────────┬───────┘
                ▼
         ┌─────────────┐
         │ ci-success  │
         └─────────────┘
```

### What it checks:
- ✅ ESLint + Prettier formatting
- ✅ TypeScript type checking
- ✅ cppcheck static analysis for C code
- ✅ Cross-platform builds (Linux, macOS, Windows)
- ✅ Unit & integration tests on Node 18, 20, 22
- ✅ Install persistence (.uplinkrc) on all 3 OS

---

## 2. Security Pipeline (`security.yml`)

**Triggers:** Push to main, PRs, Weekly schedule

### Security Jobs

| Job | Tools | Purpose |
|-----|-------|---------|
| `codeql` | GitHub CodeQL | Semantic code analysis (JS/TS + C) |
| `semgrep` | Semgrep | SAST with TypeScript & OWASP rules |
| `dependency-scan` | Snyk | Dependency vulnerability scanning |
| `secrets-scan` | TruffleHog | Deep secret detection |

### Security Tools Used:
- **CodeQL** - GitHub's semantic analysis for JS/TS and C
- **Semgrep** - Multi-language SAST with OWASP Top 10 rules
- **Snyk** - Dependency vulnerability scanning
- **TruffleHog** - Deep secret scanning

---

## 3. Prebuilds Pipeline (`prebuilds.yml`)

**Triggers:** Git tags (`v*`), Manual dispatch

### Platforms Built

| Platform | Runner | Architecture |
|----------|--------|--------------|
| `linux-x64` | ubuntu-latest | x86_64 |
| `darwin-x64` | macos-15 | x86_64 (Intel) |
| `darwin-arm64` | macos-15 | arm64 (Apple Silicon) |
| `win32-x64` | windows-latest | x86_64 |

### Output
- Archives: `uplink-nodejs-{tag}-{platform}.tar.gz`
- Contains: `libuplink.{dylib|so|dll}` + `uplink_native.node`

---

## 4. Release Pipeline (`release.yml`)

**Triggers:** Git tags (`v*.*.*`), Manual dispatch

> **Note:** npm publishing has been removed from this workflow for security
> reasons. After the GitHub Release is created, publish to npm manually with
> `npm publish`.

### Release Flow

```
┌────────────┐
│  validate  │  Extract version from tag
└─────┬──────┘
      ▼
┌────────────┐
│  security  │  npm audit (critical), Gitleaks
└─────┬──────┘
      ▼
┌────────────┐
│   build    │  All platforms (linux, macOS, windows)
└─────┬──────┘
      ▼
┌────────────┐
│  github-   │  Create GitHub Release with
│  release   │  prebuilt binary archives
└────────────┘
```

### Manual Release
```bash
# Trigger manually with dry-run
gh workflow run release.yml -f version=1.0.0 -f dry_run=true

# Actual release
gh workflow run release.yml -f version=1.0.0
```

### Post-Release (manual)
```bash
# After verifying the GitHub Release, publish to npm:
npm publish --tag latest --access public
```

---

## 5. Nightly Pipeline (`nightly.yml`)

**Triggers:** Daily at 2 AM UTC, Manual

### Extended Tests
- Full test matrix (3 Node versions × 3 OS)
- Memory leak testing with Valgrind
- AddressSanitizer builds
- Performance benchmarks
- Installation methods tests
- Dependency update checks

---

## Required Secrets

Configure these in **Settings → Secrets and variables → Actions**:

| Secret | Required | Purpose |
|--------|----------|---------|
| `SNYK_TOKEN` | 🟡 Optional | Snyk security scanning |
| `CODECOV_TOKEN` | 🟡 Optional | Code coverage reports |
| `SEMGREP_APP_TOKEN` | 🟡 Optional | Semgrep CI integration |
| `STORJ_ACCESS_GRANT` | 🟡 Optional | E2E tests with real network |

> **Note:** `NPM_TOKEN` is no longer required in GitHub Actions since npm
> publishing is done manually.

---

## Branch Protection Rules

Recommended settings for `main` branch:

- ✅ Require status checks to pass
  - `CI Success`
  - `Security Summary`
- ✅ Require branches to be up to date
- ✅ Require pull request reviews
- ✅ Do not allow bypassing the above settings

---

## Release Checklist

1. **Update version in package.json**
   ```bash
   npm version patch|minor|major
   ```

2. **Create and push tag**
   ```bash
   git tag v1.0.0
   git push origin v1.0.0
   ```

3. **Monitor workflows**
   - CI passes ✅
   - Security passes ✅
   - Release workflow completes ✅

4. **Verify GitHub Release**
   - Check GitHub release page for binary archives
   - Download and test a binary on your platform

5. **Publish to npm (manual)**
   ```bash
   npm publish --tag latest --access public
   ```

6. **Post-publish verification**
   - `npm view storj-uplink-nodejs`
   - `npm install storj-uplink-nodejs` in a fresh project

---

## Troubleshooting

### Build fails on specific platform
- Check runner availability
- Verify Go/Node versions support that platform
- Check native dependencies

### Security scan false positives
- Add to `.gitleaks.toml` allowlist
- Add `// eslint-disable-next-line` comments
- Configure tool-specific ignore rules
