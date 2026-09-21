# chat-rendering Specification

## Purpose

Defines how the AI chat panel is rendered after the Ultralight HTML renderer is removed: the panel is built exclusively from Qt widgets, with no embedded web/HTML rendering surface.

## Requirements

### Requirement: Chat panel renders via Qt widgets only

The chat panel SHALL render its transcript, welcome state, and input area using the Qt widget path. The system SHALL NOT include an HTML/JS-based chat renderer.

#### Scenario: Panel construction uses Qt widgets

- **WHEN** the chat panel is constructed
- **THEN** it builds the Qt widget transcript, welcome, and input widgets
- **AND** it creates no embedded web view or JS bridge

#### Scenario: Streaming updates target Qt widgets

- **WHEN** a response, thinking delta, or tool-call update arrives
- **THEN** the update is applied to the Qt widget transcript
- **AND** no JavaScript is evaluated to render the update

### Requirement: No Ultralight build option or dependency

The build SHALL NOT expose an `EXORCIST_USE_ULTRALIGHT` option, SHALL NOT define `EXORCIST_HAS_ULTRALIGHT`, and SHALL NOT fetch, link, or deploy the Ultralight SDK.

#### Scenario: Configuration adds no Ultralight target

- **WHEN** CMake configures the project
- **THEN** no `ultralight-sdk` target and no `cmake/ultralight` subdirectory are added

#### Scenario: Build links no Ultralight libraries

- **WHEN** the executable is built
- **THEN** it does not link Ultralight libraries
- **AND** no `ultralight-resources` directory is deployed next to the executable

### Requirement: Ultralight chat assets are not shipped

The system SHALL NOT ship the Ultralight chat HTML/CSS/JS assets.

#### Scenario: Resource bundle excludes chat web assets

- **WHEN** the resource bundle is built
- **THEN** it contains no `:/chat/chat.html`, `:/chat/chat.js`, `:/chat/chat.css`, or `:/chat/markdown.js` entries