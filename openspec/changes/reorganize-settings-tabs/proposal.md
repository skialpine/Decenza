# Change: Reorganize Settings Tabs for Clarity and Discoverability

## Why

The Settings page has grown to 13+ tabs with settings placed wherever there was room rather than where users would look for them. The Preferences tab alone has become a catch-all with 12+ cards spanning machine hardware, display, scheduling, and calibration. Settings like "Enable Server" appear in two tabs (History and Data). Theme mode selection is in Preferences, not Themes. Scale-related settings are split across Connections and Preferences.

A virtual usability study of common user workflows revealed:
- Users who want to switch dark/light theme go to Themes first, then have to backtrack to Preferences
- Virtual Scale is in Preferences, not with other scales in Connections
- "Enable Server" in two tabs (History and Data) confuses users about which is canonical
- REST API docs are in the MQTT tab, but the HTTP server is enabled in History/Data
- Device Migration has a 5-state wizard crammed into a column that's empty 99% of the time
- Steam heater idle behavior is in Preferences, but all other steam settings are on SteamPage
- The proposed Espresso tab would have only 2 items — too thin for its own tab

## What Changes

### Current Tab Structure (13 tabs + 2 conditional)

| # | Tab | Items | Problem |
|---|-----|-------|---------|
| 0 | Connections | Machine BLE/USB, Scale BLE/USB, Refractometer | Missing Virtual Scale |
| 1 | Preferences | Theme mode, Auto-Sleep, Post-Shot Review, Refill Kit, Offline Mode, Launcher Mode, Shot Map, Per-Screen Scale, Battery Charging, Flow Cal, Ignore SAV, Water Level, Headless toggle, Extraction View, Steam Heater, Virtual Scale, Auto-Wake, SAW Cal, Heater Cal | **Catch-all; 12+ unrelated cards** |
| 2 | Screensaver | Screensaver type + per-type config | OK |
| 3 | Visualizer | Account + upload settings | OK |
| 4 | AI | Provider, MCP server, Discuss Shot | OK |
| 5 | Accessibility | TTS, tick sounds, extraction announcements | OK |
| 6 | Themes | Color editor, presets | Missing theme mode selectors |
| 7 | Layout | Idle page widget editor | OK |
| 8 | Language | Language picker + translation tools | OK |
| 9 | History | Shot history link, DE1 import, **Remote Access** | Server toggle duplicated from Data |
| 10 | Data | **Server toggle (duplicate)**, Security, Backup, Device Migration, Factory Reset | Overlaps History |
| 11 | MQTT | MQTT config, Home Assistant, **REST API docs** | REST API doesn't belong here |
| 12 | Update | Version, auto-update, beta toggle | OK |
| 13 | About | Credits, donate | Too small for own tab |
| 14 | Debug | Resolution, simulation, profile converter | OK (debug only) |

### Proposed Tab Structure (11 tabs)

| # | Tab | Contents | Source |
|---|-----|----------|--------|
| 0 | **Connections** | Machine BLE/USB, Scale BLE/USB, Refractometer, Virtual Scale. Internal reorg: Active Devices → Known Devices → Find Devices → Log | Connections + Preferences(Virtual Scale) |
| 1 | **Machine** | Sections: **Power & Sleep** (Auto-Sleep, Auto-Wake, Battery Charging), **Water** (Water Level, Refill Kit, Refill Threshold), **General** (Headless, Offline/Simulation Mode, Ignore SAV with Scale), **Calibration** (Flow Cal, SAW Cal, Heater Cal) | Preferences (hardware items) |
| 2 | **Display** | Theme mode (follow system, dark/light selectors), Extraction View (Chart/Cup Fill), Per-Screen Scale, Post-Shot Review Close timer, Shot Map, Launcher Mode (Android) | Preferences (display items) |
| 3 | **Themes** | Color editor, presets (unchanged) | Themes (minus mode selectors) |
| 4 | **Screensaver** | Unchanged | Screensaver |
| 5 | **Visualizer** | Unchanged | Visualizer |
| 6 | **History & Data** | Three columns + migration dialog. **Shot History** (history link, DE1 import), **Server & Sharing** (server enable + security + TOTP + REST API docs, Factory Reset at bottom), **Backup** (daily backup, restore). "Import from Another Device" button opens stepped dialog. | History + Data merged, REST API from MQTT |
| 7 | **AI** | Two columns: **Provider** (picker, API key, provider-specific fields, test connection) and **MCP Server** (enable, access levels, confirmation, Discuss Shot) | AI (internal reorg) |
| 8 | **Home Assistant** | MQTT config, Home Assistant auto-discovery, Publish Discovery (no REST API section) | MQTT (minus REST API) |
| 9 | **Accessibility** | Unchanged | Accessibility |
| 10 | **Language** | Unchanged | Language |
| 11 | **Layout** | Unchanged | Layout |
| 12 | **About** | Version + update controls + credits + donate | Update + About merged |

### What Moves Where (complete mapping)

| Setting | From | To | Rationale |
|---------|------|----|-----------|
| Theme mode (follow system, dark/light) | Preferences | **Display** | Users try Themes tab first; this is a display preference |
| Auto-Sleep | Preferences | **Machine** | Machine power behavior |
| Auto-Wake Timer | Preferences | **Machine** | Machine scheduling |
| Battery Charging | Preferences | **Machine** | Machine hardware |
| Water Level + Refill Threshold | Preferences | **Machine** | Machine hardware |
| Refill Kit | Preferences | **Machine** | Machine hardware |
| Headless Machine toggle | Preferences | **Machine** | Machine hardware |
| Offline/Simulation Mode | Preferences | **Machine** | Replaces machine connection |
| Ignore SAV with Scale | Preferences | **Machine** | How the machine stops extraction |
| Flow Calibration | Preferences | **Machine** | Machine calibration |
| SAW Calibration | Preferences | **Machine** | Machine calibration |
| Heater Calibration | Preferences | **Machine** | Machine calibration |
| Extraction View (Chart/Cup) | Preferences | **Display** | Visual preference |
| Per-Screen Scale | Preferences | **Display** | Visual preference |
| Post-Shot Review Close timer | Preferences | **Display** | How long a UI element stays visible |
| Shot Map | Preferences | **Display** | Shot metadata display; screensaver tie-in |
| Launcher Mode (Android) | Preferences | **Display** | How the app presents on home screen |
| Virtual Scale (FlowScale) | Preferences | **Connections** | Scale alternative belongs with scales |
| REST API docs | MQTT | **History & Data** | Part of HTTP server, not MQTT |
| Server toggle (duplicate) | History + Data | **History & Data** (single) | Eliminate duplication |
| About content | About tab | **About** (merged with Update) | Too small for own tab |

### What Moves OUT of Settings Entirely

| Setting | From | To | Rationale |
|---------|------|----|-----------|
| `keepSteamHeaterOn` | Preferences (→ would go to Machine) | **SteamPage** settings view | Steam behavior preference; all other steam settings are on SteamPage |
| `steamAutoFlushSeconds` | Preferences (→ would go to Machine) | **SteamPage** settings view | "What happens after steaming" belongs with steam controls |

### What Gets Removed

| Item | Action | Rationale |
|------|--------|-----------|
| `AISettingsPage.qml` | Remove; deep-link to Settings → AI instead | Stale duplicate of AI tab, missing OpenRouter fields |
| Duplicate server toggle in History tab | Remove | Single source of truth in merged History & Data tab |

### What Stays on Operation Pages (confirmed correct)

| Setting | Page | Why it stays |
|---------|------|-------------|
| Steam presets (pitcher, duration, flow, temp) | SteamPage | Adjusted while steaming, needs live feedback |
| Hot water presets (vessel, volume, temp, flow) | HotWaterPage | Contextual adjustment during operation |
| Flush presets (flow, duration) | FlushPage | Contextual adjustment during operation |
| Bean/DYE metadata | BeanInfoPage, PostShotReviewPage | Pre/post-shot workflow, not a preference |
| Profile selection | IdlePage, ProfileSelectorPage | Changes per shot |
| Shot history sort | ShotHistoryPage | Display preference for that specific page |
| AutoFavorites preferences (groupBy, maxItems) | AutoFavoritesPage gear popup | Niche feature preferences, in-context |

## Decisions (finalized from usability study)

**Decision 1: Screensaver — own tab.** 6 screensaver types with unique sub-settings. Too much for Display.

**Decision 2: AI and MQTT — separate tabs.** Different audiences, both complex. MQTT is primarily for Home Assistant users. Renamed to "Home Assistant" to match its primary use case.

**Decision 3: Shot Map — Display tab.** It has screensaver tie-in (Shot Map screensaver type) and is a display/metadata preference. The Espresso tab was eliminated (too thin).

**Decision 4: Offline/Simulation Mode — Machine tab.** It replaces the machine connection.

**Decision 5: Launcher Mode — Display tab.** It's how the app presents on the Android home screen.

**Decision 6: Update + About — merge.** About is too small for its own tab.

**Decision 7: History + Data — merge.** Eliminates duplicate server toggle. Four-section layout with migration as dialog.

**Decision 8: Device Migration — dialog flow.** The 5-state wizard is a rare operation that clutters the common case. A stepped modal matches existing patterns (TOTP setup, ZIP extraction).

**Decision 9: Screensaver video categories — keep as-is.** Clean binary toggle.

**Decision 10 (new): Espresso tab — eliminated.** Only 2 items (Post-Shot Review timer, Ignore SAV) didn't justify a tab. Post-Shot Review timer → Display (UI duration), Ignore SAV → Machine (extraction behavior).

**Decision 11 (new): Steam heater settings — move to SteamPage.** `keepSteamHeaterOn` and `steamAutoFlushSeconds` are steam behavior preferences. All other steam settings already live on SteamPage.

**Decision 12 (new): AISettingsPage duplicate — remove.** Deep-link to Settings → AI tab instead. Current duplicate is missing OpenRouter fields.

## Cross-Tab References to Update

These user-visible strings reference old tab names and need updating:

| File | Current text | New text |
|------|-------------|----------|
| `main.qml` | "Settings → Bluetooth" | "Settings → Connections" (already correct?) |
| `ConversationOverlay.qml` | "Settings → Shot History" | "Settings → History & Data" |
| `SettingsHomeAutomationTab.qml` | "Shot History tab" | Remove (REST API moves to History & Data) |
| `SettingsDataTab.qml` | "Shot History settings" | "History & Data" |

## Impact

- Affected code:
  - `qml/pages/SettingsPage.qml` — tab bar restructure (13 → 11 tabs)
  - `qml/pages/settings/SettingsPreferencesTab.qml` — decomposed into Machine + Display
  - `qml/pages/settings/SettingsShotHistoryTab.qml` — merged with Data
  - `qml/pages/settings/SettingsDataTab.qml` — merged with History, remove duplicate server toggle, migration becomes dialog
  - `qml/pages/settings/SettingsHomeAutomationTab.qml` — remove REST API section, rename to Home Assistant
  - `qml/pages/settings/SettingsConnectionsTab.qml` — receive Virtual Scale, internal reorg (Active → Known → Find → Log)
  - `qml/pages/settings/SettingsAITab.qml` — two-column internal reorg
  - `qml/pages/settings/SettingsUpdateTab.qml` — merge with About
  - `qml/pages/settings/SettingsAboutTab.qml` — merge into Update/About
  - `qml/pages/SteamPage.qml` — receive `keepSteamHeaterOn` + `steamAutoFlushSeconds`
  - `qml/pages/AISettingsPage.qml` — remove, replace with deep-link to Settings → AI
  - New files: `SettingsMachineTab.qml`, `SettingsDisplayTab.qml`, `DeviceMigrationDialog.qml`
  - Removed files: `SettingsPreferencesTab.qml` (split), `SettingsAboutTab.qml` (merged), `AISettingsPage.qml` (duplicate removed)
  - String updates: 3-4 cross-tab reference strings in QML files
  - `qml/main.qml` — update `goToSettings()` tab indices for deep-links (Data restore, Update notification)
