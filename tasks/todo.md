# Task List: Multiplayer Refactor

## Task 1: Move protobuf to core module and update build
**Status:** pending

Move `tictactoe-ui-cpp/proto/tictactoe.proto` to `tictactoe/proto/tictactoe.proto`. Update `tictactoe/CMakeLists.txt` to compile protobuf. Update `tictactoe/metadata.json` to include protobuf in nix packages.

**Acceptance criteria:**
- [ ] `tictactoe/proto/tictactoe.proto` exists (same content as C++ UI version)
- [ ] `tictactoe/CMakeLists.txt` compiles protobuf and links it to the plugin
- [ ] `tictactoe/metadata.json` has protobuf in build and runtime nix packages
- [ ] `cd tictactoe && nix build '.#lgx-portable'` succeeds

**Verification:** `nix build` produces a valid LGX.

---

## Task 2: Add multiplayer methods to core module
**Status:** pending  
**Depends on:** Task 1

Add delivery module integration to `tictactoe_plugin.h/.cpp`:
- `enableMultiplayer()`, `disableMultiplayer()` — lifecycle
- `mpStatus()`, `mpConnected()`, `mpMessagesSent()`, `mpMessagesReceived()`, `mpError()` — state queries
- Private: delivery client setup, event handlers, `broadcastMove()`, `broadcastNewGame()`
- `play()` and `newGame()` auto-broadcast when multiplayer enabled
- Emit `eventResponse("remoteMove", ...)`, `eventResponse("remoteNewGame", ...)`, `eventResponse("mpStatusChanged", ...)`
- Update `tictactoe_interface.h` with new virtual methods

**Acceptance criteria:**
- [ ] All methods declared in interface and implemented in plugin
- [ ] `enableMultiplayer()` calls delivery_module: createNode → register events → start → subscribe
- [ ] `disableMultiplayer()` calls unsubscribe → stop, resets state
- [ ] `play()` auto-broadcasts on success when multiplayer connected
- [ ] `newGame()` auto-broadcasts when multiplayer connected
- [ ] Incoming messages decoded from protobuf, applied to game, emit `eventResponse`
- [ ] Connection state changes emit `eventResponse("mpStatusChanged", ...)`
- [ ] `cd tictactoe && nix build '.#lgx-portable'` succeeds

**Verification:** Build succeeds. Code review confirms all delivery module interaction is in core module.

---

## Task 3: Strip multiplayer from C++ UI
**Status:** pending  
**Depends on:** Task 2

Remove all delivery module code from `tictactoe_backend.h/.cpp`. Remove protobuf from C++ UI build. The backend should:
- Call `m_logos->tictactoe.enableMultiplayer()` / `disableMultiplayer()` 
- Query state via `m_logos->tictactoe.mpStatus()` etc.
- Subscribe to tictactoe module events for `remoteMove`, `remoteNewGame`, `mpStatusChanged`
- Remove `proto/` directory, protobuf from CMakeLists.txt and metadata.json

**Acceptance criteria:**
- [ ] `tictactoe_backend.cpp` has zero references to `delivery_module`, `LogosAPIClient`, `LogosObject`, protobuf
- [ ] `tictactoe_backend.h` has no delivery-related fields (`m_deliveryClient`, `m_deliveryObject`, etc.)
- [ ] `proto/` directory removed from C++ UI
- [ ] CMakeLists.txt has no protobuf compilation
- [ ] metadata.json has no protobuf dependency
- [ ] UI still shows multiplayer toggle, connection status, sent/received counts
- [ ] `cd tictactoe-ui-cpp && nix build '.#lgx-portable' --override-input tictactoe path:../tictactoe` succeeds

**Verification:** Build succeeds. grep confirms no `delivery_module` references in C++ UI source.

---

## Task 4: Strip multiplayer from QML UI
**Status:** pending  
**Depends on:** Task 2

Remove all delivery module code from `Main.qml`. The QML UI should:
- Call `logos.callModule("tictactoe", "enableMultiplayer", [])` / `disableMultiplayer`
- Poll state via `logos.callModule("tictactoe", "mpStatus", [])` etc.
- Add a Timer to poll `mpStatus` periodically when multiplayer is enabled (for connection state updates)
- Remove `delivery_module` from `metadata.json` dependencies
- Remove `callDelivery()`, `broadcastMove()`, `broadcastNewGame()`, `handleDeliveryMessage()`, `Connections` for delivery events

**Acceptance criteria:**
- [ ] `Main.qml` has zero references to `delivery_module`, `callDelivery`, broadcast functions
- [ ] `metadata.json` dependencies only contain `["tictactoe"]`
- [ ] Multiplayer toggle calls core module methods
- [ ] Status display polls core module state
- [ ] Timer polls `mpStatus` every ~2 seconds when multiplayer enabled
- [ ] `cd tictactoe-ui-qml && nix build '.#lgx-portable' --override-input tictactoe path:../tictactoe` succeeds

**Verification:** Build succeeds. grep confirms no `delivery_module` references in QML UI.

---

## Task 5: Integration build and verification
**Status:** pending  
**Depends on:** Task 3, Task 4

Build all three LGX files. Verify CI passes.

**Acceptance criteria:**
- [ ] All three `nix build` commands succeed
- [ ] No compiler warnings related to multiplayer code
- [ ] git diff shows clean separation: delivery logic only in `tictactoe/`, UI concerns only in UI modules

**Verification:** All builds green. PR ready for review.
