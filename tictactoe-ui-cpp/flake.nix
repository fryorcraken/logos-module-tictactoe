{
  description = "Tic-tac-toe UI plugin for Logos — widget frontend for tictactoe module";

  inputs = {
    logos-module-builder.url = "github:logos-co/logos-module-builder/tutorial-v1";
    tictactoe.url = "github:fryorcraken/logos-module-tictactoe?dir=tictactoe";

    # Pin `tictactoe`'s `logos-module-builder` to ours. Without this the
    # transitive module-builder rev pulled in by tictactoe can silently win
    # when building with `--override-input tictactoe path:...` and break
    # wire-format compatibility with basecamp's bundled delivery_module.
    # tictactoe/flake.nix handles its own `delivery_module →
    # logos-module-builder` follows internally, so one line is enough here.
    # TODO: remove once module-builder scaffolds this automatically.
    # Tracking: https://github.com/logos-co/logos-module-builder/issues/83
    tictactoe.inputs.logos-module-builder.follows = "logos-module-builder";
  };

  outputs = inputs@{ logos-module-builder, tictactoe, ... }:
    logos-module-builder.lib.mkLogosModule {
      src = ./.;
      configFile = ./metadata.json;
      flakeInputs = inputs;
      # Basecamp expects <name>.so but the build produces <name>_plugin.so.
      # Create a copy without the _plugin suffix (logos-co/logos-basecamp#136).
      postInstall = ''
        cp $out/lib/tictactoe_ui_cpp_plugin.so $out/lib/tictactoe_ui_cpp.so
      '';
    };
}
