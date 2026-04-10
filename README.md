# logos-module-tictactoe

A Logos module wrapping a tic-tac-toe C library. Exposes game methods (`newGame`, `play`, `status`, `getCell`, `currentPlayer`) over the Logos plugin interface.

Tested with [logos-basecamp v0.1.1](https://github.com/nicholasgasior/logos-basecamp/releases/tag/v0.1.1) (AppImage).

## Install into logos-basecamp

Download the `.lgx` file from the [latest release](https://github.com/fryorcraken/logos-module-tictactoe/releases).

1. Open **logos-basecamp**
2. Go to **Package Manager**
3. Click **Install from file**
4. Select the downloaded `.lgx` file

The module appears in the module list.

### Alternative: manual install

If the Package Manager UI is unavailable, extract and install the lgx manually:

```bash
BASECAMP_DIR="$HOME/.local/share/Logos/LogosBasecamp"  # Linux
# BASECAMP_DIR="$HOME/Library/Application Support/Logos/LogosBasecamp"  # macOS
DEST="$BASECAMP_DIR/modules/tictactoe"
mkdir -p "$DEST" /tmp/ttt-lgx
tar xzf logos-tictactoe-module-lib.lgx -C /tmp/ttt-lgx
cp /tmp/ttt-lgx/variants/linux-amd64/* "$DEST/"
cp /tmp/ttt-lgx/manifest.json "$DEST/"
echo "linux-amd64" > "$DEST/variant"
```

## Build from source

Requires [Nix](https://nixos.org/download.html) with flakes enabled.

```bash
nix build
```

Output: `result/lib/tictactoe_plugin.so` and `result/lib/libtictactoe.so`.

### Generate LGX

```bash
# Local LGX (for nix-built logoscore / logos-basecamp dev builds)
nix build '.#lgx'

# Portable LGX (for AppImage / standalone builds)
nix build '.#lgx-portable'
```

### Inspect

```bash
nix build 'github:logos-co/logos-module#lm' --out-link ./lm
./lm/bin/lm metadata result/lib/tictactoe_plugin.so
./lm/bin/lm methods  result/lib/tictactoe_plugin.so
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

## Known Limitations

- **"No methods available" in logos-basecamp:** The module loads successfully
  (visible in the module list), but basecamp's "View Methods" UI shows no
  methods. The methods are confirmed present via the `lm` inspector and
  `logoscore call`:

  ```bash
  # lm shows all 5 game methods + initLogos + eventResponse:
  ./lm/bin/lm methods result/lib/tictactoe_plugin.so

  # logoscore can invoke them:
  ./logos/bin/logoscore call tictactoe play 0 0   # returns {"result":0}
  ./logos/bin/logoscore call tictactoe status      # returns {"result":1}
  ```

  This appears to be a basecamp UI issue with core (non-UI) modules.

## API Reference

### Methods

| Method | Args | Returns | Description |
|--------|------|---------|-------------|
| `newGame` | -- | -- | Reset the board. X moves first. |
| `play` | `row col` (0-2) | Error code (0=OK) | Place current player's mark |
| `status` | -- | Status int | 0=ongoing, 1=X wins, 2=O wins, 3=draw |
| `getCell` | `row col` (0-2) | Cell int | 0=empty, 1=X, 2=O |
| `currentPlayer` | -- | Cell int | 1=X, 2=O |
