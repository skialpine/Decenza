## 0. Review
- [x] 0.1 Review proposal decisions and tab structure with stakeholder
- [x] 0.2 Finalize tab ordering and within-tab card ordering
- [x] 0.3 Decide settings search UX (modal dialog, not dropdown)

## 1. Settings Search
- [ ] 1.1 Create `SettingsSearchIndex.js` — static JS array with `{ tabIndex, cardId, title, description, keywords }` for all settings
- [ ] 1.2 Create `SettingsSearchDialog.qml` — modal Dialog with text field, filtered results list using `AccessibleButton` delegates, tab badge per result
- [ ] 1.3 Add search icon to tab bar in `SettingsPage.qml` (right end of tab bar)
- [ ] 1.4 Implement navigate-to-tab + scroll-to-card + highlight behavior on result tap
- [ ] 1.5 Add `objectName`/`id` to all setting cards across all tabs for scroll-to targeting
- [ ] 1.6 Add new files to `CMakeLists.txt`

## 2. Rename Preferences → Machine
- [ ] 2.1 Rename `SettingsPreferencesTab.qml` → `SettingsMachineTab.qml`
- [ ] 2.2 Update `SettingsPage.qml` tab button text from "Preferences" to "Machine"
- [ ] 2.3 Update translation keys (`settings.tab.preferences` → `settings.tab.machine`)
- [ ] 2.4 Update `CMakeLists.txt` file reference

## 3. Create Calibration Tab
- [ ] 3.1 Create `SettingsCalibrationTab.qml` with 5 cards: Flow Calibration, Weight Stop Timing, Heater Calibration, Virtual Scale, Prefer Weight over Volume
- [ ] 3.2 Move card QML from Machine tab to Calibration tab (cut-paste, preserve bindings)
- [ ] 3.3 Order cards: Flow Cal → Weight Stop Timing → Heater Cal → Virtual Scale → Prefer Weight over Volume
- [ ] 3.4 Add to `CMakeLists.txt` and `SettingsPage.qml` tab bar

## 4. Rename Setting Cards
- [ ] 4.1 Rename "Per-Screen Scale" → "Screen Zoom" (card title + translation key)
- [ ] 4.2 Rename "Close Shot Review Screen" → "Shot Review Timer" (card title + translation key)
- [ ] 4.3 Rename "Unlock GUI" toggle → "Simulation Mode" (toggle label + translation key)
- [ ] 4.4 Rename "Ignore Stop-at-Volume with Scale" → "Prefer Weight over Volume" (card title + translation key)
- [ ] 4.5 Rename "Stop-at-Weight Calibration" → "Weight Stop Timing" (card title + translation key)

## 5. Reorder Machine Tab Cards
- [ ] 5.1 Reorganize Machine tab into 3-column layout: Power & Schedule (left), App Behavior (middle), Water & Features (right)
- [ ] 5.2 Left column order: Auto-Sleep, Auto-wake Schedule, Battery/Charging
- [ ] 5.3 Middle column order: Theme Mode, Shot Review Timer, Screen Zoom, Simulation Mode, Launcher Mode
- [ ] 5.4 Right column order: Water Level Status, Water Refill Threshold, Refill Kit, Shot Map

## 6. Merge History + Data
- [ ] 6.1 Merge History and Data tabs into three-column layout: Shot History | Backup | Server & Data
- [ ] 6.2 Remove duplicate `shotServerEnabled` toggle (keep only in Server & Data column)
- [ ] 6.3 Move Device Migration to a dialog — add "Import from Another Device" button in Server & Data column
- [ ] 6.4 Keep Factory Reset at bottom of Server & Data column
- [ ] 6.5 Update tab button text and translation keys
- [ ] 6.6 Remove `SettingsShotHistoryTab.qml` or `SettingsDataTab.qml` (whichever is emptied)

## 7. Merge Update + About
- [ ] 7.1 Add About content (story, donate button, PayPal QR, credits) to left column of Update tab, below update toggles, separated by divider
- [ ] 7.2 Keep release notes in right column at full width — no shrinkage
- [ ] 7.3 Rename tab from "Update" to "About"
- [ ] 7.4 Update translation keys
- [ ] 7.5 Delete `SettingsAboutTab.qml`, remove from `CMakeLists.txt`

## 8. Merge Language + Accessibility
- [ ] 8.1 Add Accessibility content (TTS toggle, tick sounds, extraction announcements) to Language tab
- [ ] 8.2 Rename tab from "Language" to "Language & Access"
- [ ] 8.3 Update translation keys
- [ ] 8.4 Delete `SettingsAccessibilityTab.qml`, remove from `CMakeLists.txt`

## 9. Update Tab Bar
- [ ] 9.1 Reorder tabs in `SettingsPage.qml`: Connections, Machine, Calibration, History & Data, Themes, Layout, Screensaver, Visualizer, AI, MQTT, Language & Access, About, (Debug)
- [ ] 9.2 Update lazy loading indices
- [ ] 9.3 Update accessibility tab name announcements array
- [ ] 9.4 Update `goToSettings()` tab indices in `main.qml` for deep-links
- [ ] 9.5 Update cross-tab reference strings ("Settings → Shot History" → "Settings → History & Data", etc.)

## 10. Update Search Index
- [ ] 10.1 Update `SettingsSearchIndex.js` tab indices to match final tab order
- [ ] 10.2 Verify all cards have `objectName` set for scroll targeting

## 11. Cleanup
- [ ] 11.1 Verify all settings bindings still work (every Q_PROPERTY referenced in old tabs is referenced in new tabs)
- [ ] 11.2 Verify all dialogs still open correctly from their new locations (TOTP setup, factory reset, restore confirm, heater calibration warning)
- [ ] 11.3 Remove any orphaned translation keys

## 12. Testing
- [ ] 12.1 Manual test: settings search — type partial matches, verify navigate + highlight
- [ ] 12.2 Manual test: settings search accessibility — TalkBack/VoiceOver navigate results, activate with double-tap
- [ ] 12.3 Manual test: Machine tab — all ~12 cards read and write correctly, renamed cards show new labels
- [ ] 12.4 Manual test: Calibration tab — all 5 cards read and write correctly, renamed cards show new labels
- [ ] 12.5 Manual test: merged History & Data tab — server toggle, security, backup, DE1 import, device migration dialog
- [ ] 12.6 Manual test: merged About tab — version info, update controls, release notes scroll, donate button, credits
- [ ] 12.7 Manual test: merged Language & Access tab — language selection, accessibility toggles
- [ ] 12.8 Manual test: no duplicate server toggle exists anywhere
- [ ] 12.9 Accessibility: all moved elements retain Accessible properties; search dialog is keyboard/TalkBack navigable
- [ ] 12.10 Manual test: deep-links from other pages (e.g., "Settings → History & Data") navigate to correct tab
