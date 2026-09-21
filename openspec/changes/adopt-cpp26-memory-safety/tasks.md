## 1. Language baseline

- [ ] 1.1 Introduce an `EXO_CXX_STANDARD` cache option (default `26`) in the root [`CMakeLists.txt`](../../../CMakeLists.txt) feeding `CMAKE_CXX_STANDARD` for host, plugins, and tests; configure and build all targets with `nix develop --command cmake --build build-tests -j4`; verify the build succeeds with `-std=gnu++26`.
- [ ] 1.2 Run the full suite with `nix develop --command bash -c 'cd build-tests && QT_QPA_PLATFORM=offscreen ctest -j4 --output-on-failure'`; verify all tests pass; if an unresolvable Qt/MOC/plugin incompatibility appears, apply the fallback ladder from design (`20`, then `17`) and record the applied value and blocker in `docs/cpp26-memory-safety.md`.
- [ ] 1.3 Add a small probe test (or static assertion translation unit) exercising C++23/26 headers the code will rely on, e.g. `<expected>`; verify it compiles, links, and passes under `ctest`.

## 2. Hardened standard library

- [ ] 2.1 Add `_GLIBCXX_HARDENING_MODE=1` via `target_compile_definitions` to first-party Debug and RelWithDebInfo targets; rebuild and inspect `build-tests/compile_commands.json`; verify the definition is present for host, plugin, and test targets and absent from Release.
- [ ] 2.2 Add a unit test documenting hardening behavior for a bounds-violating access path (asserting the deterministic abort/exception semantics observed with GCC 15.3's hardened libstdc++); verify the test passes and its expectations match reality.

## 3. Sanitizers

- [ ] 3.1 Add an `EXO_SANITIZE` option plus `cmake/sanitizers.cmake` injecting `-fsanitize=address,undefined` compile and link flags, and a `sanitize` configure/build preset in [`CMakePresets.json`](../../../CMakePresets.json); verify a separate build directory configures and builds with the preset.
- [ ] 3.2 Run the full `ctest` suite in the sanitizer build with `QT_QPA_PLATFORM=offscreen`, `ASAN_OPTIONS=abort_on_error=1`, `UBSAN_OPTIONS=halt_on_error=1`; triage every finding to zero (fixing real lifetime bugs it surfaces); verify a clean run.
- [ ] 3.3 Add a CI workflow job that builds the sanitizer configuration in the Nix devShell and runs `ctest`; verify the job definition mirrors the local commands and fails on any finding.

## 4. Lifetime audit and fixes

- [ ] 4.1 Sweep `src/` and `plugins/` for `DeleteWhenStopped`, `WA_DeleteOnClose`, member `deleteLater`, and `delete this` patterns; record every site in an audit table in `docs/cpp26-memory-safety.md` as compliant or fix-needed with the chosen guard form; verify the table covers all grep hits.
- [ ] 4.2 Convert each violating member pointer per rule R1 (`QPointer`, `destroyed()`-reset, or manual lifetime in the `1bec0cf` style); verify the project builds and every touched code path's existing tests still pass.
- [ ] 4.3 Add a Qt Test regressing the toast lifetime pattern: track a `DeleteWhenStopped` animation, let it self-delete, then run the dismiss path against the guarded member; verify no use-after-free occurs, the guard observed the deletion, and the test passes in both normal and sanitizer builds.
- [ ] 4.4 Add rule R1 to the conventions section of [`AGENTS.md`](../../../AGENTS.md) and reference it from `docs/cpp26-memory-safety.md`; verify the wording matches the design decision.

## 5. Verification

- [ ] 5.1 Full build and `ctest` at the applied standard with hardening enabled; verify green.
- [ ] 5.2 Sanitizer `ctest` run; verify zero findings and that the deliberate-bug probes from 2.2 and 4.3 behave as documented.
- [ ] 5.3 Confirm `git status` shows no modifications under `src/resources/chat/` (Ultralight-rendered surfaces) or `third_party/`; verify via `git status` and record the check in the change summary.
