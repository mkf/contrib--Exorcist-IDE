## 1. Chat panel identity strings (Qt widgets)

- [ ] 1.1 In `src/agent/chat/chatpanelwidget.cpp`, replace the session-title fallback `tr("Copilot")` (both the initial `m_sessionTitleLabel` text and the empty-title fallback) with `tr("Exorcist AI")`; verify by grepping the file for `Copilot` and confirming no user-visible literal remains.
- [ ] 1.2 In `src/agent/chat/chatturnwidget.cpp`, replace the assistant name label `tr("Copilot")` with `tr("Exorcist AI")`; verify the rendered assistant name reads `Exorcist AI`.
- [ ] 1.3 In `src/agent/chat/chatwelcomewidget.cpp`, change the welcome heading `tr("Ask Copilot")` to `tr("Ask AI Assistant")` and the sign-in prompt `tr("Sign in with your GitHub account to use Copilot.")` to a neutral `tr("Sign in to use the AI assistant.")`; verify neither string contains `Copilot`.

## 2. Other user-visible surfaces

- [ ] 2.1 In `src/editor/editorview.cpp`, change the AI context-menu label `tr("Copilot")` to `tr("AI Assistant")`; verify the editor context menu shows `AI Assistant`.
- [ ] 2.2 In `src/agent/authstatusindicator.cpp`, replace the `Copilot` suffix in every state label (signed out, signing in, signed in, expired, rate limited, offline) with `Exorcist AI`; verify no state label contains `Copilot`.

## 3. Preserve genuine GitHub Copilot provider branding

- [ ] 3.1 Confirm `plugins/copilot/plugin.json` name/description, `CopilotProvider::displayName()`, the `CopilotAuthDialog` title `Sign in to GitHub Copilot`, its settings page title, and the `plugin_registry.json` entry are unchanged; verify by diffing those files against the pre-change revision.

## 4. Tests and translations

- [ ] 4.1 Add or extend a Qt Test asserting the provider-neutral surfaces expose `Exorcist AI` / `AI Assistant` and do not contain `Copilot`; verify the new test passes under `ctest`.
- [ ] 4.2 Regenerate/update `translations/` for the new source strings; verify the build emits no missing-translation warnings for the changed files.

## 5. Verification

- [ ] 5.1 Build with `nix develop --command cmake --build build-tests -j4` and run `nix develop --command bash -c 'cd build-tests && QT_QPA_PLATFORM=offscreen ctest -j4 --output-on-failure'`; verify all tests pass.
- [ ] 5.2 Audit the enumerated files for remaining user-visible `Copilot` strings and confirm `src/resources/chat/chat.html` and `chat.js` (Ultralight) were not modified; verify with a grep and `git status`.