## Context

See [`proposal.md`](proposal.md) for motivation. Current state that shapes the approach:

- The chat panel has two mutually exclusive implementations selected at compile time by `EXORCIST_HAS_ULTRALIGHT` in [`ChatPanelWidget`](src/agent/chat/chatpanelwidget.cpp:137): the Ultralight HTML path (`UltralightWidget` + `ChatJSBridge`) and the Qt widget path (`ChatTranscriptView`, `ChatWelcomeWidget`, `ChatInputWidget`, `ChatMarkdownWidget`, `ChatToolInvocationWidget`, `ChatFollowupsWidget`, `ChatThinkingWidget`, `ChatWorkspaceEditWidget`).
- [`CMakeLists.txt`](CMakeLists.txt:20) declares `option(EXORCIST_USE_ULTRALIGHT ... ON)` — the Ultralight path is the **default** build, despite [`docs/dependencies.md`](docs/dependencies.md:9) claiming "OFF by default".
- The `javascriptcore` interface target aliases the Ultralight SDK when `EXORCIST_USE_ULTRALIGHT` is ON, and falls back to system JavaScriptCore (WebKitGTK) otherwise ([`CMakeLists.txt`](CMakeLists.txt:183)). The headless JS plugin runtime needs only the JavaScriptCore C API.
- Other Ultralight consumers: the HTML agent dashboard ([`AgentDashboardPanel`](src/agent/ui/agentdashboardpanel.cpp:28) + `DashboardJSBridge`), and HTML-based JS plugin views ([`UltralightPluginView`](plugins/javascript-sdk/ultralightpluginview.h:30)).
- [`AGENTS.md`](AGENTS.md:41) normally excludes the Ultralight UI surfaces from proposals; the user explicitly authorized removing them for this change.

## Goals / Non-Goals

**Goals:**
- Make the Qt widget path the sole chat renderer, with no compile-time switch.
- Remove the Ultralight SDK from the build graph entirely (option, subdirectory, target, FetchContent, runtime deploy).
- Keep the headless JS plugin runtime building and passing tests using system JavaScriptCore.
- Remove all Ultralight-rendered surfaces and assets, including the normally-excluded `src/resources/chat/*`.

**Non-Goals:**
- Porting the HTML dashboard or HTML plugin views to Qt (they are removed, not reimplemented).
- Achieving pixel/feature parity for web-only chat presentation details.
- Changing provider, auth, or agent behavior.

## Decisions

### D1: Delete the Ultralight branch instead of defaulting the option OFF

Remove the `EXORCIST_USE_ULTRALIGHT` option and the `#ifdef EXORCIST_HAS_ULTRALIGHT` branches, leaving only the Qt widget code.

- **Why:** The goal is removal, not disabling. Keeping a dead flag and dual code paths preserves the maintenance burden the change exists to eliminate.
- **Alternatives considered:** (a) flip the option to OFF by default — rejected, leaves dead code and the SDK in the tree; (b) keep the option for downstream users — rejected, no known consumers.

### D2: Remove the Ultralight SDK and all its consumers together

Delete `cmake/ultralight/`, the `ultralight-sdk` target, the `FetchContent` download, `ultralight_deploy_runtime`, and the `ULTRALIGHT_ROOT` export, together with the dashboard and plugin-view consumers.

- **Why:** The SDK has no remaining consumer once the chat renderer, dashboard, and plugin views are removed; leaving it would keep a large external download in the build.
- **Alternatives considered:** keep the SDK for the dashboard/plugin views — rejected because those are also Ultralight-only and are being removed.

### D3: Headless JS runtime uses system JavaScriptCore unconditionally

Replace the conditional `javascriptcore` target with a direct dependency on the system JavaScriptCore (WebKitGTK, `javascriptcoregtk-4.1`), which [`flake.nix`](flake.nix:29) already provides.

- **Why:** The headless runtime only needs the JavaScriptCore C API; the Ultralight SDK was one of two providers. Removing the SDK leaves the system provider as the only option.
- **Alternatives considered:** vendor JavaScriptCore — rejected as unnecessary; the devShell already supplies it.

### D4: Remove HTML-based JS plugin views

Delete `plugins/javascript-sdk/ultralightpluginview.{h,cpp}` and the `EXORCIST_HAS_ULTRALIGHT` guards in [`jspluginsdkplugin.cpp`](plugins/javascript-sdk/jspluginsdkplugin.cpp:3). JS plugins retain the headless runtime only.

- **Why:** The HTML view is Ultralight-only; there is no other renderer for it.
- **Consequence:** JS plugins lose custom HTML/WebView UI — recorded as erased functionality in the proposal.

### D5: Remove the rich HTML dashboard, keep the QWidget fallback

Delete `src/resources/dashboard/*` and the Ultralight branch of [`AgentDashboardPanel`](src/agent/ui/agentdashboardpanel.cpp:28); the existing QWidget fallback (labels + lists) becomes the only dashboard.

- **Why:** The HTML dashboard is Ultralight-only; the fallback already exists and is wired through `AgentUIBus`.
- **Alternatives considered:** port the dashboard HTML to Qt — out of scope for this change.

### D6: Remove the normally-excluded UI surfaces

Delete `src/resources/chat/chat.html`, `chat.js`, `chat.css`, `chat_new.css`, `markdown.js` and their `resources.qrc` entries, overriding the [`AGENTS.md`](AGENTS.md:41) exclusion.

- **Why:** The user explicitly authorized removing the Ultralight UI surfaces for this change; leaving orphaned assets would contradict the removal.

## Risks / Trade-offs

- [Qt path lacks web-only presentation details (CSS shimmer, SVG tool cards, `::selection`, JS input history)] → Accepted and documented as erased functionality; the Qt path covers the functional equivalents.
- [JS plugins lose all UI rendering] → Accepted and documented; the headless runtime and its tests are unaffected.
- [Dashboard loses its rich HTML presentation] → Accepted; the QWidget fallback remains functional.
- [Leftover `EXORCIST_HAS_ULTRALIGHT` / `EXORCIST_USE_ULTRALIGHT` references break the build] → Grep the whole tree and remove every reference; build and run the full test suite.
- [`resources.qrc` still references deleted files] → Update the qrc in the same change; verify the resource bundle builds.
- [Stale comments referencing `UltralightWidget` in `ExDockWidget.cpp` / `dockbootstrap.cpp`] → Update the comments to avoid misleading future readers.
- [Docs still describe Ultralight] → Update `docs/ai.md`, `docs/dependencies.md`, `docs/roadmap.md`, `docs/todolist.md`, `docs/project_audit_working.md`.

## Migration Plan

1. Remove the Ultralight source, assets, and build wiring; switch the JS runtime to system JavaScriptCore.
2. Build (`nix develop --command cmake --build build-tests -j4`) and run the full test suite headless.
3. Rollback: revert the change commit; no data or settings migration is involved.

## Open Questions

None.