# AGENTS.md

Tooling protocol: see [`CLAUDE.md`](CLAUDE.md) (source graph; no blind grep/read).
Below are the non-obvious facts worth knowing up front.

## Environment
- NixOS + flake devShell. `cmake`, `ninja`, `gcc`, `qt6`, `gdb`, `python3` exist
  **only** inside it — bare `cmake`/`python3` fail with "command not found".
- Prefix toolchain commands with `nix develop --command …`.
- `openspec` CLI is not in the flake: `nix shell nixpkgs#openspec -c openspec …`.
- Noise is normal: nix lockfile/xorg warnings, `Exorcist dev shell: …`.

## Build / test
- Use the existing `build-tests/` tree. Binary: `build-tests/src/exorcist`.
- Build: `nix develop --command cmake --build build-tests -j4`
  (or `--target exorcist` / `--target byok_plugin`).
- Test: `nix develop --command bash -c 'cd build-tests && QT_QPA_PLATFORM=offscreen ctest -j4 --output-on-failure'`.
- Tests are Qt Test (global AUTOMOC); add a target in `tests/CMakeLists.txt`,
  end the file with `QTEST_MAIN(Class)` + `#include "<name>.moc"`.
- The app runs standalone (Qt via RPATH); set `QT_QPA_PLATFORM=offscreen` headless,
  `xcb` for a window.

## Gotchas
- `plugins/CMakeLists.txt` **comments out** plugins (`# disabled — Copilot-first`).
  A plugin dir existing ≠ built/loaded. Check that file before assuming a
  provider is active.
- `src/agent/agentchatpanel.*` is **dead code**. The active AI dock is
  `src/agent/chat/chatpanelwidget.cpp` (see `CLAUDE.md`). Don't build on
  `AgentChatPanel`.
- Provider contract: `IAgentProvider` in `src/aiinterface.h`; plugins expose via
  `IAgentPlugin::createProviders()`. Settings are plain `QSettings`.
- Header-only shared code is used when host + plugin DSO both need it (avoids
  link deps) — e.g. `src/agent/openaicompatpresets.h`.
- In tests, qualify free functions that share a name with a test slot.

## VS Code
- Tasks in `.vscode/tasks.json` wrap commands in `nix develop --command`
  (required). Debug `miDebuggerPath` is a version-specific Nix store path;
  refresh with `nix develop --command which gdb` after a devShell bump.

## Scope rules
- **Never touch the Ultralight UI** — `src/resources/chat/chat.html`,
  `src/resources/chat/chat.js`, and any other Ultralight-rendered surface.
  Exclude it from every proposal/change (no edits, not merely no wording) and
  record it as out of scope; it is handled separately.

## Conventions
- Commit prefixes: `ai:`, `dev:`, `openspec:`.
- When told to commit, commit the openspec artifacts before or (if per actual chronology) after the "apply" implementation result
- OpenSpec planning lives in `openspec/changes/<name>/`; run its CLI per above.