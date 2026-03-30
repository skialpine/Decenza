// Settings search index — maps setting cards to tab indices for search navigation
// Each entry: { tabIndex, title, description, keywords }
// tabIndex matches the tab order in SettingsPage.qml:
//   0=Connections, 1=Machine, 2=Calibration, 3=History & Data,
//   4=Themes, 5=Layout, 6=Screensaver, 7=Visualizer, 8=AI,
//   9=MQTT, 10=Language & Access, 11=About

function getTabName(index) {
    var names = [
        "Connections", "Machine", "Calibration", "History & Data",
        "Themes", "Layout", "Screensaver", "Visualizer", "AI",
        "MQTT", "Language & Access", "About"
    ]
    return index < names.length ? names[index] : ""
}

function getSearchEntries() {
    return [
        // Tab 0: Connections
        { tabIndex: 0, title: "Machine Connection", description: "Connect to DE1 via Bluetooth or USB", keywords: ["ble", "bluetooth", "usb", "pair", "connect", "device"] },
        { tabIndex: 0, title: "Scale Connection", description: "Pair a Bluetooth scale", keywords: ["scale", "acaia", "decent", "felicita", "weight", "pair"] },
        { tabIndex: 0, title: "Refractometer", description: "Connect a refractometer for TDS readings", keywords: ["tds", "refractometer", "atago"] },

        // Tab 1: Machine
        { tabIndex: 1, title: "Auto-Sleep", description: "Put the machine to sleep after inactivity", keywords: ["sleep", "timeout", "power", "idle", "standby"] },
        { tabIndex: 1, title: "Auto-Wake Timer", description: "Schedule automatic wake-up times", keywords: ["wake", "schedule", "alarm", "morning", "timer", "power"] },
        { tabIndex: 1, title: "Battery Charging", description: "Smart charging mode for battery health", keywords: ["battery", "charge", "usb", "power", "smart charging"] },
        { tabIndex: 1, title: "Theme Mode", description: "Dark mode, light mode, follow system", keywords: ["dark", "light", "theme", "mode", "appearance", "system"] },
        { tabIndex: 1, title: "Extraction View", description: "Choose between shot chart and cup fill view", keywords: ["chart", "cup", "graph", "extraction", "view"] },
        { tabIndex: 1, title: "Shot Review Timer", description: "Auto-close shot review after timeout", keywords: ["review", "post-shot", "timeout", "close", "auto"] },
        { tabIndex: 1, title: "Screen Zoom", description: "Adjust UI scale per screen", keywords: ["zoom", "scale", "size", "ui", "display", "dpi"] },
        { tabIndex: 1, title: "Launcher Mode", description: "Set as Android home screen", keywords: ["launcher", "home", "android", "kiosk"] },
        { tabIndex: 1, title: "Water Level", description: "Water tank level and refill threshold", keywords: ["water", "tank", "level", "refill"] },
        { tabIndex: 1, title: "Water Refill Threshold", description: "Set when to show refill warning", keywords: ["water", "refill", "threshold", "warning"] },
        { tabIndex: 1, title: "Refill Kit", description: "Auto-detect or force refill kit mode", keywords: ["refill", "kit", "plumb", "water", "auto"] },
        { tabIndex: 1, title: "Shot Map", description: "Share shots on the global map", keywords: ["map", "location", "gps", "share", "global"] },
        { tabIndex: 1, title: "Headless Machine", description: "Skip purge confirmation on headless DE1", keywords: ["headless", "purge", "confirm"] },
        { tabIndex: 1, title: "Steam Heater", description: "Keep steam heater on and auto-flush settings", keywords: ["steam", "heater", "flush", "auto", "temperature"] },
        { tabIndex: 1, title: "Simulation Mode", description: "Use app without a connected DE1 machine", keywords: ["offline", "simulation", "demo", "unlock", "gui", "disconnect"] },

        // Tab 2: Calibration
        { tabIndex: 2, title: "Flow Calibration", description: "Calibrate flow sensor accuracy", keywords: ["flow", "calibration", "sensor", "auto", "multiplier"] },
        { tabIndex: 2, title: "Weight Stop Timing", description: "Auto-learned stop-at-weight lag timing", keywords: ["weight", "stop", "saw", "lag", "timing", "scale"] },
        { tabIndex: 2, title: "Heater Calibration", description: "Idle temp, warmup flow rates, timeout", keywords: ["heater", "temperature", "warmup", "calibrate", "idle"] },
        { tabIndex: 2, title: "Virtual Scale", description: "Estimate weight from flow sensor", keywords: ["virtual", "scale", "flow", "weight", "estimate", "fallback"] },
        { tabIndex: 2, title: "Prefer Weight over Volume", description: "Ignore volume limit when scale is paired", keywords: ["weight", "volume", "sav", "ignore", "scale", "stop"] },

        // Tab 3: History & Data
        { tabIndex: 3, title: "Shot History", description: "View and manage shot history", keywords: ["history", "shots", "past", "records"] },
        { tabIndex: 3, title: "Import from DE1 App", description: "Import shot history from DE1 tablet app", keywords: ["import", "de1", "migrate", "transfer"] },
        { tabIndex: 3, title: "Daily Backup", description: "Auto-backup shots, settings, profiles daily", keywords: ["backup", "restore", "save", "data", "auto"] },
        { tabIndex: 3, title: "Enable Server", description: "HTTP server for data sharing and remote access", keywords: ["server", "http", "web", "remote", "network", "share"] },
        { tabIndex: 3, title: "Security", description: "HTTPS encryption and authenticator verification", keywords: ["security", "https", "totp", "authenticator", "password", "encryption"] },
        { tabIndex: 3, title: "Device Migration", description: "Import from another Decenza device on WiFi", keywords: ["migration", "transfer", "device", "import", "wifi", "sync"] },
        { tabIndex: 3, title: "Factory Reset", description: "Remove all data and uninstall", keywords: ["reset", "factory", "delete", "clear", "uninstall", "wipe"] },

        // Tab 4: Themes
        { tabIndex: 4, title: "Theme Colors", description: "Customize app color palette", keywords: ["color", "theme", "palette", "customize", "dark", "light"] },
        { tabIndex: 4, title: "Save Theme", description: "Save current color scheme as named theme", keywords: ["save", "theme", "preset", "custom"] },

        // Tab 5: Layout
        { tabIndex: 5, title: "Layout Editor", description: "Customize idle screen widgets and zones", keywords: ["layout", "widget", "zone", "customize", "idle", "home"] },

        // Tab 6: Screensaver
        { tabIndex: 6, title: "Screensaver", description: "Choose screensaver type and settings", keywords: ["screensaver", "attractor", "pipes", "clock", "video", "image", "dim"] },

        // Tab 7: Visualizer
        { tabIndex: 7, title: "Visualizer", description: "Connect to visualizer.coffee for shot sharing", keywords: ["visualizer", "coffee", "upload", "share", "account"] },

        // Tab 8: AI
        { tabIndex: 8, title: "AI Provider", description: "Configure AI for shot analysis", keywords: ["ai", "openai", "anthropic", "gemini", "ollama", "api", "key", "model"] },
        { tabIndex: 8, title: "MCP Server", description: "Model Context Protocol server for Claude", keywords: ["mcp", "claude", "server", "protocol", "discuss"] },

        // Tab 9: MQTT
        { tabIndex: 9, title: "MQTT", description: "Home automation broker connection", keywords: ["mqtt", "home", "assistant", "automation", "broker", "ha"] },

        // Tab 10: Language & Access
        { tabIndex: 10, title: "Language", description: "Select app language", keywords: ["language", "translate", "locale", "i18n"] },
        { tabIndex: 10, title: "Accessibility", description: "Screen reader and audio feedback settings", keywords: ["accessibility", "talkback", "voiceover", "screen reader", "tts", "blind"] },
        { tabIndex: 10, title: "Voice Announcements", description: "Text-to-speech during extraction", keywords: ["voice", "speech", "tts", "announce", "talk"] },
        { tabIndex: 10, title: "Frame Tick Sound", description: "Audio tick on extraction frame changes", keywords: ["tick", "sound", "audio", "frame", "beep"] },

        // Tab 11: About
        { tabIndex: 11, title: "Check for Updates", description: "Auto-check and download app updates", keywords: ["update", "version", "download", "beta", "release"] },
        { tabIndex: 11, title: "Release Notes", description: "What's new in this version", keywords: ["release", "notes", "changelog", "new", "version"] },
        { tabIndex: 11, title: "Donate", description: "Support Decenza development via PayPal", keywords: ["donate", "paypal", "support", "money", "tip"] },
    ]
}
