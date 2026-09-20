## 1. Preset model and persistence

- [x] 1.1 Define the preset registry (ids `openai-compat:openai`, `openai-compat:zai`, `openai-compat:openrouter`, `openai-compat:custom`; display names; default completions endpoints) and verify it compiles with a unit test asserting each default endpoint value
- [x] 1.2 Add the settings namespace helpers (`AI/OpenAICompatible/<preset>/{endpoint,apiKey,model,models}`, `activePreset`, `migrated`) with save/load functions, verified by a unit test covering empty-endpoint-falls-back-to-default
- [x] 1.3 Implement host-suffix classification (`openai.com` → OpenAI, `openrouter.ai` → OpenRouter, else Custom) and verify with unit tests for representative URLs

## 2. Provider plugin

- [x] 2.1 Evolve `plugins/byok/` to register four preset providers (`createProviders()` returning four instances) while keeping the legacy `custom` id alias resolving to `openai-compat:custom`, verified by a test that provider ids and display names match the registry from 1.1
- [x] 2.2 Implement per-instance `availableModels()`, `currentModel()`, and `setModel()` backed by the preset persistence, verified by a unit test that selection survives a reload
- [x] 2.3 Implement the shared models-URL derivation helper (strip query and trailing `/chat/completions`/`/completions`/`/messages`/`/responses`, append `/models`, preserve `/v1` prefix) and verify with unit tests including a Custom `/v1/chat/completions` case and a derivation-failure case
- [x] 2.4 Implement startup discovery: load and emit the persisted list first, then GET the derived models URL, parse `data[].id`, persist on success, emit `modelsChanged()`, and keep the persisted list on failure; verify with a unit test for success, failure, and unconfigured cases
- [x] 2.5 Wire `sendRequest()` to use the active preset's effective endpoint and API key, verified by a unit test asserting the request URL and `Authorization` header per preset
- [x] 2.6 Implement one-time BYOK migration (marker-guarded, non-destructive, host-routed) and verify with unit tests for Custom/OpenAI/OpenRouter mapping and idempotency

## 3. Settings UI

- [x] 3.1 Replace the free-form "Custom Endpoint (BYOK)" section in [`settingspanel.cpp`](src/agent/settingspanel.cpp:435) with a preset dropdown plus endpoint and API-key fields, verified by a build and manual check that changing the preset swaps the loaded values
- [x] 3.2 Show the active preset's default endpoint as the endpoint field placeholder when the saved value is empty, verified by a UI test/manual check that clearing the field restores the default hint
- [x] 3.3 Apply the same preset dropdown/endpoint/key behavior to [`modelconfigwidget.cpp`](src/agent/modelconfigwidget.cpp:37), verified by a build and manual check that both surfaces read/write the same keys
- [x] 3.4 Persist the selected preset and per-preset key on edit (retained across switches), verified by a manual switch test and a unit test on the persistence layer

## 4. AI panel integration

Note: the active panel is [`ChatPanelWidget`](src/agent/chat/chatpanelwidget.cpp:81); [`AgentChatPanel`](src/agent/agentchatpanel.cpp:63) is dead code and must not be targeted. The Ultralight/JS-UI variant is **out of scope** — no changes to `ChatJSBridge` or the embedded web UI.

- [x] 4.1 Add a provider dropdown to [`ChatPanelWidget`](src/agent/chat/chatpanelwidget.cpp:81), populated from `m_orchestrator->providers()` and calling `setActiveProvider(id)` on change; verify with a widget test that the combo contains the four preset display names and that selection changes the active provider
- [x] 4.2 Ensure [`ChatPanelWidget::refreshModelList()`](src/agent/chat/chatpanelwidget.cpp:1093) populates the [`ChatInputWidget`](src/agent/chat/chatinputwidget.cpp:214) "Model" dropdown from the active provider's discovered model list on `modelsChanged`/`providerRegistered`/`activeProviderChanged`, and that the `modelSelected` signal reaches `active->setModel()`; verify with a test that a synthetic model selection calls `setModel()` with the chosen id
- [x] 4.3 Verify the model picker applies the selected model to a subsequent request (assert the outgoing request's model id equals the picked id) rather than only updating UI state
- [x] 4.4 Persist and restore the selected model per preset across restart, verified by a test that reloads the provider and asserts `currentModel()` matches the saved preset model
- [x] 4.5 Update [`mainwindow_docks.cpp`](src/mainwindow_docks.cpp:530) settings→provider wiring to push preset/endpoint/key to the correct preset provider, verified by a test that editing settings reflects in the active provider's effective endpoint/key

## 5. Integration and regression

- [x] 5.1 Add a legacy-config fixture migration test (old `AI/customEndpoint`/`AI/customApiKey` in each of the three routings) and verify all pass
- [x] 5.2 Confirm existing providers (`Codex`, `Claude`, `Copilot`, `Ollama`) still register and work after the selector change, verified by building and running the full test suite (`cmake --build build && ctest --test-dir build`)
- [x] 5.3 Verify startup with an unreachable endpoint does not error or crash and keeps the persisted model list, verified by a manual run with an invalid endpoint