## Context

See [`proposal.md`](proposal.md) for motivation. Current state that shapes the approach:

- [`AgentProviderRegistry::setActiveProvider()`](src/agent/agentproviderregistry.cpp:105) calls `initialize()` on the newly active provider. [`CopilotProvider::initialize()`](plugins/copilot/copilotprovider.cpp:208) launches the OAuth dialog when no token exists — the source of the implicit login.
- The welcome auth state hardcodes `Sign In with GitHub` in [`chatwelcomewidget.cpp`](src/agent/chat/chatwelcomewidget.cpp:199) and emits `signInRequested`, which the panel handles by calling `active->initialize()` again ([`chatpanelwidget.cpp`](src/agent/chat/chatpanelwidget.cpp:554)).
- `IAgentProvider` ([`aiinterface.h`](src/aiinterface.h:222)) has no auth-action concept; `initialize()` is the only entry point.
- Providers and their auth mechanisms: Copilot = GitHub OAuth device flow; Claude = API key (`claude.editApiKey` command, `ANTHROPIC_API_KEY`); BYOK = per-preset API key (OpenAI, Z.ai, OpenRouter, Custom). Only Copilot and OpenRouter have a usable browser OAuth flow.
- The chat panel already emits `settingsRequested`, wired in [`mainwindow.cpp`](src/mainwindow.cpp:500) to open the AI settings dialog.
- The Ultralight web chat is out of scope by project convention (see [`AGENTS.md`](../../../AGENTS.md)).

## Goals / Non-Goals

**Goals:**

- Make provider selection side-effect-free with respect to authentication.
- Give every provider a correct, working auth affordance driven by provider-supplied metadata.
- Add a real OpenRouter OAuth PKCE flow.
- Keep the secondary "paste your API key" path working for every provider.

**Non-Goals:**

- Implementing OAuth for providers that do not offer it (Anthropic, OpenAI, Z.ai, Custom).
- Touching the Ultralight web chat or the unused `ErrorStateWidget::showSignInPrompt()`.
- Redesigning the settings dialog beyond what the secondary link needs.

## Decisions

### Decision: Auth descriptor on `IAgentProvider`

Add to `IAgentProvider`:

```cpp
enum class AuthAction { None, OAuth, OpenUrl, OpenSettings };
struct ProviderAuthInfo {
    AuthAction kind = AuthAction::None;
    QString    actionLabel;      // primary button text
    QString    actionUrl;        // for OpenUrl
    QString    settingsCommandId; // optional; e.g. "claude.editApiKey"
};
virtual ProviderAuthInfo authInfo() const { return {}; }
virtual void startAuth() {}
```

- **Why:** The UI must not hardcode provider knowledge. A descriptor keeps the mapping in the provider that owns it, and default implementations keep existing providers compiling.
- **Alternatives considered:** Mapping provider id → action in the panel (couples UI to every provider); a separate `IAgentAuthProvider` interface (extra plugin surface for little gain).

### Decision: Dispatch by action kind in the panel

The welcome widget renders the label and emits a single `authActionRequested` signal. The panel resolves the active provider's `authInfo()` and dispatches: `OAuth` → `startAuth()`; `OpenUrl` → `QDesktopServices::openUrl()`; `OpenSettings` → `settingsRequested()`.

- **Why:** Keeps the widget dumb and testable; the panel already owns the orchestrator and the settings signal.
- **Alternatives considered:** Passing a callback into the widget (harder to test, lifetime risk).

### Decision: Secondary link opens the provider key-entry surface

The link emits `settingsRequested`; the host opens the AI settings dialog, unless the provider supplies `settingsCommandId`, in which case the host executes that command (Claude → `claude.editApiKey`).

- **Why:** Claude's key entry is a command, not a settings field; without this the link would be a no-op for Claude — the exact bug being fixed.
- **Alternatives considered:** Adding a Claude key field to the settings panel (larger UI change, duplicates the existing command).

### Decision: Copilot OAuth moves behind `startAuth()`

`CopilotProvider::initialize()` no longer calls `tryOAuthLogin()`; it reports unavailable when no token is present. `startAuth()` calls `tryOAuthLogin()`.

- **Why:** Directly satisfies "selecting Copilot must not immediately trigger a GitHub request".
- **Alternatives considered:** A flag on `initialize()` (leaks UI intent into the provider lifecycle).

### Decision: OpenRouter PKCE with a localhost callback

The BYOK provider implements the flow: generate a PKCE verifier and S256 challenge, bind a `QTcpServer` to `127.0.0.1` on an ephemeral port, open `https://openrouter.ai/auth?callback_url=...&code_challenge=...&code_challenge_method=S256`, capture the `code` from the callback, exchange it at `https://openrouter.ai/api/v1/auth/keys`, and store the returned key as the OpenRouter preset's API key.

- **Why:** OpenRouter documents this flow and explicitly allows `localhost`/`127.0.0.1` callback URLs on any port.
- **Alternatives considered:** OpenRouter's headless paste mode (no local server, but worse UX); treating OpenRouter as API-key only (ignores an available official flow).

### Decision: Exclude Ultralight and dead code

`src/resources/chat/chat.html` / `chat.js` and `ErrorStateWidget::showSignInPrompt()` are not modified.

- **Why:** Ultralight is excluded by project convention; `ErrorStateWidget` is not instantiated anywhere.

## Risks / Trade-offs

- [Adding virtuals to `IAgentProvider` changes the vtable] → Host and plugins are built together in-tree; rebuild all plugin targets. No IID bump needed since the interface is not loaded across versions.
- [OpenRouter callback port conflicts or firewall blocks] → Bind to `127.0.0.1` on an ephemeral port, close the server on completion, cancellation, and timeout; the secondary "paste your API key" link is the fallback.
- [Remote/headless sessions cannot reach a localhost callback] → Accepted; the secondary link covers this case. A headless paste mode can be added later.
- [Claude settings command may not be registered] → If `settingsCommandId` is set but the command is missing, fall back to opening the AI settings dialog.
- [Auth action shown for providers that need no credentials] → Gate on `!isAvailable() && authInfo().kind != None`; providers that need nothing return `None`.
- [Transcript auth-error button still says "Sign In"] → Route it through the same provider auth action; the generic label is acceptable, the behavior is what matters.

## Migration Plan

No data migration. Existing stored credentials keep working. Rollback is reverting the commit; the only behavioral change on rollback is the return of implicit Copilot OAuth.