## Context

See [`proposal.md`](proposal.md) for motivation. Relevant current state:

- AI providers are plugins implementing `IAgentProvider` ([`src/aiinterface.h`](src/aiinterface.h:222)) and are held by `AgentProviderRegistry`. The **active** AI dock is [`ChatPanelWidget`](src/agent/chat/chatpanelwidget.cpp:81) (created in [`dockbootstrap.cpp`](src/bootstrap/dockbootstrap.cpp:91)); [`AgentChatPanel`](src/agent/agentchatpanel.cpp:63) — which does implement provider tabs via [`refreshProviderList()`](src/agent/agentchatpanel.cpp:880) — is **dead code** (see `CLAUDE.md`: "Active chat panel: `ChatPanelWidget` (NOT `AgentChatPanel` — dead code)").
- The active panel has **no provider selector**. It only reacts to provider signals ([`onActiveProviderChanged()`](src/agent/chat/chatpanelwidget.cpp:1086)) and refreshes models ([`refreshModelList()`](src/agent/chat/chatpanelwidget.cpp:1093)). Provider switching today is done through the "Select AI Model" menu action (`Ctrl+Shift+M`) → [`ModelPickerDialog`](src/agent/modelpickerdialog.cpp:10), which owns a provider combo.
- The model picker is the dropdown in [`ChatInputWidget`](src/agent/chat/chatinputwidget.cpp:214) (`m_modelCombo`); `currentIndexChanged` emits `modelSelected`, handled in [`ChatPanelWidget`](src/agent/chat/chatpanelwidget.cpp:210) as `active->setModel(id)`. It is already wired end-to-end — it only *looks* inert because the generic provider's `availableModels()` returns a single entry.
- The Ultralight/JS-UI path ([`ChatJSBridge`](src/agent/chat/ultralight/chatjsbridge.h:106), embedded web UI) is **out of scope**: no functionality will be added to that variant of the pane. The provider dropdown and the "Model" picker are implemented in the Qt panel only. (The bridge currently exposes model APIs but no provider-list/selection calls.)
- A provider-key manager UI already exists: [`APIKeyManagerWidget`](src/agent/apikeymanagerwidget.cpp:19) (`setProviders` + per-provider has-key state).
- The existing generic provider is [`plugins/byok/`](plugins/byok/byokprovider.cpp:1) (id `custom`). It reads `AI/customEndpoint` and `AI/customApiKey` and **does not** implement model discovery: [`availableModels()`](plugins/byok/byokprovider.cpp:40) returns only the single configured model.
- A working discovery precedent already exists in [`CodexProvider::fetchModels()`](plugins/codex/codexprovider.cpp:135), which derives `{base}/models` by cutting at `/chat/`, parses `data[].id`, and emits `modelsChanged()`.
- Settings UI is built in [`src/agent/settingspanel.cpp`](src/agent/settingspanel.cpp:435) (BYOK section) and a second BYOK surface exists in [`src/agent/modelconfigwidget.cpp`](src/agent/modelconfigwidget.cpp:37). Wiring to the provider is in [`src/mainwindow_docks.cpp`](src/mainwindow_docks.cpp:530).
- `openspec/specs/` is empty; both capabilities are new.

## Goals / Non-Goals

**Goals:**
- Present OpenAI, Z.ai, OpenRouter, Custom as one coherent OpenAI-compatible provider family with a preset dropdown.
- Per-preset endpoint (defaulted, overridable) and per-preset API key, retained across switching.
- Best-effort model discovery wired into the AI panel model picker, refreshed at startup and persisted.
- Non-destructive migration of legacy BYOK settings.

**Non-Goals:**
- Removing or rewriting the `Codex`, `Claude`, `Copilot`, or `Ollama` providers.
- Changing the `IAgentProvider` interface or the registry routing model.
- Introducing a new secret-storage backend.
- Implementing provider-specific request/response differences beyond the OpenAI-compatible wire format.
- Adding any functionality to the Ultralight/JS-UI variant of the pane or extending `ChatJSBridge`; the provider dropdown and Model picker target the Qt path only.

## Decisions

### D1: Register four providers from one preset-capable plugin
Evolve `plugins/byok/` into an OpenAI-compatible plugin that registers four `IAgentProvider` instances with stable ids `openai-compat:openai`, `openai-compat:zai`, `openai-compat:openrouter`, `openai-compat:custom`. Each instance owns its preset's endpoint and API key, so the AI panel provider selector can list them directly and "each provider's key" maps 1:1 to a registered provider.

- **Alternatives considered**: (a) one provider with an internal `setPreset()` — rejected because the AI panel dropdown and per-provider key semantics then need a second selector; (b) four separate plugins — rejected as boilerplate duplication.
- The legacy id `custom` is preserved as an alias mapping to `openai-compat:custom` so existing wiring such as [`mainwindow_docks.cpp`](src/mainwindow_docks.cpp:535) keeps working during transition.

### D2: Add a provider dropdown to the active Qt panel (not "replace tabs")
The provider tabs in `AgentChatPanel` are dead code, so nothing is replaced. Add a `QComboBox` provider selector to [`ChatPanelWidget`](src/agent/chat/chatpanelwidget.cpp:81), populated from `m_orchestrator->providers()` (display name as label, id as data), calling `setActiveProvider(id)` and refreshing models. The Ultralight/JS-UI variant is deliberately left unchanged (see Non-Goals). `ModelPickerDialog` remains as an alternative entry point.

- **Alternatives considered**: (a) rely solely on the existing `ModelPickerDialog` — rejected because the request is for an inline dropdown in the panel; (b) also render the selector in the Ultralight JS UI — rejected because no functionality is to be added to that variant.

### D3: Endpoint and key stored per preset under one settings namespace
Use `AI/OpenAICompatible/<preset>/endpoint`, `AI/OpenAICompatible/<preset>/apiKey`, `AI/OpenAICompatible/<preset>/model`, `AI/OpenAICompatible/<preset>/models` (JSON array), plus `AI/OpenAICompatible/activePreset` and a `AI/OpenAICompatible/migrated` marker. Effective endpoint = saved value if non-empty else the preset default; the settings field shows the default as placeholder when empty, satisfying the "restore default if saved empty / show default as hint" behavior.

- Defaults: OpenAI `https://api.openai.com/v1/chat/completions`; Z.ai `https://api.z.ai/api/coding/paas/v4/chat/completions`; OpenRouter `https://openrouter.ai/api/v1/chat/completions`; Custom empty until the user enters one.
- **Alternatives considered**: separate `QSettings` groups per provider id — rejected to keep the namespace discoverable and migration simple.

### D4: Shared model-URL derivation, discovery best-effort
Implement one helper that derives a models URL from a completions URL by dropping the query and the trailing `/chat/completions` (also accept `/completions`, `/messages`, `/responses`) and appending `/models`, preserving any `/v1`-style prefix. Used for all presets; for Custom a failed derivation or request is non-fatal (per spec). Discovery mirrors [`CodexProvider::fetchModels()`](plugins/codex/codexprovider.cpp:135), parses `data[].id`, persists the list, and emits `modelsChanged()`.

### D5: Startup refresh ordering — persisted list first, network second
In `initialize()`, load the persisted list and emit `modelsChanged()` immediately so the picker is populated instantly, then issue the network refresh and re-emit on success. Failed refreshes leave the persisted list intact.

### D6: Migration runs once, keyed by a marker, and never overwrites user edits
On plugin init, if `AI/OpenAICompatible/migrated` is unset and legacy `AI/customEndpoint`/`AI/customApiKey` exist, route them by host (`openai.com` → OpenAI preset; `openrouter.ai` → OpenRouter preset; otherwise Custom), preserving the legacy completions URL for the matched preset, then set the marker. Existing new-namespace values are never overwritten.

### D7: Model selection persists per preset
`setModel()` writes `AI/OpenAICompatible/<preset>/model`; `availableModels()` returns the discovered list (or `{currentModel}` when empty). This makes the AI panel picker's selection survive restart and actually drive requests.

## Risks / Trade-offs

- **Two "OpenAI" entries** (existing `Codex` plugin vs new `openai-compat:openai`) → Mitigate by distinct display names ("OpenAI (API)") and documenting the Codex plugin as the legacy OpenAI path; full consolidation is deferred (see Open Questions).
- **API keys in plaintext `QSettings`** (consistent with today's BYOK) → Mitigate by reusing the same trust model as the existing implementation; migrating to `SecureKeyStorage` is out of scope but noted.
- **Lessons from Azure-style endpoints**: a naive `/chat/completions` → `/models` derivation can 404 → Already handled by the spec's non-fatal requirement; failures keep the persisted list.
- **Settings duplication** between `settingspanel` and `modelconfigwidget` → Mitigate by having both surfaces read/write the same preset keys and share the preset default logic.
- **Migration misclassification** if a custom URL hosts both OpenAI and OpenRouter paths → Mitigate by exact host-suffix matching (`api.openai.com`, `openrouter.ai`) and only when the marker is absent.

## Migration Plan

1. On first run after upgrade, migrate legacy BYOK keys into the preset namespace per D6 and set the marker.
2. Keep reading legacy keys as a fallback for one release so downgrades do not lose configuration.
3. Rollback: because legacy keys are left untouched, reverting the plugin restores prior behavior; no destructive writes occur.

## Open Questions

- Should the new `openai-compat:openai` preset supersede the existing `Codex` provider for OpenAI, or coexist? Coexistence is assumed here; consolidating later does not change these specs or the task breakdown.
- Should Z.ai's Anthropic-protocol endpoint (`https://api.z.ai/api/anthropic`) be offered as an additional preset? The proposal explicitly scopes the international **GLM Coding Plan OpenAI Chat Completions** endpoint, so this is deferred.