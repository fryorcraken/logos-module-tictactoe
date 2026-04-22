{
  description = "Tic-tac-toe Logos module - wraps libtictactoe C library";

  inputs = {
    logos-module-builder.url = "github:logos-co/logos-module-builder/tutorial-v1";

    # Pin: head of `tutorial-v1-compat` on logos-delivery-module.
    # Forked from tag 1.0.0 (what basecamp v0.1.1 ships — RPCs return plain
    # `bool`, pre-`LogosResult`) with minimal module-builder packaging and the
    # `externalLibInputs = { input; packages.default; }` shape fix on top.
    # Must stay pinned to a commit on that branch so the wire format of the
    # typed SDK headers we generate matches basecamp's bundled delivery_module.
    # See: https://github.com/logos-co/logos-delivery-module/pull/23
    delivery_module.url = "github:logos-co/logos-delivery-module/1fde1566291fe062b98255003b9166b0261c6081";
  };

  outputs = inputs@{ logos-module-builder, delivery_module, ... }:
    logos-module-builder.lib.mkLogosModule {
      src = ./.;
      configFile = ./metadata.json;
      flakeInputs = inputs;

      # Workaround: module-builder's `vendor_path` external-library handler
      # (logos-plugin-qt/lib/buildPlugin.nix) only copies `lib*` files from the
      # vendor dir — it does NOT run `build_command`, unlike the flake-input
      # path in mkExternalLib.nix. Without this, the plugin links with 7
      # undefined `tictactoe_*` symbols (lazy-bound) and crashes at first call
      # inside basecamp. The tutorial doc tells developers to compile manually
      # before `nix build`; we do it in-sandbox for reproducibility.
      # TODO: remove once module-builder honors build_command for vendor_path.
      preConfigure = ''
        if [ -f lib/libtictactoe.c ] && [ ! -f lib/libtictactoe.so ]; then
          echo "Compiling vendored libtictactoe.so..."
          (cd lib && gcc -shared -fPIC -o libtictactoe.so libtictactoe.c)
        fi
      '';
    };
}
