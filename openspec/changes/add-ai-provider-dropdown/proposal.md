## Why

Configuring an AI provider today means hand-editing BYOK settings (`AI/customEndpoint` + `AI/customApiKey`) and typing a raw completion URL, while the AI panel's model picker is populated from a single hard-coded model and appears inert. Users cannot quickly switch between common OpenAI-compatible providers (OpenAI, Z.ai GLM Coding Plan, OpenRouter) with per-provider keys, and no model list is discovered or persisted.

## What Changes

- Add a **provider preset dropdown** offering **OpenAI**, **Z.ai (GLM Coding Plan — international)**, **OpenRouter**, and **Custom**, replacing the free-form "Custom Endpoint (BYOK)" entry.
- Give each preset a built-in default chat-completions endpoint; the endpoint is editable, and an empty saved value falls back to the default (the default is shown as the field hint/placeholder).
- Persist **a distinct API key per preset** (OpenAI, Z.ai, OpenRouter, Custom), retained when switching between presets.
- For the **Custom** preset, attempt to derive the `/models` URL from the configured `/chat/completions` URL; treat failure as non-fatal.
- **Refresh the model list on each startup** for the active provider, persist the last known list, and populate the AI panel's model picker so the selected model is actually applied.
- **Migrate existing BYOK configuration**: move `AI/customEndpoint` / `AI/customApiKey` into the "Custom" preset; if the old completions URL host matches OpenAI or OpenRouter, select that preset instead, keep the chosen completions URL, and store the key under that preset's key location.

## Capabilities

### New Capabilities
- `ai-provider-presets`: Selecting an OpenAI-compatible provider preset (OpenAI / Z.ai / OpenRouter / Custom), per-preset default and overridable endpoint, and per-preset API-key persistence including BYOK migration.
- `ai-model-discovery`: Fetching and persisting the available model list on startup and driving the model picker in the AI panel, including best-effort `/models` derivation for the Custom preset.

### Modified Capabilities
<!-- None: openspec/specs/ is empty; both capabilities are new. -->

## Impact

- **Plugins**: `plugins/byok/` (provider id currently `custom`) evolves into the OpenAI-compatible provider serving the four presets; adds model discovery + `modelsChanged` emission. Possible overlap with `plugins/codex/` ("OpenAI Codex"), which also targets OpenAI and performs `/models` discovery — see design Open Questions.
- **Host UI**: `src/agent/settingspanel.{h,cpp}` (preset dropdown, per-preset key field, endpoint field with default hint), `src/agent/modelconfigwidget.{h,cpp}`, and the **active** Qt chat panel `src/agent/chat/chatpanelwidget.{h,cpp}` + `src/agent/chat/chatinputwidget.{h,cpp}` (provider dropdown and the "Model" picker populated from each provider's models endpoint). `src/mainwindow_docks.cpp` handles settings→provider wiring. The Ultralight/JS-UI variant is intentionally **not** changed: no functionality is added to `src/agent/chat/ultralight/chatjsbridge.{h,cpp}` or the embedded web UI. The legacy `src/agent/agentchatpanel.cpp` (provider tabs) is **dead code** per project docs and is not a target.
- **Settings keys**: introduces per-preset keys under `AI/` (e.g. `AI/OpenAICompatible/<preset>/{endpoint,api_key}`), plus a persisted last-model list; existing `AI/customEndpoint` and `AI/customApiKey` become migration inputs.
- **Provider interface**: relies on the existing `IAgentProvider::availableModels()` / `currentModel()` / `setModel()` / `modelsChanged()` contract in [`src/aiinterface.h`](src/aiinterface.h:222); no interface change is expected.