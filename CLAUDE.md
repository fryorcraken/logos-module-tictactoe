# CLAUDE.md

## Repository Overview

Tic-tac-toe mini app for the Logos platform. Three modules:

- `tictactoe/` — core module (C library + Logos plugin, Tutorial Part 1)
- `tictactoe-ui-cpp/` — C++ Qt widget UI (Tutorial Part 3, Option B)
- `tictactoe-ui-qml/` — QML UI (Tutorial Part 2)

Both UIs are alternative frontends providing identical gameplay.

## Build Commands

```bash
# Core module
cd tictactoe && nix build '.#lgx-portable' --out-link result-lgx-portable

# C++ UI
cd tictactoe-ui-cpp && nix build '.#lgx-portable' --override-input tictactoe path:../tictactoe --out-link result-lgx-portable

# QML UI
cd tictactoe-ui-qml && nix build '.#lgx-portable' --override-input tictactoe path:../tictactoe --out-link result-lgx-portable

# Standalone test (no basecamp)
cd tictactoe-ui-cpp && nix run . --override-input tictactoe path:../tictactoe
cd tictactoe-ui-qml && nix run . --override-input tictactoe path:../tictactoe
```

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

## Releases

When creating releases, include:
- A developer-facing note explaining what the tag provides as a working example
- Basecamp install instructions
- Table of included LGX files
