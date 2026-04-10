# logos-module-tictactoe

A Logos module wrapping a tic-tac-toe C library. Exposes game methods (`newGame`, `play`, `status`, `getCell`, `currentPlayer`) over the Logos plugin interface.

## Build

Requires [Nix](https://nixos.org/download.html) with flakes enabled.

```bash
nix build
```

Output: `result/lib/tictactoe_plugin.so` and `result/lib/libtictactoe.so`.

## Inspect

```bash
nix build 'github:logos-co/logos-module#lm' --out-link ./lm
./lm/bin/lm metadata result/lib/tictactoe_plugin.so
./lm/bin/lm methods  result/lib/tictactoe_plugin.so
```

## Generate LGX

```bash
# Local LGX (for dev builds of logoscore / logos-basecamp)
nix build '.#lgx'
# Output: result/logos-tictactoe-module-lib.lgx

# Portable LGX (for standalone / distributed builds)
nix build '.#lgx-portable'
```

## Install into logos-basecamp

### Dev build (nix-built basecamp → LogosBasecampDev)

```bash
nix build 'github:logos-co/logos-package-manager/tutorial-v1#cli' --out-link ./pm
BASECAMP_DIR="$HOME/.local/share/Logos/LogosBasecampDev"
./pm/bin/lgpm --modules-dir "$BASECAMP_DIR/modules" install --file result/*.lgx
```

### Portable / AppImage build (LogosBasecamp)

The nix-built `lgpm` is a dev tool and refuses to install portable lgx variants.
Extract and install manually instead:

```bash
nix build '.#lgx-portable' --out-link result-lgx-portable
BASECAMP_DIR="$HOME/.local/share/Logos/LogosBasecamp"
DEST="$BASECAMP_DIR/modules/tictactoe"
mkdir -p "$DEST"
tar xzf result-lgx-portable/*.lgx -C /tmp/ttt-lgx
cp /tmp/ttt-lgx/variants/linux-amd64/* "$DEST/"
cp /tmp/ttt-lgx/manifest.json "$DEST/"
echo "linux-amd64" > "$DEST/variant"
```

## Test with logoscore

```bash
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

## API Reference

### Methods

| Method | Args | Returns | Description |
|--------|------|---------|-------------|
| `newGame` | — | — | Reset the board. X moves first. |
| `play` | `row col` (0-2) | Error code (0=OK) | Place current player's mark |
| `status` | — | Status int | 0=ongoing, 1=X wins, 2=O wins, 3=draw |
| `getCell` | `row col` (0-2) | Cell int | 0=empty, 1=X, 2=O |
| `currentPlayer` | — | Cell int | 1=X, 2=O |
