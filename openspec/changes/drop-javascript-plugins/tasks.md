## 1. Remove the JavaScript plugin runtime and examples

- [ ] 1.1 Delete `plugins/javascript-sdk/` (`jspluginsdkplugin.{h,cpp}`, `jspluginruntime.{h,cpp}`, `jshostapi.{h,cpp}`, `plugin.json`, `CMakeLists.txt`) and verify no source references remain with a tree-wide grep for `JsPluginRuntime`, `JsPluginSdkPlugin`, and `jshostapi`.
- [ ] 1.2 Delete the bundled JavaScript plugin examples `plugins/javascript/{hello-world,word-count,file-stats}` and verify the `plugins/javascript/` directory is gone.

## 2. Remove build wiring and the devShell dependency

- [ ] 2.1 Remove the `javascriptcore` interface target and the `EXORCIST_HAS_JAVASCRIPTCORE` variable from `CMakeLists.txt`; verify CMake configure succeeds.
- [ ] 2.2 Remove the `javascript-sdk` subdirectory (and its `EXORCIST_HAS_JAVASCRIPTCORE` gate) from `plugins/CMakeLists.txt`; verify the plugins configure.
- [ ] 2.3 Remove the `test_jspluginruntime` target from `tests/CMakeLists.txt` and delete `tests/test_jspluginruntime.cpp`; verify `ctest -N` lists no `JsPluginRuntime` test.
- [ ] 2.4 Remove the `webkitgtk_4_1` package and its JavaScriptCore comment from `flake.nix`; verify the devShell evaluates.

## 3. Verify the change

- [ ] 3.1 Build with `nix develop --command cmake --build build-tests -j4`; verify it succeeds.
- [ ] 3.2 Run `nix develop --command bash -c 'cd build-tests && QT_QPA_PLATFORM=offscreen ctest -j4 --output-on-failure'`; verify all tests pass.
- [ ] 3.3 Grep the tree for `javascript-sdk`, `javascriptcore`, `EXORCIST_HAS_JAVASCRIPTCORE`, and `JsPluginRuntime`; verify no functional references remain outside `openspec/changes/archive/`.