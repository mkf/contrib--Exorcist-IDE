## Purpose

Defines the user-visible naming of the provider-neutral AI assistant surface, and the boundary that reserves GitHub Copilot branding for the genuine GitHub Copilot provider only.

## ADDED Requirements

### Requirement: Provider-neutral assistant naming

The provider-neutral AI assistant surfaces SHALL NOT display the word "Copilot". The chat panel/session title and the assistant name shown on assistant messages SHALL read `Exorcist AI`. Generic references to the assistant SHALL read `AI Assistant`.

#### Scenario: Chat panel title

- **WHEN** the AI chat panel is shown with no session-specific title
- **THEN** the panel/session title SHALL read `Exorcist AI`
- **AND** it SHALL NOT contain the word `Copilot`

#### Scenario: Welcome heading

- **WHEN** the chat welcome screen is shown
- **THEN** the welcome heading SHALL read `Ask AI Assistant`
- **AND** it SHALL NOT contain the word `Copilot`

#### Scenario: Sign-in prompt

- **WHEN** the chat surface indicates that sign-in is required
- **THEN** the prompt SHALL refer to the AI assistant without naming `Copilot`

#### Scenario: Editor AI menu

- **WHEN** the editor context menu is opened
- **THEN** the AI submenu SHALL be labeled `AI Assistant`
- **AND** it SHALL NOT be labeled `Copilot`

#### Scenario: Auth status indicator

- **WHEN** the auth status indicator renders any of its states (signed out, signing in, signed in, expired, rate limited, offline)
- **THEN** its label SHALL read `Exorcist AI`
- **AND** it SHALL NOT contain the word `Copilot`

#### Scenario: Assistant name on messages

- **WHEN** an assistant message is rendered in the chat transcript
- **THEN** the displayed assistant name SHALL read `Exorcist AI`

### Requirement: GitHub Copilot provider retains product branding

The genuine GitHub Copilot provider SHALL continue to identify itself as GitHub Copilot, because it authenticates against GitHub and calls the GitHub Copilot API.

#### Scenario: Provider display name

- **WHEN** the GitHub Copilot provider is listed among available AI providers
- **THEN** its display name SHALL read `GitHub Copilot`

#### Scenario: Sign-in dialog

- **WHEN** the GitHub Copilot sign-in dialog is opened
- **THEN** its title SHALL read `Sign in to GitHub Copilot`

#### Scenario: Plugin metadata

- **WHEN** the GitHub Copilot plugin is presented in the plugin gallery or settings
- **THEN** its name and description SHALL identify it as GitHub Copilot

### Requirement: GitHub-compatible instruction files remain honored

The system SHALL continue to load GitHub-compatible custom instruction files from their conventional paths, since these are compatibility inputs rather than user-visible branding.

#### Scenario: Copilot instruction file loaded

- **WHEN** a workspace contains `.github/copilot-instructions.md`
- **THEN** the system SHALL load its contents as custom instructions for the AI assistant