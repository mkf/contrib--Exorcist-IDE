## Why

The generic AI assistant surface is labeled "Copilot" throughout the UI (chat panel title, welcome screen, editor menu, auth status indicator, assistant name in messages). This implies the assistant *is* GitHub Copilot even when the active provider is Claude, BYOK, or any other backend, which misleads users and misrepresents third-party providers. Only the genuine GitHub Copilot provider plugin — which really authenticates against GitHub and calls `api.githubcopilot.com` — should carry Copilot branding.

## What Changes

- Replace generic, provider-neutral "Copilot" user-visible wording with neutral product wording:
  - **"Exorcist AI"** for the chat panel/session title and the assistant name shown on messages.
  - **"AI Assistant"** for generic references (welcome heading, sign-in prompt, editor menu).
- Affected user-visible surfaces: chat panel session title, chat welcome heading/subtitle, sign-in-required prompt, editor context-menu AI submenu, auth status indicator label, and the assistant name rendered in chat turns.
- **Keep** genuine GitHub Copilot product branding on the real provider: `plugins/copilot/plugin.json` name/description, the provider's `displayName()`, the "Sign in to GitHub Copilot" auth dialog, its settings page title, and the bundled plugin registry entry.
- **No change** to internal identifiers, class/file names, `QSettings` keys, logging categories, API endpoints/headers, or GitHub-compatible instruction file paths (`.github/copilot-instructions.md`, `.copilot-*-instructions.md`).
- Out of scope: `docs/`, `README.md`, `.github/` (repo-facing, not in-app UI); the Ultralight web chat (`src/resources/chat/chat.html`, `chat.js`), which project convention excludes from proposals; and the dead-code `src/agent/agentchatpanel.*` (not built or loaded).

## Capabilities

### New Capabilities
- `ai-assistant-branding`: Defines the user-visible naming of the provider-neutral AI assistant surface, and the boundary that reserves GitHub Copilot branding for the genuine GitHub Copilot provider only.

### Modified Capabilities
<!-- None: no existing capability's requirements change. -->

## Impact

- UI code: `src/agent/chat/chatpanelwidget.cpp`, `src/agent/chat/chatwelcomewidget.cpp`, `src/agent/chat/chatturnwidget.cpp`, `src/agent/authstatusindicator.cpp`, `src/editor/editorview.cpp`.
- Translations: `translations/` (new source strings).
- Unchanged: `plugins/copilot/*` product branding, internal identifiers, settings keys, endpoints, instruction-file compatibility paths, and the Ultralight web chat.