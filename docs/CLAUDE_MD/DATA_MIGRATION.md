## Data Migration (Device-to-Device Transfer)

Transfer profiles, shots, settings, media, and AI conversations between devices over WiFi.

### Architecture
- **Server**: `ShotServer` (same as Remote Access) exposes `/api/backup/*` REST endpoints
- **Client**: `DataMigrationClient` discovers servers and imports data
- **Discovery**: UDP broadcast on port 8889 for automatic device finding, with manual IP:port fallback
- **Authentication**: TOTP-based authentication when server requires it (QR code + manual code entry, session token caching)
- **Default port**: 8888 (configurable in Settings → Shot History → Port)
- **UI**: Settings → Data tab

### Key Files
- `src/core/datamigrationclient.cpp/.h` - Client-side discovery, authentication, and import
- `src/network/shotserver_backup.cpp` - Server-side backup endpoint handlers
- `src/network/shotserver.cpp` - Route dispatch (routes `/api/backup/*` to handlers)
- `qml/pages/settings/SettingsDataTab.qml` - UI

### How It Works
1. **Source device**: Enable "Remote Access" in Settings → Shot History tab
2. **Target device**: Go to Settings → Data tab, tap "Search"
3. Client broadcasts UDP discovery packet (`DECENZA_DISCOVER`) to port 8889
4. Servers respond with device info (name, URL, platform)
5. Client filters out own device (compares sender IP against local network interfaces), shows discovered servers
6. If discovery fails, user can enter IP:port manually (e.g., `192.168.1.100:8888`)
7. If the server requires authentication, user is prompted for a TOTP code
8. User selects device and imports desired data types

### REST Endpoints (on ShotServer)
- `GET /api/backup/manifest` - List available data (counts, sizes)
- `GET /api/backup/settings` - Export settings JSON
- `GET /api/backup/profiles` - List profile filenames
- `GET /api/backup/profile/{category}/{filename}` - Download single profile
- `GET /api/backup/shots` - List shot IDs
- `GET /api/backup/media` - List media files
- `GET /api/backup/media/{filename}` - Download media file
- `GET /api/backup/ai-conversations` - Export AI conversations
- `GET /api/backup/full` - Download full backup archive
- `POST /api/backup/restore` - Restore from backup archive

### Import Options
- **Import All**: Settings, profiles, shots, media, AI conversations
- **Individual**: Import only specific data types (Settings, Profiles, Shots, Media, AI Conversations buttons)
