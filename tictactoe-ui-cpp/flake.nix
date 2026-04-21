{
  description = "Tic-tac-toe UI plugin for Logos — widget frontend for tictactoe module";

  inputs = {
    logos-module-builder.url = "github:logos-co/logos-module-builder";
    nix-bundle-lgx.follows = "logos-module-builder/nix-bundle-lgx";
    tictactoe.url = "github:fryorcraken/logos-module-tictactoe?dir=tictactoe";
  };

  outputs = inputs@{ logos-module-builder, nix-bundle-lgx, tictactoe, ... }:
    let
      base = logos-module-builder.lib.mkLogosModule {
        src = ./.;
        configFile = ./metadata.json;
        flakeInputs = inputs;
        # Basecamp expects <name>.so but the build produces <name>_plugin.so.
        # Create a copy without the _plugin suffix (logos-co/logos-basecamp#136).
        postInstall = ''
          cp $out/lib/tictactoe_ui_cpp_plugin.so $out/lib/tictactoe_ui_cpp.so
        '';
      };
    in
    base // {
      # Expose .#lgx-dual manually until the builder ships it upstream
      # (tracked in logos-co/logos-module-builder#81).
      packages = builtins.mapAttrs (system: pkgs: pkgs // {
        lgx-dual = nix-bundle-lgx.bundlers.${system}.dual pkgs.lib;
      }) base.packages;
    };
}
