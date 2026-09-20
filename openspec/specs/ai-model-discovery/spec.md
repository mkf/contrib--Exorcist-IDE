# ai-model-discovery Specification

## Purpose

Keeps the AI panel's model list current by discovering the models an OpenAI-compatible provider exposes, remembering the last known list, and letting the user actually select the model used for requests.

## Requirements

### Requirement: Startup model refresh
On application startup, the system SHALL request the available model list for the active provider, and SHALL update the stored list when the request succeeds.

#### Scenario: Successful refresh
- **WHEN** the application starts with a configured provider whose model endpoint responds successfully
- **THEN** the stored model list SHALL be replaced with the models returned by that provider

#### Scenario: Failed refresh keeps previous list
- **WHEN** the application starts and the model request fails or returns no models
- **THEN** the system SHALL retain the previously persisted model list and SHALL NOT report a fatal error

#### Scenario: No configuration
- **WHEN** the application starts with no endpoint or API key configured for the active provider
- **THEN** the system SHALL skip model discovery without error

### Requirement: OpenAI-compatible model list parsing
For OpenAI-compatible providers, the system SHALL request the `/models` resource and read the model identifiers from the `data[].id` field of the response.

#### Scenario: Standard response
- **WHEN** the model endpoint returns an OpenAI-shaped payload containing a `data` array of objects each with an `id`
- **THEN** the system SHALL collect those `id` values as the available models

#### Scenario: Empty or non-standard response
- **WHEN** the response contains no usable `data[].id` values
- **THEN** the system SHALL treat the refresh as unsuccessful and keep the previous list

### Requirement: Custom preset model URL derivation
For the `Custom` preset, the system SHALL attempt to derive a model-list URL from the configured chat-completions URL by removing the trailing completions operation segment and appending `models`, and SHALL treat an unsuccessful derivation or request as non-fatal.

#### Scenario: Derive from chat completions URL
- **WHEN** the Custom endpoint is `https://host/v1/chat/completions`
- **THEN** the system SHALL attempt to request `https://host/v1/models`

#### Scenario: Derivation or request fails gracefully
- **WHEN** the derived model URL does not exist or the request fails
- **THEN** the system SHALL continue without models and SHALL NOT fail the provider or the application

### Requirement: Persisted model list
The system SHALL persist the most recently discovered model list so it is available before the next network request completes.

#### Scenario: Restore before refresh
- **WHEN** the application starts
- **THEN** the previously persisted model list SHALL be available to the UI even before the startup refresh returns

### Requirement: Model picker reflects and applies selection
The AI panel's model picker SHALL be populated from the available model list and SHALL apply the user's selected model as the model used for subsequent requests.

#### Scenario: Picker populated
- **WHEN** the AI panel becomes visible with a non-empty model list
- **THEN** the model picker SHALL list those models and preselect the provider's current model

#### Scenario: Selection applies to requests
- **WHEN** the user selects a model from the picker
- **THEN** subsequent requests from that provider SHALL use the selected model

#### Scenario: Selection persists
- **WHEN** the user selects a model and restarts the application
- **THEN** the previously selected model SHALL be restored as the current model

#### Scenario: List updates propagate
- **WHEN** the provider's model list changes after a refresh
- **THEN** the model picker SHALL be repopulated to reflect the new list