# tictactoe core module tests

Self-contained build that does not require the Logos module toolchain.

```
cmake -B build -S .
cmake --build build
ctest --test-dir build --output-on-failure
```

Requires `cmake`, a C99 compiler, a C++17 compiler, `protoc`, and the protobuf
runtime. On NixOS:

```
nix-shell -p cmake protobuf gcc
```

## What's covered

- `test_libtictactoe.c` — C library behavior: turn alternation, win detection
  on every line (rows, cols, diagonals), draw, invalid position, occupied
  cell, game-over rejection, reset, bounds handling.
- `test_proto_roundtrip.cpp` — protobuf `GameMessage` serializes and
  deserializes for both `Move` and `new_game` payloads.

Plugin-level behavior (delivery module interaction, event emission) is not
covered here — those require a running Logos host and are exercised through
integration testing.
