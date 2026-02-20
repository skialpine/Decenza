# Web Theme Editor Design

**Date:** 2026-02-19
**Status:** Approved

## Overview

Add a web-based theme editor served by ShotServer at `/theme`, allowing users to change colors and font sizes from a browser while the QML app updates in real-time via reactive Settings bindings. Bidirectional sync via SSE keeps multiple clients (web + QML) in sync.

## Architecture

### Approach: Dedicated `/api/theme` endpoints + SSE

Purpose-built REST endpoints delegate to existing `Settings` methods. SSE stream (same pattern as layout editor) provides bidirectional real-time sync.

## API Endpoints

| Method | Path | Body | Response | Purpose |
|--------|------|------|----------|---------|
| `GET` | `/theme` | -- | HTML | Theme editor web UI |
| `GET` | `/api/theme` | -- | JSON | Full theme state |
| `POST` | `/api/theme/color` | `{"name":"primaryColor","value":"#4e85f4"}` | 200 | Set single color |
| `POST` | `/api/theme/font` | `{"name":"bodySize","value":18}` | 200 | Set single font size |
| `POST` | `/api/theme/preset` | `{"name":"Ocean"}` | JSON | Apply preset theme |
| `POST` | `/api/theme/palette` | `{"hue":220,"saturation":75,"lightness":55}` | JSON | Generate & apply palette |
| `POST` | `/api/theme/save` | `{"name":"My Theme"}` | 200 | Save current as named theme |
| `POST` | `/api/theme/reset` | -- | JSON | Reset to defaults |
| `DELETE` | `/api/theme/preset/{name}` | -- | 200 | Delete user theme |
| `GET` | `/api/theme/subscribe` | -- | SSE | Real-time theme change events |

### GET /api/theme Response

```json
{
  "activeThemeName": "Ocean",
  "colors": {
    "backgroundColor": "#1a1a2e",
    "surfaceColor": "#303048",
    "primaryColor": "#4e85f4",
    ...all 30+ colors
  },
  "fonts": {
    "headingSize": 32,
    "titleSize": 24,
    "subtitleSize": 18,
    "bodySize": 18,
    "labelSize": 14,
    "captionSize": 12,
    "valueSize": 48,
    "timerSize": 72
  },
  "presets": [
    {"name": "Default", "isBuiltIn": true, "primaryColor": "#4e85f4"},
    ...
  ]
}
```

## Web UI Layout

```
+-----------------------------------------------------------+
|  Menu    Theme Editor                                      |
+------------------------+----------------------------------+
|                        |                                  |
|  Core UI               |   Fonts                          |
|    Background [#1a1a]  |   Heading ------*------ 32px    |
|    Surface    [#3030]  |   Title   -----*------- 24px    |
|    Primary    [#4e85]  |   Body    ----*-------- 18px    |
|    ...                 |   ...                            |
|                        |                                  |
|  Status                |   Presets                         |
|    Success    [#00cc]  |   [Default] [Ocean] [Warm] [+]  |
|    Warning    [#ffaa]  |   [Random Theme]                  |
|    ...                 |                                  |
|                        |                                  |
|  Chart                 |                                  |
|    Pressure   [#18c3]  |                                  |
|    Flow       [#4e85]  |                                  |
|    ...                 |                                  |
|                        |                                  |
+------------------------+----------------------------------+
|  [Reset to Default]                     [Save Theme...]   |
+-----------------------------------------------------------+
```

- Left panel: Color categories with native `<input type="color">` + hex text input
- Right panel: Font size sliders with numeric display, preset buttons, palette generator
- No preview panel -- the actual app on the device IS the live preview

## Real-Time Sync (SSE)

### Web -> App

1. User picks color in browser
2. JS sends `POST /api/theme/color` with `{name, value}`
3. ShotServer calls `Settings::setThemeColor(name, value)`
4. Settings emits `customThemeColorsChanged`
5. Theme.qml bindings update -> QML UI refreshes instantly
6. ShotServer slot pushes SSE `theme-changed` to all connected browsers

### App -> Web

1. User changes color on QML settings tab
2. Settings emits `customThemeColorsChanged`
3. ShotServer slot pushes SSE `theme-changed`
4. Web UI receives event, fetches `GET /api/theme`, updates all pickers

### Debouncing

JS debounces POST calls to ~50ms during color picker drag. Self-echo from SSE is ignored by tracking `lastSentTimestamp`.

## Font Size Persistence

### Settings Changes

New `Q_PROPERTY`:
```cpp
Q_PROPERTY(QVariantMap customFontSizes READ customFontSizes
           WRITE setCustomFontSizes NOTIFY customFontSizesChanged)
```

New methods: `setFontSize(name, value)`, `getFontSize(name)`

### Theme.qml Changes

Font properties read from Settings with fallback defaults:
```qml
property font headingFont: Qt.font({
    family: "Inter",
    pixelSize: scaled(Settings.customFontSizes.headingSize || 32),
    bold: true
})
```

Editable sizes: `headingSize`, `titleSize`, `subtitleSize`, `bodySize`, `labelSize`, `captionSize`, `valueSize`, `timerSize`

## File Changes

| File | Change |
|------|--------|
| `src/network/shotserver.h` | Add `m_sseThemeClients`, `onThemeChanged()` slot |
| `src/network/shotserver.cpp` | Add `/theme` page, `/api/theme/*` endpoints, SSE logic |
| `src/network/webtemplates/theme_page.h` | New: HTML/CSS/JS for theme editor |
| `src/core/settings.h` | Add `customFontSizes` property, `setFontSize()`, `getFontSize()` |
| `src/core/settings.cpp` | Implement font size persistence |
| `qml/Theme.qml` | Read font sizes from Settings with fallback defaults |

## SSE Implementation

Mirrors existing layout SSE pattern:
- New member: `QSet<QTcpSocket*> m_sseThemeClients`
- New slot: `onThemeChanged()` iterates clients, sends `event: theme-changed\ndata: {}\n\n`
- Connect to `Settings::customThemeColorsChanged` and `Settings::customFontSizesChanged`
- Cleanup dead connections same as layout SSE

## Design Decisions

1. **Dedicated endpoints over extending /api/settings**: Atomic single-color updates, no over-fetching
2. **SSE over WebSocket**: Proven pattern already in codebase, no WS protocol implementation needed
3. **Native color pickers**: Browser `<input type="color">` is good enough and zero-dependency
4. **Font sizes only (not weights/families)**: Font families and bold/normal are structural -- sizes are what users want to tweak
5. **No preview panel**: The real app is the preview, shown on the device in real-time
