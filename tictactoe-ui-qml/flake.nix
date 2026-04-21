{
  description = "Tic-tac-toe QML UI plugin for Logos — QML frontend for tictactoe module";

  inputs = {
    logos-module-builder.url = "github:logos-co/logos-module-builder";
    nix-bundle-lgx.follows = "logos-module-builder/nix-bundle-lgx";
    tictactoe.url = "github:fryorcraken/logos-module-tictactoe?dir=tictactoe";
  };

  outputs = inputs@{ logos-module-builder, nix-bundle-lgx, ... }:
    let
      base = logos-module-builder.lib.mkLogosQmlModule {
        src = ./.;
        configFile = ./metadata.json;
        flakeInputs = inputs;
      };
    in
    base // {
      # Expose .#lgx-dual manually until the builder ships it upstream
      # (tracked in logos-co/logos-module-builder#81).
      packages = builtins.mapAttrs (system: pkgs: pkgs // {
        lgx-dual = nix-bundle-lgx.bundlers.${system}.dual pkgs.default;
      }) base.packages;
    };
}
