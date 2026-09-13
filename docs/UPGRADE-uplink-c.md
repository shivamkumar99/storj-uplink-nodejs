# uplink-c upgrade plan

_Assessed 2026-09-13 on branch `feat/uplink-c-1.15-upgrade` (from `main` @ c869dc5)._

## 1. Where we are

| Item | Value | Evidence |
|---|---|---|
| Published npm version | storj-uplink-nodejs 1.0.3 | `package.json`, npm |
| uplink-c embedded in the shipped prebuilt | **v1.14.1** (module `storj.io/uplink-c`), `storj.io/uplink v1.14.1`, built with go1.25.0 | `go version -m native/prebuilds/darwin-arm64/libuplink.dylib` |
| uplink-c version the prebuild workflow *says* it builds | v1.14.0 (`UPLINK_C_VERSION` default in `.github/workflows/prebuilds.yml`) | |
| uplink-c version a source build actually uses | **whatever `main` is at build time** — the Makefile clones `main` (`git clone --depth 1`, `git pull origin main`) and never reads `UPLINK_C_VERSION` | `Makefile` lines 346-351; no `UPLINK_C_VERSION` in the Makefile |
| C functions the binding calls | 82 (79 `uplink_*` + edge + 3 local `*_to_js` helpers) | `grep -rhoE '\b(uplink|edge)_[a-z0-9_]+\(' native/src` |

So the effective version is v1.14.1 for prebuilt installs and "unpinned main" for `hybrid`/`source` installs. Those can silently differ, and the workflow input that looks like a pin does nothing.

## 2. What upstream has

**uplink-c** (`storj/uplink-c`)

| Ref | Date | C exports | Change vs previous |
|---|---|---|---|
| v1.10.1 | 2025-04-29 | 85 | |
| v1.14.0 | 2026-04-13 | 86 | +`uplinkFlushCoverage` (test-coverage hook, not an API) |
| v1.14.1 | 2026-04-29 | 86 | dependency bump only |
| `main` @ fa48e8c | 2026-08-24 | 86 | `go.mod`: uplink v1.14.1 → **v1.14.3**; `dial_timeout_milliseconds` marked **deprecated** in `uplink_definitions.h` (comment only, field still present) |

Between v1.14.1 and `main` there are **no new exported functions, no removed functions and no signature changes** (`git diff v1.14.1 main -- '*.go' | grep '^[+-]func'` is empty; `uplink_definitions.h` gains two comment lines). The C surface has been stable since v1.10.1.

**storj.io/uplink** (the Go library uplink-c wraps)

| Tag | Date | Notable |
|---|---|---|
| v1.14.3 | 2026-06-22 | benign stream-close error no longer surfaced on partial download; error wrapping fixes |
| v1.14.4 | 2026-08-18 | `DeleteObjects` reports delete-marker; race fix in stream buffer Close/Write; dependency bumps |
| v1.14.5 | 2026-08-31 | **granular Object Lock permissions**; upload buffer pooling; per-download task naming; splitter/write-ahead changes |

uplink-c `main` is on v1.14.3; nothing in uplink-c exposes Object Lock, retention, legal hold or object versioning yet (zero matches in `uplink_definitions.h` and the `//export` list). Those Go-level features cannot reach us until uplink-c grows C entry points for them.

## 3. Binding coverage against v1.14.1

Exports the binding does not call, and why that is fine:

| Export | Status |
|---|---|
| `uplink_free_read_result`, `uplink_free_write_result`, `uplink_free_upload_info`, `uplink_free_part` | thin wrappers around `uplink_free_error(result.error)`; the binding frees the error directly in every `*_complete.c` path (verified) — no leak |
| `edge_free_credentials` | the binding uses `edge_free_credentials_result`, which frees the credentials and the error |
| `uplink_internal_UniverseIsEmpty`, `uplinkFlushCoverage` | test/debug hooks (the first is already exposed via `internalUniverseIsEmpty` for leak tests) |

Conclusion: **there is no missing function or parameter to add today.** The upgrade is about reproducibility and staying current, not about new API.

## 4. Plan

_Status (2026-09-13, branch `feat/uplink-c-1.15-upgrade`): Phase 1 done, Phase 3
done (bumped to `main` @ fa48e8c = storj.io/uplink v1.14.3, verified by
`make verify-uplink-c-version` and `uplinkCVersion()`), workflows moved to Go
1.25 and hardened; Phase 2 (API-drift script) and the linux-arm64 prebuild
(Phase 4) are still open. `dialTimeoutMilliseconds` deprecation note pending._


### Phase 1 — make the uplink-c version explicit and reproducible (this branch)

1. `Makefile`: add `UPLINK_C_VERSION ?= v1.14.1` and clone with `--branch $(UPLINK_C_VERSION) --depth 1`; drop the `git pull origin main` path. Allow a commit SHA as well as a tag (fetch + checkout) so we can build a pre-tag `main` when needed.
2. `.github/workflows/prebuilds.yml`: keep the input, default it to the same value, and pass it through — it already does, the Makefile just ignored it.
3. Record the version in the package: write `native/include/UPLINK_C_VERSION` during the build and export it from TypeScript (`uplinkCVersion()` next to `VERSION`), so `storj-uplink-mcp` and users can log which uplink-c they run.
4. CI guard: after any build, run `go version -m libuplink.*` and fail if the `storj.io/uplink-c` module version does not equal `UPLINK_C_VERSION`.

### Phase 2 — API-drift guard (this branch)

5. `scripts/check-uplink-c-api.mjs`: clone the pinned uplink-c ref, collect `//export` names, diff against the symbols used in `native/src`, and fail on (a) a used symbol that no longer exists, (b) a signature change in a used symbol, and (c) print new exports as a warning. Run it in CI. This is the mechanism that answers "did upstream add functions or parameters?" automatically from now on.

### Phase 3 — bump (after Phase 1 lands)

6. Move the pin to uplink-c `main` @ fa48e8c (brings storj.io/uplink v1.14.3: partial-download error fix) or to the next uplink-c tag if one appears first; nothing in the binding changes for it.
7. `dialTimeoutMilliseconds`: keep accepting it (the field still exists), mark it `@deprecated` in `src/types/index.ts` and the README, and stop mentioning it in examples. Remove in the next major once upstream removes the field.
8. Rebuild all prebuilds, run the native test suite and the storj-uplink-mcp e2e suite (local storj-up network) against the new binaries.

### Phase 4 — release 1.0.4

9. Ship together with the already-merged node-gyp fix for hybrid/source installs, and add **linux-arm64** to the prebuild matrix (currently absent, which is what forces the MCP Docker image onto linux/amd64 emulation).

### Later — features that need upstream first

10. Object Lock / versioning bindings once uplink-c exposes them (track `storj/uplink-c` for `uplink_*object_lock*` / retention exports; storj.io/uplink v1.14.5 has the Go side).

## 5. How to verify each step

- Phase 1: `make install-source UPLINK_C_VERSION=v1.14.1` twice on a clean tree yields byte-identical `libuplink.*` module info (`go version -m`), and `node -e "console.log(require('.').uplinkCVersion())"` prints `v1.14.1`.
- Phase 2: temporarily point the script at v1.10.1 — it must report `uplinkFlushCoverage` missing and nothing else.
- Phase 3: `npm test` (native + TS), then in `storj-uplink-mcp`: `npm run test:e2e` with the new library linked.

## 6. Header-level verification (v1.14.0 vs v1.14.1 vs `main`)

Done by building uplink-c at each ref (`go build -buildmode=c-shared`) and diffing the
*generated* `libuplink.h`, not by reading Go sources:

| Comparison | Result |
|---|---|
| `extern` declarations v1.14.0 → v1.14.1 | identical (88) |
| `extern` declarations v1.14.1 → `main` @ fa48e8c | identical (88) |
| full generated header v1.14.0 → `main` (ignoring the Go-version banner) | no difference |
| header the published 1.0.3 package was compiled against (`native/include/uplink.h`) | identical to v1.14.1 |
| `uplink_definitions.h` struct fields v1.14.0 → v1.14.1 | no field added or removed |
| `uplink_definitions.h` v1.14.1 → `main` | two comment lines: `dial_timeout_milliseconds` deprecated |
| option/config struct fields the binding never sets | none — all 14 structs fully covered |
| output struct fields the binding never reads | none |

The only source change between v1.14.0 and v1.14.1 is the optional coverage
hook (`uplinkFlushCoverage`, behind the `uplink_coverage` build tag). Between
v1.14.1 and `main` it is the storj.io/uplink bump (v1.14.1 → v1.14.3) plus the
deprecation annotation. **No function in the binding needs a signature or
parameter change for any of these refs.**
