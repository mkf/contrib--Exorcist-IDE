## Why

The Ultralight (WebKit-fork) HTML renderer is an optional, heavyweight third-party SDK that duplicates the Qt-widget chat UI and creates a second, divergent rendering path. The project is consolidating on the Qt widget chat surface, so the duplicate renderer, its SDK fetch/deploy machinery, and its maintenance burden should be removed.

## What Changes

- **BREAKING**: Remove the Ultralight HTML chat renderer: `UltralightEngine`, `UltralightWidget`, `ChatJSBridge` (`src/agent/chat/ultralight/*`).
- **BREAKING**: Remove the Ultralight-rendered chat UI assets: `src/resources/chat/chat.html`, `chat.js`, `chat.css`, `chat_new.css`, `markdown.js`, and their `resources.qrc` entries.
- **BREAKING**: Remove the Ultralight HTML agent dashboard path: `src/resources/dashboard/*` and the Ultralight branch of `AgentDashboardPanel`; the QWidget fallback becomes the only dashboard.
- **BREAKING**: Remove HTML-based JS plugin views: `plugins/javascript-sdk/ultralightpluginview.*`; JS plugins keep only the headless runtime.
- Remove the `EXORCIST_USE_ULTRALIGHT` CMake option, `cmake/ultralight/`, the `ultralight-sdk` target, SDK `FetchContent`, and runtime deploy.
- Remove `EXORCIST_HAS_ULTRALIGHT` compile-time branches from `ChatPanelWidget`, `AgentDashboardPanel`, `DashboardJSBridge`, and `jspluginsdkplugin`.
- Make the headless JS plugin runtime use the system JavaScriptCore (WebKitGTK) unconditionally.
- Remove `tests/test_ultralightwidget.cpp` and its CMake target.
- Update docs (`docs/ai.md`, `docs/dependencies.md`, `docs/roadmap.md`, `docs/todolist.md`, `docs/project_audit_working.md`) to drop Ultralight.

### Functionality erased that exists nowhere else

The Qt widget chat path already covers the functional equivalents of most web-UI chat features (markdown, tool cards, welcome states, slash/mention popups, changes bar, followups, thinking). The following capabilities are **not** available anywhere else and are lost outright:

- **HTML-based JS plugin views** (`UltralightPluginView`): the only mechanism for JS plugins to render custom HTML/CSS/JS UI. After removal, JS plugins have no UI rendering path at all — only the headless runtime remains.
- **Rich HTML agent dashboard** (`dashboard.html`/`dashboard.css`/`dashboard.js`): the dark-themed dashboard with steps, metrics, logs, and artifacts. The QWidget fallback is basic (labels + lists), so the rich dashboard presentation is lost.
- **The web-based chat rendering surface itself** (`chat.html`/`chat.js`/`chat.css`/`markdown.js`): the entire HTML/CSS/JS chat UI. The Qt path is a parallel implementation, but web-only presentation details (CSS streaming shimmer, SVG tool cards, `::selection` styling, JS input-history navigation) have no Qt equivalent.
- **The Ultralight renderer/SDK integration**: the CPU-rendered WebKit fork (`BitmapSurface`→`QImage`) and its `ultralight-resources` runtime.

## Capabilities

### New Capabilities
- `chat-rendering`: the chat panel is rendered exclusively by the Qt widget transcript path; no HTML/JS renderer exists.

### Modified Capabilities
<!-- None: no existing spec's requirements change. -->

## Impact

- **Code**: `src/agent/chat/ultralight/*`, `src/agent/chat/chatpanelwidget.{h,cpp}`, `src/agent/ui/agentdashboardpanel.{h,cpp}`, `src/agent/ui/dashboardjsbridge.{h,cpp}`, `src/resources/chat/*`, `src/resources/dashboard/*`, `src/resources.qrc`, `plugins/javascript-sdk/*`, `CMakeLists.txt`, `src/CMakeLists.txt`, `plugins/CMakeLists.txt`, `tests/CMakeLists.txt`, `flake.nix`.
- **Dependencies**: drops the Ultralight SDK (fetch + runtime deploy); the headless JS plugin runtime now requires system JavaScriptCore (`webkitgtk_4_1`) unconditionally.
- **Scope note**: this change explicitly overrides the [`AGENTS.md`](../../../AGENTS.md) rule that excludes the Ultralight UI surfaces (`src/resources/chat/chat.html`, `chat.js`, and other Ultralight-rendered surfaces) from proposals. The user authorized removing those surfaces for this change.