## Context

See [`proposal.md`](proposal.md) for motivation. The relevant current state:

- The provider-neutral chat surface is rendered by the Qt widget implementation (`src/agent/chat/chatpanelwidget.cpp`, `chatwelcomewidget.cpp`, `chatturnwidget.cpp`).
- Additional user-visible "Copilot" strings live in `src/agent/authstatusindicator.cpp` (status label) and `src/editor/editorview.cpp` (context-menu submenu).
- The genuine GitHub Copilot provider lives in `plugins/copilot/` and is the only surface that actually talks to GitHub Copilot (`api.githubcopilot.com`, GitHub OAuth). Its branding is correct and must be preserved.
- Many other "Copilot" occurrences are non-user-visible: class/file names, `QSettings` keys, logging categories, API endpoints/headers, comments, and GitHub-compatible instruction file paths.
- The Ultralight web chat (`src/resources/chat/chat.html`, `src/resources/chat/chat.js`) is **out of scope by project convention** — Ultralight UI is never touched by proposals (see [`AGENTS.md`](../../../AGENTS.md)).

## Goals / Non-Goals

**Goals:**

- Remove every user-visible "Copilot" string from the provider-neutral AI assistant surface.
- Apply a consistent two-term vocabulary: `Exorcist AI` for identity, `AI Assistant` for generic references.
- Preserve genuine GitHub Copilot provider branding and all compatibility inputs.

**Non-Goals:**

- Renaming internal identifiers, files, classes, settings keys, log categories, endpoints, or headers.
- Changing the GitHub Copilot provider's display name, plugin metadata, or sign-in dialog.
- Touching repo-facing docs (`docs/`, `README.md`, `.github/`).
- Touching the Ultralight web chat (`src/resources/chat/chat.html`, `chat.js`) — excluded by project convention.
- Touching dead code (`src/agent/agentchatpanel.*`), which is not built or loaded.
- Migrating already-persisted session titles that contain "Copilot".

## Decisions

### Decision: Two-term vocabulary

Use `Exorcist AI` where the string names the assistant as an entity (chat panel/session title, assistant name on messages, auth status indicator label) and `AI Assistant` where the string is a generic reference (welcome heading `Ask AI Assistant`, editor submenu `AI Assistant`, sign-in prompt).

- **Why:** A single term reads awkwardly in both roles; the split matches the existing status-bar vocabulary ("AI Ready", "No AI") and the user's stated preference.
- **Alternatives considered:** `AI Assistant` everywhere (loses product identity); `Exorcist` alone (ambiguous with the IDE itself).

### Decision: Scope by visibility, not by token

Only strings a user can read are changed. Internal identifiers, `QSettings` keys (`ai/copilot/*`), log categories (`exorcist.copilot`), API endpoints/headers, and instruction-file paths (`.github/copilot-instructions.md`, `.copilot-*-instructions.md`) are left untouched.

- **Why:** Renaming them would break persisted settings, log filtering, API compatibility, and GitHub-compatible instruction loading for no user-visible benefit.
- **Alternatives considered:** A blanket rename of every "Copilot" token (high risk, no user-visible gain).

### Decision: Exclude the Ultralight web chat

`src/resources/chat/chat.html` and `chat.js` are not edited, even though they contain "Copilot" strings.

- **Why:** Project convention treats Ultralight UI as off-limits for proposals; the Qt widget path is the maintained surface for this change.
- **Consequence:** The Ultralight renderer may still show "Copilot" until a dedicated Ultralight change addresses it. This is accepted and recorded as a known limitation.

### Decision: Keep GitHub Copilot provider branding

`plugins/copilot/plugin.json`, the provider's `displayName()`, the "Sign in to GitHub Copilot" dialog, its settings title, and the bundled `plugin_registry.json` entry are unchanged.

- **Why:** These genuinely denote the GitHub Copilot product; removing them would misrepresent the integration.

## Risks / Trade-offs

- [Ultralight renderer still shows "Copilot"] → Accepted and documented; Ultralight UI is excluded by convention and handled separately.
- [Existing tests assert old strings] → Search tests for the affected strings and update any assertions; the only current test hit (`test_pluginmanifest.cpp`) uses "Copilot" as an arbitrary manifest fixture and is not user-visible, so it stays.
- [Persisted session titles still read "Copilot"] → Accepted; the fallback title is neutral, and stored titles are user data, not branding. No migration.
- [New source strings are untranslated] → `tr()` strings fall back to English until translations are regenerated; note the new strings for the translation pass.
- [Over-reach into internal identifiers] → Constrain edits to the enumerated files and string literals; do not rename symbols.

## Migration Plan

No data migration. The change is a set of string edits; rollback is reverting the commit. No settings, endpoints, or file formats change.