# Dynamic Layout System

The IdlePage (home screen) uses a dynamic, user-configurable layout system. Users can place any widget (action buttons, readouts, labels) in any zone, with adaptive rendering based on zone type.

## Architecture Overview

```
Settings (C++)                    IdlePage (QML)
  layoutConfiguration  ────────>  JSON.parse() ─> zone item arrays
  Q_PROPERTY (JSON string)        ├── topLeftItems
  getZoneItems()                  ├── topRightItems
  addItem() / removeItem()       ├── centerStatusItems
  moveItem() / reorderItem()     ├── centerTopItems
  resetLayoutToDefault()         ├── centerMiddleItems
  hasItemType()                  ├── bottomLeftItems
                                  └── bottomRightItems
                                        │
                                        ▼
                              LayoutBarZone / LayoutCenterZone
                                        │
                                        ▼
                              LayoutItemDelegate (Loader)
                                        │
                                        ▼
                              items/EspressoItem.qml, etc.
```

## Zones

| Zone Name | Location | Rendering | Default Contents |
|-----------|----------|-----------|------------------|
| `topLeft` | Top of IdlePage, left | Compact | *(empty)* |
| `topRight` | Top of IdlePage, right | Compact | *(empty)* |
| `centerStatus` | Center area, top row | Full-size (despite bar container) | Temperature, Water Level, Connection |
| `centerTop` | Center area, main row | Full-size (calculated button sizing) | Espresso, Steam, Hot Water, Flush |
| `centerMiddle` | Center area, below buttons | Full-size | Shot Plan |
| `bottomLeft` | Bottom bar, left | Compact | Sleep |
| `bottomRight` | Bottom bar, right | Compact | Settings |

**Compact vs full-size**: Determined by zone name prefix. `top*` and `bottom*` zones render compact. `center*` zones render full-size. This is computed in `LayoutItemDelegate.qml`:
```qml
readonly property bool isCompact: zoneName.startsWith("top") || zoneName.startsWith("bottom")
```

## Data Model

### Storage

Layout is stored as a JSON string in QSettings under `layout/configuration`. The `Settings` class exposes it as:

```cpp
Q_PROPERTY(QString layoutConfiguration READ layoutConfiguration
           WRITE setLayoutConfiguration NOTIFY layoutConfigurationChanged)
```

### JSON Format

```json
{
  "version": 1,
  "zones": {
    "topLeft": [],
    "topRight": [],
    "centerStatus": [
      {"type": "temperature", "id": "temp1"},
      {"type": "waterLevel", "id": "water1"},
      {"type": "connectionStatus", "id": "conn1"}
    ],
    "centerTop": [
      {"type": "espresso", "id": "espresso1"},
      {"type": "steam", "id": "steam1"},
      {"type": "hotwater", "id": "hotwater1"},
      {"type": "flush", "id": "flush1"}
    ],
    "centerMiddle": [
      {"type": "shotPlan", "id": "plan1"}
    ],
    "bottomLeft": [
      {"type": "sleep", "id": "sleep1"}
    ],
    "bottomRight": [
      {"type": "settings", "id": "settings1"}
    ]
  }
}
```

Each item has a `type` (widget type string) and `id` (unique instance identifier like `"steam2"`).

### C++ API (settings.h/cpp)

| Method | Description |
|--------|-------------|
| `getZoneItems(zoneName)` | Returns `QVariantList` of items in a zone |
| `addItem(type, zone, index=-1)` | Creates new item with auto-generated ID, appends or inserts |
| `removeItem(itemId, zone)` | Removes item by ID from a zone |
| `moveItem(itemId, fromZone, toZone, toIndex)` | Moves item between zones |
| `reorderItem(zoneName, fromIndex, toIndex)` | Swaps item position within a zone |
| `resetLayoutToDefault()` | Clears stored layout (reverts to `defaultLayoutJson()`) |
| `hasItemType(type)` | Returns true if any zone contains an item of this type |
| `generateItemId(type)` | Creates unique ID like `"steam2"` by finding max existing number |

All mutating methods call `saveLayoutObject()` which persists to QSettings and emits `layoutConfigurationChanged`.

## Widget Types

### Action Buttons (with presets)

| Type | Label | Presets | Compact Behavior |
|------|-------|---------|------------------|
| `espresso` | Espresso | Favorite profiles | Popup with PresetPillRow |
| `steam` | Steam | Pitcher presets | Popup with PresetPillRow |
| `hotwater` | Hot Water | Vessel presets | Popup with PresetPillRow |
| `flush` | Flush | Flush presets | Popup with PresetPillRow |
| `beans` | Beans | Bean presets | Popup with PresetPillRow |

### Action Buttons (no presets)

| Type | Label | Action |
|------|-------|--------|
| `history` | History | Navigate to shot history |
| `autofavorites` | Favorites | Navigate to auto-favorites |
| `sleep` | Sleep | Put machine to sleep |
| `settings` | Settings | Navigate to settings page |

### Display Widgets

| Type | Label | Shows |
|------|-------|-------|
| `temperature` | Temp / Temperature | Group head current/target temperature |
| `waterLevel` | Water / Water Level | Water tank level (ml or %) |
| `connectionStatus` | Connection | Online/Offline + scale info |
| `scaleWeight` | Scale | Current scale weight readout |
| `shotPlan` | Shot Plan | Shot plan summary text |

## File Structure

```
qml/components/layout/
  LayoutItemDelegate.qml        # Loader factory: type string -> item QML file
  LayoutBarZone.qml             # RowLayout container for bar zones
  LayoutCenterZone.qml          # RowLayout container with calculated button sizing
  items/
    EspressoItem.qml            # Dual-mode: full ActionButton / compact with popup
    SteamItem.qml               #   "
    HotWaterItem.qml            #   "
    FlushItem.qml               #   "
    BeansItem.qml               #   "
    HistoryItem.qml             # Dual-mode: full ActionButton / compact icon+text
    AutoFavoritesItem.qml       #   "
    SleepItem.qml               #   "
    SettingsItem.qml            #   "
    TemperatureItem.qml         # Dual-mode: full large readout / compact text
    WaterLevelItem.qml          #   "
    ConnectionStatusItem.qml    #   "
    ScaleWeightItem.qml         #   "
    ShotPlanItem.qml            # Shot plan text (full or compact)

qml/pages/settings/
  SettingsLayoutTab.qml         # Layout editor settings page
  LayoutEditorZone.qml          # Zone card component for editor
```

## Component Details

### LayoutItemDelegate.qml

A `Loader` that maps type strings to item QML files via a switch statement. Key properties:

```qml
required property var modelData    // {type: "espresso", id: "espresso1"}
required property string zoneName  // "centerTop", "bottomLeft", etc.
readonly property string itemType  // computed from modelData.type
readonly property string itemId    // computed from modelData.id
readonly property bool isCompact   // true for top*/bottom* zones
```

On load, it binds `isCompact` and `itemId` to the loaded item and sets `item.anchors.fill = root`.

**Qt 6 gotcha**: The delegate declares `required property var modelData` explicitly. In Qt 6, when a delegate has any `required property` declarations, the implicit `modelData` injection is suppressed. Without this explicit declaration, `modelData` would be undefined and all items would fail to load silently.

### LayoutBarZone.qml

Simple `Item` with a `RowLayout` containing a `Repeater` over the items. Used for top, bottom, and centerStatus zones. `implicitHeight` is `Theme.bottomBarHeight`.

### LayoutCenterZone.qml

`Item` with a centered `RowLayout`. Calculates button sizing based on item count and available width:

```qml
readonly property real buttonWidth: Math.min(Theme.scaled(150), availableWidth / buttonCount)
readonly property real buttonHeight: Theme.scaled(120)
```

Each delegate gets `Layout.preferredWidth: root.buttonWidth` and `Layout.preferredHeight: root.buttonHeight`.

### Item Components

Every item component has two required properties:
- `isCompact: false` (bool) - rendering mode
- `itemId: ""` (string) - unique instance ID

Each item has two visual modes:
- **Full mode** (`!isCompact`): Used in center zones. Action buttons use `ActionButton`, display widgets show large readouts.
- **Compact mode** (`isCompact`): Used in bar zones. Shows icon + text in a row. Action buttons with presets show a `Popup` containing `PresetPillRow` on tap.

### Preset Handling

**In center zones** (full mode): Clicking an action button toggles `idlePage.activePresetFunction`. The IdlePage contains inline `PresetPillRow` loaders that expand/collapse below the button row with animation. This is shared state -- all instances of preset buttons in center zones use the same `activePresetFunction`.

**In bar zones** (compact mode): Clicking opens a `Popup` positioned above (for bottom bar) or below (for top bar). Each compact item manages its own popup independently. The popup contains a `PresetPillRow` with the relevant presets.

Items find the IdlePage via parent chain traversal:
```qml
property var idlePage: {
    var p = root.parent
    while (p) {
        if (p.objectName === "idlePage") return p
        p = p.parent
    }
    return null
}
```

## IdlePage Structure

The IdlePage consumes layout configuration and renders zones:

```
┌──────────────────────────────────────────────┐
│  LayoutBarZone(topLeft)  │  LayoutBarZone(topRight)  │  ← compact
├──────────────────────────────────────────────┤
│                                              │
│  LayoutBarZone(centerStatus)                 │  ← full-size readouts
│  LayoutCenterZone(centerTop)                 │  ← full-size action buttons
│  [Inline preset rows - animated expand]      │
│  LayoutCenterZone(centerMiddle)              │  ← full-size info widgets
│                                              │
├──────────────────────────────────────────────┤
│  LayoutBarZone(bottomLeft) │ LayoutBarZone(bottomRight) │  ← compact
└──────────────────────────────────────────────┘
```

The layout JSON is parsed in QML via `JSON.parse(Settings.layoutConfiguration)` and split into per-zone arrays (`topLeftItems`, `centerTopItems`, etc.).

## Layout Editor (Settings)

Located in Settings > Layout tab.

### Files
- `SettingsLayoutTab.qml` - Main editor page with zone cards and widget pool
- `LayoutEditorZone.qml` - Individual zone card showing current items

### UX Flow

1. **Select target zone**: Tap a zone card to select it (highlighted with primary color border/tint)
2. **Add widget**: Tap a widget from the "Add Widgets" pool at the bottom to add it to the selected zone
3. **Select item**: Tap an item chip in any zone to select it (highlighted)
4. **Move item**: With an item selected, tap a different zone to move it there
5. **Reorder**: When an item is selected, left/right arrows appear for reordering within the zone
6. **Remove**: When an item is selected, an X button appears to remove it
7. **Deselect**: Tap the selected item again to deselect

The selected target zone is tracked via `selectedTargetZone` property. The "Add Widgets" section shows an arrow indicator (`→ Center - Action Buttons`) showing where new widgets will be added.

### Zone Display Names

| Zone | Display Name |
|------|-------------|
| `topLeft` | Top Bar (Left) |
| `topRight` | Top Bar (Right) |
| `centerStatus` | Center - Top |
| `centerTop` | Center - Action Buttons |
| `centerMiddle` | Center - Info |
| `bottomLeft` | Bottom Bar (Left) |
| `bottomRight` | Bottom Bar (Right) |

## Known Issues and Gotchas

### Qt 6 required property suppresses modelData

When a Repeater delegate has `required property` declarations, Qt 6 does NOT inject the implicit `modelData` context variable. The delegate must explicitly declare `required property var modelData`. Without this, `modelData.type` evaluates to undefined and items silently fail to render.

### Font double-assignment

QML does not allow `font: Theme.bodyFont` followed by `font.bold: true`. Use individual properties:
```qml
// WRONG
font: Theme.bodyFont
font.bold: true  // Error: "Property has already been assigned a value"

// CORRECT
font.family: Theme.bodyFont.family
font.pixelSize: Theme.bodyFont.pixelSize
font.bold: true
```

### Popup binding loops

Popup width/x binding loops occur when `x: -width / 2 + ...` reads `width` which depends on content `implicitWidth`. Fix by setting explicit `width` on the Popup:
```qml
width: Theme.scaled(600) + 2 * padding
```

### Loader sizing

Loaded items don't auto-fill the Loader. Must explicitly bind in `onLoaded`:
```qml
onLoaded: {
    item.anchors.fill = root
}
```

## Debug Logging

All layout-related logging is tagged with `[IdlePage]` for easy filtering. Logging is present in:
- `LayoutItemDelegate.qml` - type, zone, source, load status, sizing
- `LayoutBarZone.qml` - item counts, dimensions
- `LayoutCenterZone.qml` - item counts, button sizing
- `IdlePage.qml` - JSON parsing, zone item counts

## Pending Work

- **StatusBar customization**: The plan includes making the StatusBar show custom content on IdlePage (replacing default left/right with `topLeft`/`topRight` zone items). Currently the top zones render inside IdlePage itself, not in the system StatusBar.
- **Visibility toggle replacement**: `Settings.showHistoryButton`, `Settings.visualizerExtendedMetadata`, and `Settings.autoFavoritesEnabled` should be replaced with `Settings.hasItemType("history")`, `Settings.hasItemType("beans")`, and `Settings.hasItemType("autofavorites")` respectively.
- **Debug logging cleanup**: `[IdlePage]` logging in LayoutItemDelegate, LayoutBarZone, LayoutCenterZone, and IdlePage should be removed once the system is stable.
