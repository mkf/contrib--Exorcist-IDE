## 1. Remove the Ultralight chat renderer

- [x] 1.1 Delete `src/agent/chat/ultralight/` (`ultralightengine.*`, `ultralightwidget.*`, `chatjsbridge.*`) and verify no source references remain with a tree-wide grep for `UltralightEngine`, `UltralightWidget`, and `ChatJSBridge`.
- [x] 1.2 Remove the `EXORCIST_HAS_ULTRALIGHT` branches from `src/agent/chat/chatpanelwidget.{h,cpp}`, keeping the Qt widget path as the only implementation; verify the file contains no Ultralight references and compiles.

## 2. Remove Ultralight UI assets

- [x] 2.1 Delete `src/resources/chat/chat.html`, `chat.js`, `chat.css`, `chat_new.css`, `markdown.js` and remove their `src/resources.qrc` entries; verify the resource bundle builds.
- [x] 2.2 Delete `src/resources/dashboard/dashboard.html`, `dashboard.css`, `dashboard.js` and remove their `src/resources.qrc` entries; verify the resource bundle builds.

## 3. Remove dashboard and plugin-view Ultralight paths

- [x] 3.1 Remove the Ultralight branch from `src/agent/ui/agentdashboardpanel.{h,cpp}` and `src/agent/ui/dashboardjsbridge.{h,cpp}`, keeping the QWidget fallback; verify no `EXORCIST_HAS_ULTRALIGHT` references remain in these files.
- [x] 3.2 Delete `plugins/javascript-sdk/ultralightpluginview.{h,cpp}` and remove the `EXORCIST_HAS_ULTRALIGHT` guards from `plugins/javascript-sdk/jspluginsdkplugin.cpp`; verify the plugin builds.

## 4. Remove build wiring and switch the JS runtime

- [x] 4.1 Remove the `EXORCIST_USE_ULTRALIGHT` option and the `cmake/ultralight` subdirectory from `CMakeLists.txt`; verify CMake configure succeeds.
- [x] 4.2 Make the `javascriptcore` target use system JavaScriptCore (WebKitGTK) unconditionally in `CMakeLists.txt`; verify the headless JS plugin runtime builds.
- [x] 4.3 Remove the Ultralight sources, `EXORCIST_HAS_ULTRALIGHT` definition, `ultralight-sdk` link, and `ultralight_deploy_runtime` call from `src/CMakeLists.txt`; verify the `exorcist` target builds.
- [x] 4.4 Remove the Ultralight sources, link, and definition from `plugins/javascript-sdk/CMakeLists.txt`; verify the plugin builds.
- [x] 4.5 Remove the `test_ultralightwidget` target from `tests/CMakeLists.txt` and delete `tests/test_ultralightwidget.cpp`; verify `ctest -N` lists no `UltralightWidget` test.

## 5. Update docs and stale comments

- [x] 5.1 Update `docs/ai.md`, `docs/dependencies.md`, `docs/roadmap.md`, `docs/todolist.md`, and `docs/project_audit_working.md` to drop Ultralight; verify no Ultralight references remain in those files.
- [x] 5.2 Update the stale `UltralightWidget` comments in `src/ui/dock/ExDockWidget.cpp` and `src/bootstrap/dockbootstrap.cpp`; verify the comments no longer reference Ultralight.

## 6. Verify the change

- [x] 6.1 Build with `nix develop --command cmake --build build-tests -j4`; verify it succeeds.
- [x] 6.2 Run `nix develop --command bash -c 'cd build-tests && QT_QPA_PLATFORM=offscreen ctest -j4 --output-on-failure'`; verify all tests pass.
- [x] 6.3 Grep the tree for `ultralight`, `EXORCIST_USE_ULTRALIGHT`, and `EXORCIST_HAS_ULTRALIGHT`; verify no functional references remain outside `openspec/changes/archive/`.