{
  description = "C++23 coursework dev shell";

  inputs = {
    nixpkgs.url = "https://flakehub.com/f/NixOS/nixpkgs/0.2605.1012336";
    flake-schemas.url = "https://flakehub.com/f/DeterminateSystems/flake-schemas/0.5.0";
  };

  outputs = { self, nixpkgs, flake-schemas }: let
    supportedSystems = [
      "aarch64-darwin"
      "x86_64-darwin"

      "aarch64-linux"
      "x86_64-linux"
    ];

    forEachSupportedSystem = f:
      nixpkgs.lib.genAttrs supportedSystems (system:
        f { pkgs = import nixpkgs { inherit system; }; }
      );
  in {
    schemas = flake-schemas.schemas;
    devShells = forEachSupportedSystem ({ pkgs }: let
      llvm = pkgs.llvmPackages_21;
    in {
      default = pkgs.mkShell {
        stdenv = llvm.stdenv;
        packages = [
          llvm.clang 
          llvm.clang-tools
          llvm.lldb
          pkgs.just
          pkgs.ninja
          pkgs.meson
        ];
      };
    });
  };
}
