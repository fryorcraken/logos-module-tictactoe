{
  description = "Tic-tac-toe UI plugin for Logos — widget frontend for tictactoe module";

  inputs = {
    logos-module-builder.url = "github:logos-co/logos-module-builder/tutorial-v1";
    tictactoe.url = "github:fryorcraken/logos-module-tictactoe?dir=tictactoe";
  };

  outputs = inputs@{ logos-module-builder, tictactoe, ... }:
    logos-module-builder.lib.mkLogosModule {
      src = ./.;
      configFile = ./metadata.json;
      flakeInputs = inputs;
      # Basecamp expects <name>.so but the build produces <name>_plugin.so.
      # Create a copy without the _plugin suffix (logos-co/logos-basecamp#136).
      postInstall = ''
        cp $out/lib/tictactoe_ui_plugin.so $out/lib/tictactoe_ui.so
      '';
    };
}
