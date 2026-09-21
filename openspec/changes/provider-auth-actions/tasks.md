## 1. Provider auth contract

- [x] 1.1 Add `AuthAction` enum and `ProviderAuthInfo` struct to `src/aiinterface.h`, plus `virtual ProviderAuthInfo authInfo() const` and `virtual void startAuth()` with default implementations on `IAgentProvider`; verify the project still builds with existing providers unchanged.
- [x] 1.2 In `plugins/copilot/copilotprovider.*`, remove the `tryOAuthLogin()` call from `initialize()` (report unavailable instead), and implement `authInfo()` returning `OAuth` with label `Sign in with GitHub` and `startAuth()` calling `tryOAuthLogin()`; verify selecting Copilot no longer opens the OAuth dialog and the action does.
- [x] 1.3 In `plugins/claude/claudeprovider.*`, implement `authInfo()` returning `OpenUrl` with label `Get an Anthropic API Key`, URL `https://console.anthropic.com/settings/keys`, and `settingsCommandId` `claude.editApiKey`; verify the descriptor is returned when the provider is unavailable.
- [x] 1.4 In `plugins/byok/byokprovider.*`, implement `authInfo()` per active preset: OpenRouter → `OAuth` / `Sign in with OpenRouter`; OpenAI → `OpenUrl` / `Get an OpenAI API Key` / `https://platform.openai.com/api-keys`; Z.ai → `OpenUrl` / `Get a Z.ai API Key` / `https://z.ai/manage-apikey/apikey-list`; Custom → `OpenSettings` / `Open Settings to Paste API Key`; verify each preset returns the expected descriptor.

## 2. OpenRouter OAuth PKCE flow

- [x] 2.1 Implement PKCE generation (verifier + S256 challenge) and a `QTcpServer` bound to `127.0.0.1` on an ephemeral port that serves the callback; verify the authorization URL is built with `callback_url`, `code_challenge`, and `code_challenge_method=S256`.
- [x] 2.2 Implement the code exchange against `https://openrouter.ai/api/v1/auth/keys` and store the returned key as the OpenRouter preset's API key, then mark the provider available; verify a successful exchange makes the provider available.
- [x] 2.3 Handle cancellation, timeout, and exchange failure by closing the server and leaving the provider unavailable; verify no listening socket remains after each path.
- [x] 2.4 Add a unit test for PKCE challenge derivation and callback URL construction; verify it passes under `ctest`.

## 3. Chat UI and host wiring

- [x] 3.1 In `src/agent/chat/chatwelcomewidget.*`, replace the hardcoded `Sign In with GitHub` button with a provider-supplied primary action label plus a secondary link `or open settings to paste your API key`, emitting distinct signals for each; verify the widget renders the supplied label and link.
- [x] 3.2 In `src/agent/chat/chatpanelwidget.cpp`, dispatch the primary action by `authInfo().kind` (`OAuth` → `startAuth()`, `OpenUrl` → `QDesktopServices::openUrl()`, `OpenSettings` → `settingsRequested()`), remove the implicit `initialize()` call from the sign-in handler, and show the action only when `!isAvailable() && kind != None`; verify selecting a provider does not start auth.
- [x] 3.3 Route the transcript auth-error button (`chatturnwidget.*` / `chattranscriptview.*`) through the same provider auth action instead of `initialize()`; verify an auth error offers the provider's action.
- [x] 3.4 In `src/mainwindow.cpp`, handle the secondary link by executing the provider's `settingsCommandId` when set (falling back to the AI settings dialog if the command is missing), otherwise opening the AI settings dialog; verify the link opens the correct surface for Claude and for BYOK.

## 4. Tests

- [x] 4.1 Add a Qt Test asserting `IAgentProvider` auth defaults (`None`, empty label) and that the welcome widget renders a supplied action label and the secondary link; verify it passes under `ctest`.
- [x] 4.2 Add a test asserting that activating a provider does not call `startAuth()` and that the auth action is hidden when the provider is available; verify it passes under `ctest`.

## 5. Verification

- [x] 5.1 Build with `nix develop --command cmake --build build-tests -j4` and run `nix develop --command bash -c 'cd build-tests && QT_QPA_PLATFORM=offscreen ctest -j4 --output-on-failure'`; verify all tests pass.
- [x] 5.2 Confirm `src/resources/chat/chat.html` and `chat.js` (Ultralight) were not modified and that no provider selection path calls an interactive auth flow; verify with `git status` and a grep for `tryOAuthLogin` call sites.