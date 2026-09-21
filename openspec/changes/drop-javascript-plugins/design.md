## Context

See [`proposal.md`](proposal.md) for motivation. Current state that shapes the approach:

- The JavaScript plugin runtime is a self-contained plugin at `plugins/javascript-sdk/` (`jspluginsdkplugin.{h,cpp}`, `jspluginruntime.{h,cpp}`, `jshostapi.{h,cpp}`, `plugin.json`, `CMakeLists.txt`). It loads JS plugins from `<app>/plugins/javascript` and exposes a read-only `ex.*` API.
- The LuaJIT engine ([`LuaScriptEngine`](src/sdk/luajit/luascriptengine.h:82)) exposes the same `ex.*` surface plus events, git, diagnostics, and the agent `run_lua` tool, and is the first-class scripting path.
- The `javascriptcore` interface target and `EXORCIST_HAS_JAVASCRIPTCORE` in [`CMakeLists.txt`](CMakeLists.txt) exist only to serve the JS SDK plugin and its test; the devShell provides system JavaScriptCore via `webkitgtk_4_1` ([`flake.nix`](flake.nix:27)).
- `IViewService` / `IHostServices::views()` are **not** JS-only: they are used by [`WorkbenchPluginBase`](src/plugin/workbenchpluginbase.cpp:152) and the C ABI bridge ([`cabi_bridge.cpp`](src/sdk/cabi/cabi_bridge.cpp:12)).
- `plugin_registry.json` lists no JavaScript plugins (only AI providers and Lua samples).

## Goals / Non-Goals

**Goals:**
- Remove the JavaScript plugin runtime, its examples, its test, and all build/devShell wiring.
- Leave Lua as the sole scripting plugin mechanism, with no behavioral change to it.
- Keep `IViewService` and the rest of the host-services surface intact.

**Non-Goals:**
- Porting any JavaScript plugin to Lua (the bundled JS examples duplicate existing Lua examples).
- Changing the Lua engine, its sandbox, or its API.
- Removing `IViewService`/`views()` or other general host services.

## Decisions

### D1: Delete the JavaScript runtime instead of disabling it

Remove `plugins/javascript-sdk/` and its build wiring outright.

- **Why:** The goal is removal, not disabling; a dead runtime and its dependency would preserve the maintenance burden the change exists to eliminate.
- **Alternatives considered:** (a) keep the plugin but stop building it — rejected, leaves dead code and the JavaScriptCore dependency; (b) keep it for VS Code-ecosystem alignment — rejected, the read-only `ex.*` surface is far narrower than the VS Code API and no such consumer exists.

### D2: Remove the `javascriptcore` target and `EXORCIST_HAS_JAVASCRIPTCORE`

Delete the interface target and the variable from `CMakeLists.txt`, and the `javascript-sdk` subdirectory gate in `plugins/CMakeLists.txt`.

- **Why:** Their only consumers are the JS SDK plugin and `test_jspluginruntime`; both are removed.
- **Alternatives considered:** keep the target for future use — rejected as speculative and untested.

### D3: Drop the WebKitGTK devShell dependency

Remove `webkitgtk_4_1` from `flake.nix`.

- **Why:** It was added solely to provide system JavaScriptCore for the headless JS runtime; nothing else uses it.
- **Alternatives considered:** keep it — rejected, it is a large dependency with no remaining consumer.

### D4: Keep `IViewService` and `views()`

Leave the view service and host-services surface unchanged.

- **Why:** They are general plugin infrastructure used by `WorkbenchPluginBase` and the C ABI bridge, not JS-specific.
- **Alternatives considered:** remove them — rejected, would break non-JS plugins.

### D5: Remove JS-plugin-only view contribution types

Delete `JsViewContribution`, `HtmlPluginInfo`, and `LoadedHtmlPlugin` together with the SDK.

- **Why:** They exist only to describe HTML/JS plugin views and have no other consumer.

### D6: Remove the bundled JavaScript examples

Delete `plugins/javascript/{hello-world,word-count,file-stats}`.

- **Why:** They cannot run without the runtime and duplicate existing Lua examples (`plugins/lua/hello-world`, `word-count`, `file-stats`).

## Risks / Trade-offs

- [JavaScript plugin authors lose the runtime] → Accepted and documented as erased functionality; Lua is the supported scripting path and covers the same surface plus more.
- [Leftover `javascript-sdk` / `javascriptcore` / `EXORCIST_HAS_JAVASCRIPTCORE` references break the build] → Grep the whole tree and remove every reference; build and run the full test suite.
- [Stale devShell comment or docs] → Update the `flake.nix` comment; verified no dedicated JS-plugin docs exist.
- [`plugin_registry.json` or host services accidentally touched] → Verified no JS entries in the registry; `IViewService` is retained.

## Migration Plan

1. Delete the JS SDK plugin, the JS examples, and `test_jspluginruntime`; remove the `javascriptcore` target, `EXORCIST_HAS_JAVASCRIPTCORE`, the `javascript-sdk` subdirectory, and the WebKitGTK devShell dependency.
2. Build (`nix develop --command cmake --build build-tests -j4`) and run the full test suite headless.
3. Rollback: revert the change commit; no data or settings migration is involved.

## Open Questions

None.