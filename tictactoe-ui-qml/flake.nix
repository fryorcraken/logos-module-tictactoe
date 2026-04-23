{
  description = "Tic-tac-toe QML UI plugin for Logos — QML frontend for tictactoe module";

  inputs = {
    logos-module-builder.url = "github:logos-co/logos-module-builder/tutorial-v1";
    tictactoe.url = "github:fryorcraken/logos-module-tictactoe?dir=tictactoe";

    # Declared so module-builder's logos_sdk generator can resolve
    # delivery_module_api.h when this UI plugin lists delivery_module in its
    # metadata.json dependencies. Basecamp's UI-side dep walker is
    # non-recursive, so if the UI plugin does not list delivery_module
    # directly, basecamp will not spawn it on UI open.
    delivery_module.url = "github:logos-co/logos-delivery-module/1fde1566291fe062b98255003b9166b0261c6081";
    delivery_module.inputs.logos-module-builder.follows = "logos-module-builder";

    # Pin `tictactoe`'s `logos-module-builder` to ours. Without this the
    # transitive module-builder rev pulled in by tictactoe can silently win
    # when building with `--override-input tictactoe path:...` and break
    # wire-format compatibility with basecamp's bundled delivery_module.
    # tictactoe/flake.nix handles its own `delivery_module →
    # logos-module-builder` follows internally, so one line is enough here.
    # TODO: remove once module-builder scaffolds this automatically.
    # Tracking: https://github.com/logos-co/logos-module-builder/issues/83
    tictactoe.inputs.logos-module-builder.follows = "logos-module-builder";
    tictactoe.inputs.delivery_module.follows = "delivery_module";
  };

  outputs = inputs@{ logos-module-builder, ... }:
    logos-module-builder.lib.mkLogosQmlModule {
      src = ./.;
      configFile = ./metadata.json;
      flakeInputs = inputs;
    };
}
