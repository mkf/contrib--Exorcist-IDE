## Why

The JavaScript plugin runtime duplicates the LuaJIT scripting engine with a narrower, read-only API and no capability of its own — its only unique feature, HTML UI rendering, was removed with Ultralight. Maintaining two sandboxed scripting runtimes adds build, dependency, and test surface for no functional gain.

## What Changes

- **BREAKING**: Remove the JavaScript plugin runtime plugin (`plugins/javascript-sdk/*`: `jspluginsdkplugin.{h,cpp}`, `jspluginruntime.{h,cpp}`, `jshostapi.{h,cpp}`, `plugin.json`, `CMakeLists.txt`).
- **BREAKING**: Remove the bundled JavaScript plugin examples (`plugins/javascript/*`: `hello-world`, `word-count`, `file-stats`).
- Remove the `javascriptcore` interface target and the `EXORCIST_HAS_JAVASCRIPTCORE` variable from `CMakeLists.txt`, and drop the `javascript-sdk` subdirectory from `plugins/CMakeLists.txt`.
- Remove the `test_jspluginruntime` target and `tests/test_jspluginruntime.cpp`.
- Drop the system JavaScriptCore (WebKitGTK) devShell dependency from `flake.nix`.
- Remove the JS-plugin-only view-contribution types (`JsViewContribution`, `HtmlPluginInfo`, `LoadedHtmlPlugin`) that exist solely for the JS SDK.
- Lua scripting plugins are unaffected and remain the sole scripting plugin mechanism.

## Capabilities

### New Capabilities
- `plugin-scripting`: scripting plugins are Lua-only; the system ships no JavaScript plugin runtime, no JavaScriptCore dependency, and no JavaScript plugin artifacts.

### Modified Capabilities
<!-- None: no existing spec's requirements change. -->

## Impact

- **Code**: `plugins/javascript-sdk/*`, `plugins/javascript/*`, `plugins/CMakeLists.txt`, `CMakeLists.txt`, `tests/CMakeLists.txt`, `tests/test_jspluginruntime.cpp`, `flake.nix`.
- **Dependencies**: drops the system JavaScriptCore provider (`webkitgtk_4_1`) from the devShell; the LuaJIT engine is unaffected.
- **Erased functionality**: JavaScript plugins (headless, read-only command/editor/workspace automation). This is fully covered by the Lua engine, which exposes the same `ex.*` surface plus events, git, diagnostics, and the agent `run_lua` tool.
- **Unaffected**: `IViewService`/`IHostServices::views()` remain (used by `WorkbenchPluginBase` and the C ABI bridge); `plugin_registry.json` lists no JavaScript plugins.