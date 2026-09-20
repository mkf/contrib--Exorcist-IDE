## Why

The chat welcome screen shows a hardcoded **"Sign In with GitHub"** button for *any* provider that is not available, and clicking it just re-runs `initialize()` — a no-op for API-key providers. Meanwhile, selecting GitHub Copilot from the provider dropdown immediately launches the GitHub OAuth dialog, so the button is both wrong and redundant. Each provider needs its own, working authentication affordance, and selecting a provider must never start an interactive login on its own.

## What Changes

- **No implicit auth on selection**: choosing a provider from the dropdown (or restoring the saved provider at startup) SHALL NOT start an interactive authentication flow. The provider becomes active; if it needs credentials, the welcome screen presents the auth action instead.
- **Provider-specific auth action**: the welcome auth state SHALL show a primary button whose label and behavior come from the active provider:
  - **GitHub Copilot** → `Sign in with GitHub` (OAuth device flow, started on click).
  - **OpenRouter** (BYOK preset) → `Sign in with OpenRouter` (OAuth PKCE with a localhost callback, started on click).
  - **Claude** → `Get an Anthropic API Key` (opens `https://console.anthropic.com/settings/keys`).
  - **OpenAI** (BYOK preset) → `Get an OpenAI API Key` (opens `https://platform.openai.com/api-keys`).
  - **Z.ai** (BYOK preset) → `Get a Z.ai API Key` (opens `https://z.ai/manage-apikey/apikey-list`).
  - **Custom** (BYOK preset) → `Open Settings to Paste API Key` (opens settings; no public key page exists).
- **Secondary link**: below the button, a small link `or open settings to paste your API key` opens the provider's key-entry surface (the AI settings dialog, or the provider's own settings command such as `claude.editApiKey`).
- **Provider auth contract**: `IAgentProvider` gains an auth descriptor (action kind, label, URL, settings command) and a `startAuth()` entry point, so the UI no longer hardcodes GitHub.
- **Copilot provider**: `initialize()` no longer launches OAuth; it only reports availability. OAuth moves behind `startAuth()`.
- **OpenRouter OAuth**: the BYOK provider implements the OpenRouter PKCE flow (browser + localhost callback) and stores the returned key as the OpenRouter preset's API key.
- Out of scope: the Ultralight web chat (`src/resources/chat/chat.html`, `chat.js`), which project convention excludes; and the unused `ErrorStateWidget::showSignInPrompt()`.

## Capabilities

### New Capabilities
- `ai-provider-auth`: Defines how the active AI provider advertises and performs its authentication action, and how the chat surface presents it without starting auth implicitly.

### Modified Capabilities
<!-- None: no existing capability's requirements change. -->

## Impact

- Provider contract: `src/aiinterface.h` (`IAgentProvider` auth descriptor + `startAuth()`).
- Providers: `plugins/copilot/copilotprovider.*` (defer OAuth), `plugins/byok/byokprovider.*` (per-preset auth + OpenRouter PKCE), `plugins/claude/claudeprovider.*` (auth descriptor).
- Chat UI: `src/agent/chat/chatwelcomewidget.*` (provider-specific button + link), `src/agent/chat/chatpanelwidget.cpp` (dispatch auth action, no implicit auth), `src/agent/chat/chatturnwidget.*` / `chattranscriptview.*` (auth-error button routes to the provider action).
- Host wiring: `src/mainwindow.cpp` (settings link → settings dialog or provider settings command).
- Tests: new coverage for the auth descriptor and welcome-state rendering.