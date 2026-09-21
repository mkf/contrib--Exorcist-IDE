## Why

A real SEGFAULT in [`notificationtoast.cpp`](../../../src/ui/notificationtoast.cpp) (fixed in commit `1bec0cf`) exposed a whole bug class this codebase currently has no systemic defense against: **raw member pointers to QObjects whose lifetime is policy-managed** (`QAbstractAnimation::DeleteWhenStopped`, `Qt::WA_DeleteOnClose`, `deleteLater()`, self-deleting dialogs). The toast held `m_fadeAnim` across a `DeleteWhenStopped` self-deletion and crashed on a use-after-free when the auto-dismiss timer fired. More such sites exist: an audit shows 8 `WA_DeleteOnClose` call sites and additional `deleteLater()`/self-delete patterns across `src/` and `plugins/`.

Meanwhile the toolchain has moved ahead of the code: the Nix devShell ships **GCC 15.3.0** (which accepts `-std=c++26`) and **Qt 6.11.2**, while [`CMakeLists.txt`](../../../CMakeLists.txt:5) still pins `CMAKE_CXX_STANDARD 17`. Modern C++ (23/26) plus libstdc++ hardening and sanitizer builds give compiler-enforced detection for exactly the memory-safety failures we just debugged by hand.

## What Changes

- **C++26 language baseline**: raise `CMAKE_CXX_STANDARD` from 17 to 26 (`-std=gnu++26`) for the host app, plugins, and tests, with a **staged verification** and a documented fallback ladder (26 → 20 → 17) exposed as a CMake cache option, because Qt 6.11 does not officially certify C++26 application builds.
- **Hardened C++ standard library**: enable libstdc++ hardening (`_GLIBCXX_HARDENING_MODE=1`) by default in Debug/RelWithDebInfo builds so bounds and null checks inside the standard library abort deterministically instead of corrupting memory silently.
- **Sanitizer builds**: add an AddressSanitizer + UndefinedBehaviorSanitizer build option (CMake preset + ctest wiring) and a CI job that runs the full `ctest` suite under ASan/UBSan; the suite must be clean of findings.
- **Qt-lifetime guard convention + sweep**: adopt and document the rule that no raw member pointer may reference a QObject whose lifetime is policy-managed — such members become `QPointer` (or are nulled via `destroyed()`, or ownership is made explicit). Sweep `src/` and `plugins/` for the pattern, fix violations, and add a regression test that reproduces the toast-style use-after-free and proves the sanitizer build detects it.
- **Feature availability probes**: verify with compile probes which C++26/23 memory-safety features GCC 15.3 actually provides (contract assertions, hardened stdlib mode, `std::expected` for error propagation in new APIs) and record the matrix in `docs/cpp26-memory-safety.md`; adopt per-feature, never speculatively.
- Out of scope: the Ultralight web chat and any Ultralight-rendered surface (`src/resources/chat/chat.html`, `src/resources/chat/chat.js`), which project convention excludes from every change and handles separately; `third_party/` (LuaJIT is built by its own Makefile with its own standard); and any bulk stylistic rewrite of working code — code changes are limited to the lifetime-pattern fixes and build wiring.

## Capabilities

### New Capabilities
- `cpp-memory-safety`: Defines the language-standard baseline, hardened-standard-library and sanitizer requirements, and the object-lifetime guarding rules that prevent use-after-free bugs of the toast kind.

### Modified Capabilities
<!-- None: no existing capability's requirements change. -->

## Impact

- Build: [`CMakeLists.txt`](../../../CMakeLists.txt) (standard + hardening defines + sanitizer option), [`CMakePresets.json`](../../../CMakePresets.json) (sanitize preset), new `cmake/sanitizers.cmake`, CI workflow in [`.github/workflows`](../../../.github/workflows).
- Conventions: new `docs/cpp26-memory-safety.md` (feature matrix + lifetime rules); a lifetime-guard line added to [`AGENTS.md`](../../../AGENTS.md).
- Code sweep (expected small, bounded by audit): `src/ui/notificationtoast.cpp` already fixed (`1bec0cf`); remaining candidates from the audit are `WA_DeleteOnClose` sites in [`src/lsp/lspeditorbridge.cpp`](../../../src/lsp/lspeditorbridge.cpp:1026), [`src/plugin/plugingallerypanel.cpp`](../../../src/plugin/plugingallerypanel.cpp:346), [`src/plugin/languageworkbenchpluginbase.cpp`](../../../src/plugin/languageworkbenchpluginbase.cpp:137), [`src/bootstrap/agentcallbacksbootstrap.cpp`](../../../src/bootstrap/agentcallbacksbootstrap.cpp:782), plus `DeleteWhenStopped` and member-`deleteLater` sites surfaced by the grep sweep.
- Tests: one lifetime regression test that a sanitizer build flags, one hardened-stdlib bounds test, full `ctest` run under ASan/UBSan as the acceptance gate.
- Plugins: standard bump applies to all plugin targets in [`plugins/CMakeLists.txt`](../../../plugins/CMakeLists.txt); the sweep covers `plugins/` too, but commented-out (disabled) plugins are only audited, never activated.
