# Refactor Plan: Move Multiplayer Logic to Core Module

## Problem

The delivery module (multiplayer) integration currently lives in the C++ UI (`tictactoe-ui-cpp/`). This is poor architecture:

1. **UI modules contain business logic** — the C++ UI directly calls `delivery_module` via `LogosAPI::getClient()`, manages connection state, serializes protobuf messages, and handles incoming events.
2. **QML UI can't do multiplayer** — `LogosQmlBridge` has no event subscription (`callModule()` only). QML can't receive async events from `delivery_module`. Moving logic to the core module and emitting `eventResponse` is the only path.
3. **Code duplication** — both UIs would need to independently implement the same delivery module interaction pattern.

## Target Architecture

```
┌─────────────────┐     ┌──────────────────┐
│ tictactoe-ui-cpp│     │ tictactoe-ui-qml │
│ (C++ Qt Widget) │     │   (pure QML)     │
│                 │     │                  │
│ Calls:          │     │ Calls:           │
│  tictactoe.     │     │  logos.callModule │
│   enableMP()    │     │   ("tictactoe",  │
│   disableMP()   │     │    "enableMP",[])│
│   play()        │     │                  │
│   newGame()     │     │                  │
│                 │     │                  │
│ Subscribes to:  │     │ Polls:           │
│  eventResponse  │     │  mpStatus()      │
│  (via LogosModules │  │  mpConnected()   │
│   .on() pattern)│     │  (after each     │
│                 │     │   callModule)    │
└────────┬────────┘     └────────┬─────────┘
         │                       │
         └───────────┬───────────┘
                     │
         ┌───────────▼───────────┐
         │     tictactoe/        │
         │   (core module)       │
         │                       │
         │ Game logic (C lib)    │
         │ + Delivery module     │
         │   integration (C++)   │
         │                       │
         │ Owns:                 │
         │  - delivery_module    │
         │    client lifecycle   │
         │  - protobuf ser/de   │
         │  - connection state   │
         │  - message routing    │
         │                       │
         │ Emits eventResponse:  │
         │  "mpStatusChanged"    │
         │  "remoteMove"         │
         │  "remoteNewGame"      │
         └───────────┬───────────┘
                     │
         ┌───────────▼───────────┐
         │   delivery_module     │
         │   (pre-installed)     │
         └───────────────────────┘
```

## Key Design Decisions

### 1. QML event reception
QML cannot receive `eventResponse` signals from other modules. The QML UI will **poll** multiplayer state by calling query methods (`mpStatus`, `mpConnected`, etc.) after every `callModule` interaction, and on a timer for connection state changes. This is the pragmatic solution given the platform limitation.

### 2. Protobuf stays in core module
The `.proto` file and protobuf compilation move from `tictactoe-ui-cpp/` to `tictactoe/`. The core module serializes/deserializes messages. UIs never touch wire format.

### 3. New core module methods
- `enableMultiplayer()` — creates delivery node, registers event handlers, starts, subscribes
- `disableMultiplayer()` — unsubscribes, stops
- `mpStatus()` → int (0=off, 1=connecting, 2=connected, 3=error)
- `mpConnected()` → bool
- `mpMessagesSent()` → int
- `mpMessagesReceived()` → int
- `mpError()` → QString (empty if no error)

### 4. Core module events (via eventResponse)
- `"remoteMove"` with args `[row, col, status]` — a remote player made a move
- `"remoteNewGame"` with args `[]` — remote player started new game
- `"mpStatusChanged"` with args `[statusInt]` — connection state changed

### 5. play() and newGame() auto-broadcast
When multiplayer is enabled, `play()` and `newGame()` in the core module automatically broadcast to the delivery module. UIs don't need to call separate broadcast methods.

### 6. Content topic
Single content topic `/tictactoe/1/moves/proto` used by all instances. Core module owns this.

## Dependency Graph

```
Task 1 (proto + cmake)  ──→  Task 2 (core module logic)  ──→  Task 3 (strip C++ UI)
                                                           ──→  Task 4 (strip QML UI)
                                                                      │
                                                           Task 5 (build + verify)
```

## Risk Assessment

- **metadata.json dependency on delivery_module**: Adding `"delivery_module"` to `tictactoe/metadata.json` dependencies will trigger the build system to generate typed wrappers. This requires `delivery_module` as a flake input OR we use raw `LogosAPI::getClient()` pattern (which we know works from the C++ UI). We'll use the raw client pattern to avoid flake input complexity.
- **QML polling latency**: Connection state changes won't be instant in QML. A 1-2 second timer is acceptable for a game.
- **Protobuf in core module build**: The core module CMakeLists.txt currently only builds C code. Adding protobuf requires `find_package(Protobuf)` and linking. The nix metadata must include protobuf in build/runtime packages.
