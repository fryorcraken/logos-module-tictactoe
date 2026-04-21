{
  description = "Tic-tac-toe Logos module - wraps libtictactoe C library";

  inputs = {
    logos-module-builder.url = "github:logos-co/logos-module-builder";
    nix-bundle-lgx.follows = "logos-module-builder/nix-bundle-lgx";
    delivery_module.url = "github:logos-co/logos-delivery-module";
  };

  outputs = inputs@{ logos-module-builder, nix-bundle-lgx, delivery_module, ... }:
    let
      base = logos-module-builder.lib.mkLogosModule {
        src = ./.;
        configFile = ./metadata.json;
        flakeInputs = inputs;
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
