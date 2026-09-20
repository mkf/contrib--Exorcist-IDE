# ai-provider-presets Specification

## Purpose

Lets users pick a ready-made OpenAI-compatible provider (OpenAI, Z.ai GLM Coding Plan, OpenRouter, or a free-form Custom endpoint) and configure each one's endpoint and API key independently, so switching providers never loses credentials.

## Requirements

### Requirement: Provider preset selection
The system SHALL present a single-selection dropdown listing the presets `OpenAI`, `Z.ai`, `OpenRouter`, and `Custom`. Selecting a preset SHALL make that preset the active OpenAI-compatible provider.

#### Scenario: Default preset
- **WHEN** no preset has ever been saved
- **THEN** the system SHALL select `Custom` as the active preset

#### Scenario: Switch presets
- **WHEN** the user selects a different preset from the dropdown
- **THEN** the system SHALL treat the newly selected preset as active and load that preset's saved endpoint and API key

### Requirement: Per-preset default endpoints
Each preset SHALL define a built-in default chat-completions endpoint. The effective endpoint for a preset SHALL be the user-saved endpoint when non-empty, otherwise the preset default.

#### Scenario: Preset defaults
- **WHEN** the `OpenAI` preset is active with no user-saved endpoint
- **THEN** the effective endpoint SHALL be `https://api.openai.com/v1/chat/completions`

#### Scenario: Z.ai default
- **WHEN** the `Z.ai` preset is active with no user-saved endpoint
- **THEN** the effective endpoint SHALL be `https://api.z.ai/api/coding/paas/v4/chat/completions`

#### Scenario: OpenRouter default
- **WHEN** the `OpenRouter` preset is active with no user-saved endpoint
- **THEN** the effective endpoint SHALL be `https://openrouter.ai/api/v1/chat/completions`

#### Scenario: Empty saved endpoint restores default
- **WHEN** the user clears a preset's endpoint field such that the saved value is empty
- **THEN** the effective endpoint SHALL fall back to that preset's default

### Requirement: Default endpoint shown as hint
The endpoint input SHALL display the active preset's default endpoint as the field's placeholder/hint whenever the saved endpoint is empty.

#### Scenario: Hint reflects preset
- **WHEN** the active preset changes to `OpenRouter` and no endpoint is saved for it
- **THEN** the endpoint input SHALL show `https://openrouter.ai/api/v1/chat/completions` as its hint

### Requirement: Endpoint override
The endpoint for each preset SHALL be user-editable, and a non-empty saved value SHALL override the preset default for that preset only.

#### Scenario: Override one preset
- **WHEN** the user saves a custom endpoint for `OpenAI`
- **THEN** the `OpenAI` preset SHALL use the saved endpoint while other presets' effective endpoints remain unchanged

### Requirement: Per-preset API key persistence
The system SHALL persist a separate API key for each preset, keyed by preset, and SHALL retain each preset's key when the user switches to another preset.

#### Scenario: Key retained across switches
- **WHEN** the user sets an API key for `OpenAI`, switches to `Z.ai`, and later returns to `OpenAI`
- **THEN** the `OpenAI` API key field SHALL still contain the previously saved key

#### Scenario: Custom is a single slot
- **WHEN** the user configures the `Custom` preset
- **THEN** the system SHALL store exactly one endpoint and one API key for `Custom`, regardless of how many times it is edited

### Requirement: Migration of legacy BYOK configuration
On first run after upgrade, the system SHALL migrate the legacy BYOK settings (`AI/customEndpoint` and `AI/customApiKey`) into the preset model, and SHALL NOT lose the legacy value.

#### Scenario: Legacy endpoint maps to Custom
- **WHEN** a legacy BYOK endpoint exists whose host is neither OpenAI nor OpenRouter
- **THEN** the system SHALL select the `Custom` preset, store the legacy endpoint as the Custom endpoint, and store the legacy API key as the Custom API key

#### Scenario: Legacy OpenAI endpoint maps to OpenAI preset
- **WHEN** a legacy BYOK endpoint's host matches the OpenAI domain
- **THEN** the system SHALL select the `OpenAI` preset, preserve the legacy completions URL as the OpenAI endpoint, and store the legacy API key under the OpenAI preset

#### Scenario: Legacy OpenRouter endpoint maps to OpenRouter preset
- **WHEN** a legacy BYOK endpoint's host matches the OpenRouter domain
- **THEN** the system SHALL select the `OpenRouter` preset, preserve the legacy completions URL as the OpenRouter endpoint, and store the legacy API key under the OpenRouter preset

#### Scenario: Migration is idempotent
- **WHEN** the system runs again after a successful migration
- **THEN** it SHALL NOT overwrite preset values the user has since saved