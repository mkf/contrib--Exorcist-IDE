## Context

See [`proposal.md`](proposal.md) for motivation. Current state that shapes the approach:

- The triggering incident: [`NotificationToast::dismiss()`](../../../src/ui/notificationtoast.cpp) dereferenced `m_fadeAnim` after the animation's `DeleteWhenStopped` self-deletion — fixed in commit `1bec0cf` by giving the member-tracked animation a manually-managed lifetime and nulling both stop paths.
- Baseline is `CMAKE_CXX_STANDARD 17` in [`CMakeLists.txt:5`](../../../CMakeLists.txt:5). The Nix devShell ([`flake.nix`](../../../flake.nix)) ships GCC 15.3.0 and Qt 6.11.2; a `-std=c++26` probe translation unit compiles.
- Pattern census (observed during scoping): one further `DeleteWhenStopped` site (local variable, correct); eight `WA_DeleteOnClose` sites in [`src/`](../../../src) (dialogs/docks — mostly raw locals or explicit ownership transfer, but several deserve re-verification); member-`deleteLater` sites to be enumerated by the sweep task.
- No sanitizer or standard-library hardening configuration exists today; CI runs plain `ctest` only.
- The Ultralight web chat (`src/resources/chat/chat.html`, `chat.js`) is out of scope per [`AGENTS.md`](../../../AGENTS.md).

## Goals / Non-Goals

**Goals:**

- Single-point control of the language standard with a safe fallback ladder, defaulting to C++26.
- Deterministic failure for standard-library bounds/pointer violations in dev builds.
- ASan/UBSan as an enforced test gate, not an optional tool.
- A written, audited lifetime rule that removes the toast bug class.

**Non-Goals:**

- Bulk stylistic modernization of working code.
- Touching Ultralight-rendered surfaces or `third_party/`.
- Adopting any C++26 feature before a compile probe proves GCC 15.3 support.
- Activating plugins currently commented out in [`plugins/CMakeLists.txt`](../../../plugins/CMakeLists.txt).

## Decisions

### Decision: Standard via cache option with fallback ladder

Introduce `EXO_CXX_STANDARD` (cache, default `26`) feeding `CMAKE_CXX_STANDARD` for host, plugins, and tests. If Qt 6.11/MOC/plugin code hits an unresolvable C++26 incompatibility, configuring with `-DEXO_CXX_STANDARD=20` (then `17`) is the documented fallback — one variable, all targets.

- **Why:** Qt does not officially certify C++26 application builds, so the default must be reversible without code edits, and the applied value must be recorded in `docs/cpp26-memory-safety.md`.

### Decision: Hardening as a target compile definition

Set `_GLIBCXX_HARDENING_MODE=1` via `target_compile_definitions` on first-party Debug and RelWithDebInfo targets. Release builds stay untouched to avoid profiling noise.

- **Why:** Zero build-system risk, immediate deterministic aborts on hardening violations, and GCC 15.3's libstdc++ ships the hardened mode.

### Decision: Sanitizer option plus preset and CI gate

Add `EXO_SANITIZE` (string, e.g. `address,undefined`) consumed by a new `cmake/sanitizers.cmake` that injects `-fsanitize=...` compile and link flags; a `sanitize` entry in [`CMakePresets.json`](../../../CMakePresets.json); ctest runs with `ASAN_OPTIONS=abort_on_error=1` and `UBSAN_OPTIONS=halt_on_error=1` under `QT_QPA_PLATFORM=offscreen`. CI gains a job running this configuration.

- **Why:** The suite must prove the codebase is free of the bug class, and keep proving it on every change.

### Decision: Lifetime rule R1 for policy-managed QObjects

No raw member pointer to a `QObject` whose lifetime ends by policy (`DeleteWhenStopped`, `WA_DeleteOnClose`, `deleteLater()`, `delete this`). Allowed forms: `QPointer`, reset via `destroyed()`, or manual lifetime ownership (the `1bec0cf` style: drop the self-delete policy, parent the object, manage it explicitly). Fixes prefer manual lifetime when the code needs to stop or reuse the object.

- **Why:** Each form makes the dangling window structurally impossible rather than depending on call-order discipline.

### Decision: Probe matrix documented, not generated

Feature availability (contract assertions, hardened mode, `std::expected`) is verified with `check_cxx_source_compiles` probes during development and the results are written by hand into `docs/cpp26-memory-safety.md`. CMake does not branch on probes at configure time.

- **Why:** Keeps builds hermetic and the toolchain requirements explicit; features are adopted per-feature, never speculatively.

## Risks / Trade-offs

- Qt 6.11 under C++26 is uncharted → mitigated by the fallback ladder plus the full ctest gate before merge.
- ASan slows the suite (~2x) and increases memory → sanitizer job is CI-side and locally opt-in via preset; default builds unaffected.
- Hardening converts silent corruption into aborts → intentional: fail-fast matches the crash-forward handling elsewhere in the app.
- `QPointer` conversions can hide a genuine ownership design flaw → the audit table records each site's chosen form and rationale.

## Migration Plan

1. Build wiring: `EXO_CXX_STANDARD` + hardening defines; full build and ctest green at the chosen standard (fallback if blocked), recording the outcome.
2. Sanitizer preset + CI job; triage findings to zero — expected findings are few and concentrated in the audited lifetime sites.
3. Lifetime audit sweep (`src/` then `plugins/`), fixes per rule R1, behavioral regression test for the toast pattern.
4. Documentation: `docs/cpp26-memory-safety.md` (audit table, probe matrix, applied standard) and the R1 line in [`AGENTS.md`](../../../AGENTS.md).
