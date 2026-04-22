{
  description = "Tic-tac-toe UI plugin for Logos — widget frontend for tictactoe module";

  inputs = {
    logos-module-builder.url = "github:logos-co/logos-module-builder/tutorial-v1";
    tictactoe.url = "github:fryorcraken/logos-module-tictactoe?dir=tictactoe";

    # Force `tictactoe`'s transitive `logos-module-builder` to follow our root
    # pin. Without this, tictactoe drags in its own (and delivery_module's own)
    # `logos-module-builder` as extra flake.lock entries, and the transitive
    # rev can silently win when building with `--override-input tictactoe
    # path:...`. See tictactoe/flake.nix for the full story.
    #
    # Second line covers the nested case: tictactoe's `delivery_module` has its
    # own `logos-module-builder` input which isn't reached by the first follows.
    # We force it here rather than relying on the remote tictactoe carrying the
    # same follows, so this flake builds cleanly against both the local checkout
    # and the published tag.
    # TODO: remove once module-builder scaffolds this automatically.
    # Tracking: https://github.com/logos-co/logos-module-builder/issues/83
    tictactoe.inputs.logos-module-builder.follows = "logos-module-builder";
    tictactoe.inputs.delivery_module.inputs.logos-module-builder.follows = "logos-module-builder";
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
