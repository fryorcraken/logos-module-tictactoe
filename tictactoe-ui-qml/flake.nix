{
  description = "Tic-tac-toe QML UI plugin for Logos — QML frontend for tictactoe module";

  inputs = {
    logos-module-builder.url = "github:logos-co/logos-module-builder/tutorial-v1";
    tictactoe.url = "github:fryorcraken/logos-module-tictactoe?dir=tictactoe";
  };

  outputs = inputs@{ logos-module-builder, ... }:
    logos-module-builder.lib.mkLogosQmlModule {
      src = ./.;
      configFile = ./metadata.json;
      flakeInputs = inputs;
    };
}
