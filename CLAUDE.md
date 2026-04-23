# CLAUDE.md

## Repository Overview

Tic-tac-toe mini app for the Logos platform. Three modules:

- `tictactoe/` — core module (C library + Logos plugin, Tutorial Part 1)
- `tictactoe-ui-cpp/` — C++ Qt widget UI (Tutorial Part 3, Option B)
- `tictactoe-ui-qml/` — QML UI (Tutorial Part 2)

Both UIs are alternative frontends providing identical gameplay.

## Build Commands

Each flake exposes two LGX bundlers:
- `.#lgx-portable` — `linux-amd64` variant, self-contained (bundles all shared libs). Consumed by the v0.1.1 AppImage.
- `.#lgx` — `linux-amd64-dev` variant, unbundled (resolves libs from the host Nix store). Consumed by a dev basecamp built from source (e.g. via `logos-scaffold-basecamp`).

Both are published to every GH release; CI builds them side-by-side. Portable files keep the plain name; dev files get the `-dev.lgx` suffix.

```bash
# Core module
cd tictactoe && nix build '.#lgx-portable' --out-link result-lgx-portable
cd tictactoe && nix build '.#lgx'          --out-link result-lgx

# C++ UI
cd tictactoe-ui-cpp && nix build '.#lgx-portable' --override-input tictactoe path:../tictactoe --out-link result-lgx-portable
cd tictactoe-ui-cpp && nix build '.#lgx'          --override-input tictactoe path:../tictactoe --out-link result-lgx

# QML UI
cd tictactoe-ui-qml && nix build '.#lgx-portable' --override-input tictactoe path:../tictactoe --out-link result-lgx-portable
cd tictactoe-ui-qml && nix build '.#lgx'          --override-input tictactoe path:../tictactoe --out-link result-lgx

# Standalone test (no basecamp)
cd tictactoe-ui-cpp && nix run . --override-input tictactoe path:../tictactoe
cd tictactoe-ui-qml && nix run . --override-input tictactoe path:../tictactoe
```

The core module's flake.nix has a `preConfigure` hook that compiles `lib/libtictactoe.c` → `libtictactoe.so` in-sandbox. Without it, logos-plugin-qt's `vendor_path` external-library handler stages only `lib*` files from the source `lib/` dir, so `.#lgx` ends up shipping `tictactoe_plugin.so` with undefined `tictactoe_*` symbols. See commit c486825. Upstream fix tracked in logos-co/logos-module-builder#83.

## Reset basecamp state

Basecamp (AppImage v0.1.1) does not hot-reload plugins. After rebuilding an LGX, you must kill basecamp, clean installed modules, and relaunch.

```bash
# Kill all basecamp processes (including orphaned logos_host children)
pkill -9 -f 'logos_host|logos-basecamp|\.logos_host\.elf'

# Remove installed tictactoe modules
BASECAMP_DIR="$HOME/.local/share/Logos/LogosBasecamp"
rm -rf "$BASECAMP_DIR/modules/tictactoe"
rm -rf "$BASECAMP_DIR/plugins/tictactoe_ui_cpp"
rm -rf "$BASECAMP_DIR/plugins/tictactoe_ui_cpp_qml"

# Relaunch
/path/to/logos-basecamp-x86_64-v0.1.1.AppImage &
```

**Important:** `pkill` returns exit code 1 if no processes match. Do not chain `rm` after `pkill` with `&&` or it will be skipped. Use `;` instead.

For a full reset: `rm -rf "$BASECAMP_DIR"` — basecamp re-preinstalls bundled modules on next launch.

## Known issues

- Basecamp expects `<name>.so` but CMake produces `<name>_plugin.so` — worked around with `postInstall` hook in flake.nix (logos-co/logos-basecamp#136)
- Logos Design System (`import Logos.Theme 1.0`) is NOT available in portable AppImage v0.1.1 — use hardcoded colors instead
- UI plugin icons not refreshed on reinstall without restart (logos-co/logos-basecamp#137)
- **QML events:** `LogosQmlBridge` supports `logos.onModuleEvent(moduleName, eventName)` + `Connections { function onModuleEventReceived() }` for subscribing to events from dependency modules. QML can receive events from modules it depends on (e.g., tictactoe's `eventResponse` signals), but NOT cross-module events directly. Requires `logos-module-builder` default branch (not `tutorial-v1`).

## Releases

When creating releases, include:
- A developer-facing note explaining what the tag provides as a working example
- Basecamp install instructions
- Table of included LGX files
