## ADDED Requirements

### Requirement: Machine Settings Tab
The app SHALL provide a "Machine" settings tab containing all hardware and scheduling settings: Auto-Sleep, Auto-Wake Timer, Battery Charging, Water Level, Water Refill Threshold, Refill Kit, Steam Heater (idle mode + auto-flush), Headless Machine toggle, Flow Calibration, Stop-at-Weight Calibration, Heater Calibration, and Offline/Simulation Mode.

#### Scenario: User finds Auto-Wake in Machine tab
- **WHEN** user navigates to Settings > Machine
- **THEN** the Auto-Wake Timer card is visible without scrolling to a different tab

#### Scenario: All calibration settings grouped together
- **WHEN** user looks for calibration options
- **THEN** Flow Calibration, SAW Calibration, and Heater Calibration are all in the Machine tab

### Requirement: Display Settings Tab
The app SHALL provide a "Display" settings tab containing: theme mode selection (follow system, dark theme picker, light theme picker), Extraction View choice (Shot Chart / Cup Fill), and Per-Screen Scale toggle.

#### Scenario: User switches between dark and light theme
- **WHEN** user navigates to Settings > Display
- **THEN** the Follow System Theme toggle and Dark/Light theme dropdowns are visible

#### Scenario: Extraction view choice in Display
- **WHEN** user wants to change from Shot Chart to Cup Fill view
- **THEN** the Extraction View radio buttons are in the Display tab

### Requirement: Espresso Settings Tab
The app SHALL provide an "Espresso" settings tab containing shot workflow settings: Post-Shot Review Close timer and Ignore Stop-at-Volume with Scale toggle.

#### Scenario: User configures post-shot review timeout
- **WHEN** user navigates to Settings > Espresso
- **THEN** the Post-Shot Review Close timer stepper is visible

### Requirement: Merged History and Data Tab
The app SHALL provide a single "History & Data" tab combining shot history access, DE1 import, server enable with security, backup/restore, device migration, and factory reset. The server enable toggle SHALL appear exactly once.

#### Scenario: No duplicate server toggle
- **WHEN** user searches for the server enable toggle
- **THEN** it exists in exactly one location (History & Data tab)

#### Scenario: Server security adjacent to server enable
- **WHEN** user enables the server
- **THEN** security settings (HTTPS, TOTP setup) are visible in the same tab

### Requirement: Virtual Scale in Connections Tab
The app SHALL show the Virtual Scale (FlowScale) enable toggle in the Connections tab alongside physical scale and refractometer controls.

#### Scenario: User enables virtual scale
- **WHEN** user navigates to Settings > Connections
- **THEN** the Virtual Scale enable toggle is visible in the scale section

### Requirement: Merged Update and About Tab
The app SHALL provide a single "About" tab containing version info, update controls (auto-check, beta toggle, download), credits, and donate link.

#### Scenario: User checks for updates and sees credits
- **WHEN** user navigates to Settings > About
- **THEN** both the update controls and credits/donate are visible without switching tabs

## REMOVED Requirements

### Requirement: Preferences Tab
**Reason**: The Preferences tab is decomposed into Machine, Display, and Espresso tabs. All settings move to their new homes; no settings are deleted.
**Migration**: Each card moves to the tab matching its mental model (see design.md mapping table).

### Requirement: Separate About Tab
**Reason**: About content (credits, donate) is merged into the Update tab to reduce tab count.
**Migration**: About content appended below update controls.

### Requirement: Separate History and Data Tabs
**Reason**: These tabs had overlapping server controls. Merging eliminates the duplicate `shotServerEnabled` toggle and groups all data management in one place.
**Migration**: History left column + Data columns combined into a single tab.
