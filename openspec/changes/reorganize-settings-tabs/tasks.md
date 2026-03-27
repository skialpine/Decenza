## 0. Review
- [ ] 0.1 Review proposal decisions 1-9 and open questions with stakeholder
- [ ] 0.2 Finalize tab structure based on feedback
- [ ] 0.3 Decide: Device Migration as dialog flow or inline column (Decision 8)
- [ ] 0.4 Decide: Screensaver video categories — keep as-is or on-demand (Decision 9)

## 1. Create New Tab Files
- [ ] 1.1 Create `SettingsMachineTab.qml` — Auto-Sleep, Auto-Wake, Battery, Water Level, Refill Kit, Refill Threshold, Steam Heater, Headless, Flow Cal, SAW Cal, Heater Cal, Offline Mode
- [ ] 1.2 Create `SettingsDisplayTab.qml` — Theme mode (follow system, dark/light selectors), Extraction View, Per-Screen Scale, Launcher Mode (if decided here)
- [ ] 1.3 Create `SettingsEspressoTab.qml` — Post-Shot Review Close timer, Ignore SAV with Scale, Shot Map (if decided here)

## 2. Move Settings Between Existing Tabs
- [ ] 2.1 Move Virtual Scale card from Preferences to Connections tab (Active Devices section)
- [ ] 2.2 Move theme mode selectors (follow system, dark/light dropdowns) from Preferences to Display tab
- [ ] 2.3 Move REST API documentation section from MQTT tab to the merged History & Data tab (Server & Sharing column)

## 3. Merge Tabs
- [ ] 3.1 Merge History + Data tabs into three-column layout: Shot History | Server & Sharing | Backup; remove duplicate `shotServerEnabled` toggle
- [ ] 3.2 Merge Update + About tabs into single "About" tab

## 4. Restructure Existing Tabs (internal reorganization)
- [ ] 4.1 AI tab: convert from single scrolling column to two-column layout (Provider left, MCP right)
- [ ] 4.2 Connections tab: restructure scale/refractometer column into sections (Active Devices → Known Devices → Find Devices → Log)
- [ ] 4.3 (If Decision 8 = dialog) Create `DeviceMigrationDialog.qml` — stepped modal with search, auth, manifest, import states; add "Import from Another Device" button + Factory Reset to merged History & Data tab
- [ ] 4.4 (If Decision 8 = inline) Add Device Migration as fourth column in merged History & Data tab

## 5. Update Tab Bar
- [ ] 5.1 Update `SettingsPage.qml` tab bar: add new tabs, remove Preferences, remove old About, reorder, update tab indices and lazy loading
- [ ] 5.2 Add new QML files to `CMakeLists.txt`
- [ ] 5.3 Remove deleted QML files from `CMakeLists.txt`

## 6. Cleanup
- [ ] 6.1 Delete `SettingsPreferencesTab.qml` (all content moved to Machine/Display/Espresso/Connections)
- [ ] 6.2 Delete `SettingsAboutTab.qml` (merged into About/Update)
- [ ] 6.3 Verify all settings bindings still work (every Q_PROPERTY referenced in old tabs is referenced in new tabs)
- [ ] 6.4 Verify all dialogs still open correctly from their new locations (TOTP setup, factory reset, restore confirm, migration auth)

## 7. Testing
- [ ] 7.1 Manual test: every setting in Machine tab reads and writes correctly
- [ ] 7.2 Manual test: every setting in Display tab reads and writes correctly
- [ ] 7.3 Manual test: every setting in Espresso tab reads and writes correctly
- [ ] 7.4 Manual test: merged History & Data tab — server toggle, security, backup, DE1 import all work
- [ ] 7.5 Manual test: Device Migration full flow (search → connect → auth → manifest → import)
- [ ] 7.6 Manual test: Virtual Scale in Connections tab
- [ ] 7.7 Manual test: theme mode in Display tab
- [ ] 7.8 Manual test: AI tab two-column layout — provider switching, MCP enable/disable
- [ ] 7.9 Manual test: no duplicate server toggle exists anywhere
- [ ] 7.10 Accessibility: all moved elements retain Accessible properties; dialog flow (if chosen) is keyboard/TalkBack navigable
