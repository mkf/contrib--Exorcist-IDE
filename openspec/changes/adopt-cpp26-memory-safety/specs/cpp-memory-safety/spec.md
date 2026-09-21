## Purpose

Defines the compiler-standard baseline, standard-library hardening, sanitizer testing, and QObject lifetime-guarding rules that make the memory-safety bug class behind the toast SEGFAULT structurally detectable instead of hand-debugged.

## ADDED Requirements

### Requirement: C++26 language baseline with verified fallback

The project SHALL build all first-party targets (host app, plugins, tests) with the C++26 standard when the toolchain supports it, and SHALL provide a documented, single-point fallback to the last-good standard if Qt or MOC proves incompatible.

#### Scenario: Building with C++26

- **WHEN** the project is configured in the Nix devShell (GCC 15.3, Qt 6.11.2)
- **THEN** all first-party targets SHALL compile with `-std=gnu++26`
- **AND** the full ctest suite SHALL pass without standard-related regressions

#### Scenario: Falling back on toolchain incompatibility

- **WHEN** a Qt, MOC, or plugin incompatibility under C++26 cannot be resolved
- **THEN** a single CMake cache option SHALL lower the language standard for all first-party targets at once
- **AND** the chosen value and the blocking incompatibility SHALL be recorded in `docs/cpp26-memory-safety.md`

### Requirement: Hardened C++ standard library

Development and test builds SHALL enable libstdc++ hardening so standard-library bounds and pointer checks abort deterministically instead of continuing with corrupted state.

#### Scenario: Out-of-bounds container access in a hardened build

- **WHEN** code violates a hardening-checked constraint of the C++ standard library in a Debug or RelWithDebInfo build
- **THEN** the program SHALL abort with the hardening violation instead of silently continuing

### Requirement: Sanitizer-clean test suite

The project SHALL provide an AddressSanitizer plus UndefinedBehaviorSanitizer build configuration, and the full ctest suite SHALL pass under it with zero findings.

#### Scenario: Running tests under sanitizers

- **WHEN** the sanitizer build configuration is configured and the full ctest suite runs
- **THEN** all tests SHALL pass
- **AND** no ASan or UBSan findings SHALL be reported

#### Scenario: Detecting a toast-style use-after-free

- **WHEN** code dereferences a member pointer to a QObject that has already self-deleted
- **THEN** the sanitizer build SHALL report a use-after-free at the dereference site

### Requirement: No raw member pointers to policy-managed QObjects

A class member SHALL NOT hold a raw pointer to a QObject whose lifetime is managed by policy (`DeleteWhenStopped`, `WA_DeleteOnClose`, `deleteLater()`, self-delete); such members SHALL use `QPointer`, a `destroyed()`-based reset, or explicit manual lifetime ownership.

#### Scenario: Member-tracked self-deleting animation

- **WHEN** a class starts a `QAbstractAnimation` with `DeleteWhenStopped` and tracks it in a member
- **THEN** that member SHALL be a `QPointer` or SHALL be reset via the animation's `destroyed()` signal
- **AND** no code path SHALL call methods on the member after the animation has self-deleted

#### Scenario: Audit of self-deleting object sites

- **WHEN** the lifetime audit sweeps `src/` and `plugins/` for `DeleteWhenStopped`, `WA_DeleteOnClose`, member `deleteLater`, and `delete this` patterns
- **THEN** every site SHALL be recorded in `docs/cpp26-memory-safety.md` as compliant or fixed
