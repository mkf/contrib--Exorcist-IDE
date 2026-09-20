## Purpose

Defines how the active AI provider advertises and performs its authentication action, and how the chat surface presents that action without ever starting authentication implicitly.

## ADDED Requirements

### Requirement: No implicit authentication on provider selection

Selecting a provider SHALL make it active without starting an interactive authentication flow. Authentication SHALL start only when the user explicitly triggers the provider's auth action.

#### Scenario: Selecting a provider from the dropdown

- **WHEN** the user selects a provider from the provider dropdown
- **THEN** the system SHALL make that provider active
- **AND** it SHALL NOT open a browser, OAuth dialog, or any other interactive authentication flow

#### Scenario: Restoring a saved provider at startup

- **WHEN** the application starts and restores a saved active provider that has no stored credentials
- **THEN** the system SHALL NOT start an interactive authentication flow
- **AND** the provider SHALL report itself unavailable

### Requirement: Provider-specific authentication action

When the active provider is unavailable and requires credentials, the chat welcome surface SHALL present a primary action whose label and behavior are supplied by that provider.

#### Scenario: GitHub Copilot action

- **WHEN** the active provider is GitHub Copilot and it is unavailable
- **THEN** the primary action SHALL be labeled `Sign in with GitHub`
- **AND** activating it SHALL start the GitHub OAuth device flow

#### Scenario: OpenRouter action

- **WHEN** the active provider is the OpenRouter preset and it is unavailable
- **THEN** the primary action SHALL be labeled `Sign in with OpenRouter`
- **AND** activating it SHALL start the OpenRouter OAuth PKCE flow

#### Scenario: Claude action

- **WHEN** the active provider is Claude and it is unavailable
- **THEN** the primary action SHALL be labeled `Get an Anthropic API Key`
- **AND** activating it SHALL open `https://console.anthropic.com/settings/keys` in the browser

#### Scenario: OpenAI action

- **WHEN** the active provider is the OpenAI preset and it is unavailable
- **THEN** the primary action SHALL be labeled `Get an OpenAI API Key`
- **AND** activating it SHALL open `https://platform.openai.com/api-keys` in the browser

#### Scenario: Z.ai action

- **WHEN** the active provider is the Z.ai preset and it is unavailable
- **THEN** the primary action SHALL be labeled `Get a Z.ai API Key`
- **AND** activating it SHALL open `https://z.ai/manage-apikey/apikey-list` in the browser

#### Scenario: Custom action

- **WHEN** the active provider is the Custom preset and it is unavailable
- **THEN** the primary action SHALL be labeled `Open Settings to Paste API Key`
- **AND** activating it SHALL open the settings surface

### Requirement: Secondary settings link

The auth action SHALL be accompanied by a secondary link that opens the provider's key-entry surface.

#### Scenario: Link is shown with the action

- **WHEN** the provider auth action is shown
- **THEN** a secondary link labeled `or open settings to paste your API key` SHALL be shown below it

#### Scenario: Link opens the provider key entry

- **WHEN** the user activates the secondary link
- **THEN** the system SHALL open the provider's key-entry surface
- **AND** when the provider defines its own settings command, the system SHALL invoke that command instead of the generic settings dialog

### Requirement: Auth action visibility

The auth action SHALL be shown only when the active provider is unavailable and requires credentials.

#### Scenario: Provider is available

- **WHEN** the active provider is available
- **THEN** the auth action SHALL NOT be shown

#### Scenario: Provider needs no credentials

- **WHEN** the active provider requires no credentials
- **THEN** the auth action SHALL NOT be shown

### Requirement: OpenRouter OAuth PKCE flow

The OpenRouter preset SHALL support a browser-based OAuth PKCE flow that yields a user-controlled API key.

#### Scenario: Starting the flow

- **WHEN** the user activates the OpenRouter auth action
- **THEN** the system SHALL open the browser to OpenRouter's authorization URL with a PKCE code challenge and a localhost callback URL

#### Scenario: Completing the flow

- **WHEN** OpenRouter redirects to the localhost callback with an authorization code
- **THEN** the system SHALL exchange the code for an API key
- **AND** it SHALL store the returned key as the OpenRouter preset's API key
- **AND** the provider SHALL become available

#### Scenario: Flow fails or is cancelled

- **WHEN** the flow is cancelled, times out, or the exchange fails
- **THEN** the provider SHALL remain unavailable
- **AND** the system SHALL NOT crash or leave a listening socket open

### Requirement: Copilot OAuth is explicit

The GitHub Copilot provider SHALL NOT start OAuth during initialization.

#### Scenario: Initialized without a token

- **WHEN** the Copilot provider is initialized and no GitHub token is stored
- **THEN** it SHALL report itself unavailable
- **AND** it SHALL NOT open the OAuth dialog

#### Scenario: Auth action starts OAuth

- **WHEN** the user activates the Copilot auth action
- **THEN** the GitHub OAuth device flow SHALL start