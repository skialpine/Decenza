# Tasks: Extract ProfileManager from MainController

## Phase 1: Mechanical extraction
- [ ] Create `src/controllers/profilemanager.h` with method declarations moved from MainController
- [ ] Create `src/controllers/profilemanager.cpp` with implementations moved from MainController
- [ ] Add `ProfileManager*` member to MainController, create in constructor
- [ ] Replace moved method bodies in MainController with forwarding calls to ProfileManager
- [ ] Update `CMakeLists.txt` to compile `profilemanager.cpp`
- [ ] Build and verify all platforms (Qt Creator)
- [ ] Run existing test suite — all tests pass unchanged

## Phase 2: MCP tool migration
- [ ] Change `registerProfileTools()` to take `ProfileManager*`
- [ ] Change `registerWriteTools()` profile paths to use `ProfileManager*`
- [ ] Update `McpServer` to hold and pass `ProfileManager*`
- [ ] Write `tst_mcptools.cpp` with ProfileManager + MockTransport fixture
- [ ] Verify: `profiles_edit_params` triggers BLE upload (the PR #561 test)
- [ ] Verify: `settings_set` temperature/weight triggers BLE upload
- [ ] Verify: all sync profile tools return correct responses
- [ ] Remove MainController forwarding for methods only used by MCP

## Phase 3: QML migration (optional, deferred)
- [ ] Register `ProfileManager` as QML context property in `main.cpp`
- [ ] Update `ProfileEditorPage.qml` to call `ProfileManager` directly
- [ ] Update `RecipeEditorPage.qml` to call `ProfileManager` directly
- [ ] Update `SimpleProfileEditorPage.qml` to call `ProfileManager` directly
- [ ] Update `BrewDialog.qml` to call `ProfileManager` directly
- [ ] Remove remaining forwarding stubs from MainController
