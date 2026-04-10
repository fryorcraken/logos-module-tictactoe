# logos-module-tictactoe

A tic-tac-toe Logos mini app. Contains a core module and two alternative UI frontends (pick either one — they provide the same gameplay):

- **tictactoe** (core) — wraps a C tic-tac-toe library, exposing game methods (`newGame`, `play`, `status`, `getCell`, `currentPlayer`) over the Logos plugin interface
- **tictactoe_ui** (C++ widget UI) — compiled C++ Qt widget frontend, calls the core module via the generated Logos SDK (Tutorial Part 3, Option B)
- **tictactoe_ui_qml** (QML UI) — declarative QML frontend, calls the core module via the `logos.callModule()` bridge (Tutorial Part 2). No compilation needed.

Both UIs are functionally identical — same 3x3 board, same X/O turns, same win/draw detection. The QML UI is significantly simpler: 1 file / ~170 LOC vs 7 files / ~310 LOC for the C++ widget UI, with no compilation needed. The QML UI also uses the Logos Design System for consistent theming with basecamp.

Built following the [Logos module tutorials](https://github.com/logos-co/logos-tutorial) (Part 1 + Part 2 + Part 3, Option B).

**Quick start:** grab the pre-built `.lgx` files from the [latest release](https://github.com/fryorcraken/logos-module-tictactoe/releases/latest) and install them in logos-basecamp — no compilation needed. See [Install into logos-basecamp](#install-into-logos-basecamp).

## Install into logos-basecamp

Tested with [logos-basecamp v0.1.1](https://github.com/nicholasgasior/logos-basecamp/releases/tag/v0.1.1) (AppImage).

Download the `.lgx` files from the [latest release](https://github.com/fryorcraken/logos-module-tictactoe/releases).

1. Open **logos-basecamp**
2. Go to **modules** → **install LGX Package** and select `logos-tictactoe-module-lib.lgx` (core module)
3. Go to **modules** → **install LGX Package** again and select **one** of (both provide the same gameplay):
   - `logos-tictactoe_ui-module-lib.lgx` — C++ widget UI
   - `logos-tictactoe_ui_qml-module-lib.lgx` — QML UI

The tic-tac-toe UI tab appears in the sidebar.

### Alternative: manual install

If the Package Manager UI is unavailable, extract and install manually:

```bash
BASECAMP_DIR="$HOME/.local/share/Logos/LogosBasecamp"  # Linux
# BASECAMP_DIR="$HOME/Library/Application Support/Logos/LogosBasecamp"  # macOS

# Core module
mkdir -p "$BASECAMP_DIR/modules/tictactoe" /tmp/ttt-lgx
tar xzf logos-tictactoe-module-lib.lgx -C /tmp/ttt-lgx
cp /tmp/ttt-lgx/variants/linux-amd64/* "$BASECAMP_DIR/modules/tictactoe/"
cp /tmp/ttt-lgx/manifest.json "$BASECAMP_DIR/modules/tictactoe/"
echo "linux-amd64" > "$BASECAMP_DIR/modules/tictactoe/variant"

# UI module
mkdir -p "$BASECAMP_DIR/plugins/tictactoe_ui" /tmp/ttt-ui-lgx
tar xzf logos-tictactoe_ui-module-lib.lgx -C /tmp/ttt-ui-lgx
cp /tmp/ttt-ui-lgx/variants/linux-amd64/* "$BASECAMP_DIR/plugins/tictactoe_ui/"
cp /tmp/ttt-ui-lgx/manifest.json "$BASECAMP_DIR/plugins/tictactoe_ui/"
echo "linux-amd64" > "$BASECAMP_DIR/plugins/tictactoe_ui/variant"
```

### Standalone UI (no basecamp needed)

Requires [Nix](https://nixos.org/download.html) with flakes enabled. Only the C++ widget UI supports standalone mode (the QML UI uses the Logos Design System which is only available inside basecamp).

```bash
cd tictactoe-ui
nix run . --override-input tictactoe path:../tictactoe
```

A standalone window opens with a 3x3 tic-tac-toe board. Full gameplay works (X/O turns, win/draw detection, new game).

## Build from source

Requires [Nix](https://nixos.org/download.html) with flakes enabled.

### Core module

```bash
cd tictactoe
nix build
```

Output: `tictactoe/result/lib/tictactoe_plugin.so` and `tictactoe/result/lib/libtictactoe.so`.

### UI module (C++ widget)

```bash
cd tictactoe-ui
nix build --override-input tictactoe path:../tictactoe
```

Output: `tictactoe-ui/result/lib/tictactoe_ui_plugin.so`.

### UI module (QML)

```bash
cd tictactoe-ui-qml
nix build --override-input tictactoe path:../tictactoe
```

Output: QML files staged in `tictactoe-ui-qml/result/`.

### Generate LGX

```bash
# Core module
cd tictactoe
nix build '.#lgx-portable' --out-link result-lgx-portable

# UI module (C++ widget)
cd tictactoe-ui
nix build '.#lgx-portable' --override-input tictactoe path:../tictactoe --out-link result-lgx-portable

# UI module (QML)
cd tictactoe-ui-qml
nix build '.#lgx-portable' --override-input tictactoe path:../tictactoe --out-link result-lgx-portable
```

Replace `.#lgx-portable` with `.#lgx` for dev (nix-built) basecamp builds.

### Inspect

```bash
cd tictactoe
nix build
nix build 'github:logos-co/logos-module#lm' --out-link ./lm
./lm/bin/lm metadata result/lib/tictactoe_plugin.so
./lm/bin/lm methods  result/lib/tictactoe_plugin.so
```

## Test with logoscore

```bash
cd tictactoe
nix build 'github:logos-co/logos-logoscore-cli/tutorial-v1' --out-link ./logos
nix build 'github:logos-co/logos-package-manager/tutorial-v1#cli' --out-link ./pm
nix build '.#lgx'
mkdir -p modules
./pm/bin/lgpm --modules-dir ./modules install --file result/*.lgx

./logos/bin/logoscore -D -m ./modules &
./logos/bin/logoscore load-module tictactoe
./logos/bin/logoscore call tictactoe newGame
./logos/bin/logoscore call tictactoe play 0 0    # X plays top-left
./logos/bin/logoscore call tictactoe play 1 1    # O plays center
./logos/bin/logoscore call tictactoe status       # 0=ongoing, 1=X wins, 2=O wins, 3=draw
./logos/bin/logoscore stop
```

## Known Limitations

- **Dark theme contrast:** The UI widget uses explicit light-on-dark colors but
  may not perfectly match basecamp's theme. X marks are blue (#4a9eff), O marks
  are red (#ff6b6b).

- **`.so` naming workaround:** Basecamp expects `<name>.so` but the
  `logos_module()` CMake macro produces `<name>_plugin.so`. The UI `flake.nix`
  includes a `postInstall` hook that creates a copy with the correct name, so
  the lgx ships both `tictactoe_ui.so` and `tictactoe_ui_plugin.so`. The
  duplicate will be removed once the upstream fix lands.
  Tracked in logos-co/logos-basecamp#136.

- **"No methods available" in logos-basecamp:** The core module loads
  (visible in the module list), but basecamp's "View Methods" UI shows no
  methods. The methods are confirmed present via `lm` and `logoscore call`.
  Tracked in logos-co/logos-basecamp#135.

## API Reference

### Methods

| Method | Args | Returns | Description |
|--------|------|---------|-------------|
| `newGame` | -- | -- | Reset the board. X moves first. |
| `play` | `row col` (0-2) | Error code (0=OK) | Place current player's mark |
| `status` | -- | Status int | 0=ongoing, 1=X wins, 2=O wins, 3=draw |
| `getCell` | `row col` (0-2) | Cell int | 0=empty, 1=X, 2=O |
| `currentPlayer` | -- | Cell int | 1=X, 2=O |
