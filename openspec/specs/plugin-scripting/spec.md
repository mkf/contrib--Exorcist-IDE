# plugin-scripting Specification

## Purpose

Defines the scripting-plugin surface after the JavaScript runtime is removed: scripting plugins are written in Lua only, with no JavaScript runtime, JavaScriptCore dependency, or JavaScript plugin artifacts.

## Requirements

### Requirement: Scripting plugins are Lua-only

The system SHALL load scripting plugins written in Lua. The system SHALL NOT include a JavaScript plugin runtime or load JavaScript plugins.

#### Scenario: Lua plugins load

- **WHEN** the application starts and a Lua plugin directory is present
- **THEN** the Lua scripting engine loads and initializes those plugins

#### Scenario: No JavaScript plugin runtime

- **WHEN** the application starts
- **THEN** no JavaScript plugin runtime is initialized
- **AND** no JavaScript plugin directory is scanned

### Requirement: No JavaScriptCore dependency

The build SHALL NOT link JavaScriptCore and SHALL NOT require a JavaScriptCore provider for plugin scripting.

#### Scenario: Configuration adds no JavaScriptCore target

- **WHEN** CMake configures the project
- **THEN** no `javascriptcore` interface target is defined
- **AND** no `EXORCIST_HAS_JAVASCRIPTCORE` variable is set

#### Scenario: Dev shell needs no WebKitGTK

- **WHEN** the development shell is evaluated
- **THEN** it does not provide a system JavaScriptCore (WebKitGTK) package

### Requirement: JavaScript plugin artifacts are not shipped

The system SHALL NOT ship the JavaScript plugin runtime plugin or any bundled JavaScript plugin examples.

#### Scenario: No JavaScript SDK plugin is built

- **WHEN** the plugins are built
- **THEN** no `javascript-sdk` plugin target is produced

#### Scenario: No JavaScript plugin examples are bundled

- **WHEN** the application is packaged
- **THEN** it contains no JavaScript plugin example directories