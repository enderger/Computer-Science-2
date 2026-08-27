# Copyright 2026 Danielle Hutzley
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#   http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
{
  description = "C++23 Computer Science 2 coursework monorepo";

  inputs = {
    nixpkgs.url = "https://flakehub.com/f/NixOS/nixpkgs/0.2605.1012336";
    flake-schemas.url = "https://flakehub.com/f/DeterminateSystems/flake-schemas/0.5.0";
  };

  outputs = inputs@{ self, nixpkgs, flake-schemas }: let
    supportedSystems = [
      "aarch64-darwin"
      "x86_64-darwin"

      "aarch64-linux"
      "x86_64-linux"
    ];

    getSystemAttrs = system: rec {
      pkgs = import nixpkgs { inherit system; }; 
      llvm = pkgs.llvmPackages_21;

      inherit system inputs;
    };

    forEachSupportedSystem = f: let
      lib = nixpkgs.lib;
    in 
      lib.genAttrs supportedSystems 
        (lib.flip lib.pipe [getSystemAttrs f]);

    projectNames = builtins.attrNames
      (nixpkgs.lib.filterAttrs (name: type: type == "directory")
        (builtins.readDir ./projects));
    projectDefinition = system: name: let
      defFile = ./projects + "/${name}/project.nix";
    in if builtins.pathExists defFile
       then import defFile (getSystemAttrs system)
       else { }; 

    extendDerivationAttrs = base: extra:
      extra // {
        nativeBuildInputs = nixpkgs.lib.unique (
          (base.nativeBuildInputs or [])
          ++ (extra.nativeBuildInputs or [])
        );

        buildInputs = nixpkgs.lib.unique (
          (base.buildInputs or [])
          ++ (extra.buildInputs or [])
        );
      };
  in {
    schemas = flake-schemas.schemas;

    packages = forEachSupportedSystem ({pkgs, llvm, system, ...}: let
      baseTools = [ llvm.clang pkgs.meson pkgs.ninja pkgs.doctest ];

      mkProject = name: let
        def = projectDefinition system name;
      in llvm.stdenv.mkDerivation (extendDerivationAttrs
          { nativeBuildInputs = baseTools; }
          { pname = name; version = "0.1.0"; src = ./projects + "/${name}"; } // def);

      projectPackages = nixpkgs.lib.genAttrs projectNames mkProject;
    in projectPackages // {
        assignments = llvm.stdenv.mkDerivation {
          pname = "compsci2-assignments";
          version = "0.1.0";
          src = ./assignments;

          nativeBuildInputs = baseTools;
        };

        assignment-picker = pkgs.stdenv.mkDerivation {
          pname = "compsci2-picker";
          version = "0.1.0";
          dontUnpack = true;
          src = ./assignments/picker.scm;
          nativeBuildInputs = [ pkgs.makeWrapper ];
          installPhase = ''
            install -Dm755 $src $out/bin/compsci2-picker
            wrapProgram $out/bin/compsci2-picker \
              --prefix PATH : ${pkgs.lib.makeBinPath [ pkgs.guile pkgs.fzf ]} \
              --set PICKER_BIN_DIR "${self.packages.${system}.assignments}/bin"
          '';
          meta.mainProgram = "compsci2-picker";
        };

        init-assignment = pkgs.stdenv.mkDerivation {
          pname = "compsci2-picker";
          version = "0.1.0";
          dontUnpack = true;
          src = ./assignments/init.scm;
          nativeBuildInputs = [ pkgs.makeWrapper ];
          installPhase = ''
            install -Dm755 $src $out/bin/compsci2-init
            wrapProgram $out/bin/compsci2-init \
              --prefix PATH : ${pkgs.lib.makeBinPath [ pkgs.guile ]}
          '';
          meta.mainProgram = "compsci2-init";
        };

        default = self.packages.${system}.assignment-picker;
    });

    apps = forEachSupportedSystem ({ pkgs, system, ... }: {
      default = {
        type = "app";
        program = pkgs.lib.getExe self.packages.${system}.assignment-picker;
      };
    });

    devShells = forEachSupportedSystem ({ pkgs, llvm, system, ... }: let
      basePackages = [ 
        llvm.clang
        llvm.clang-tools
        llvm.lldb

        pkgs.doctest
        pkgs.just
        pkgs.meson
        pkgs.ninja
        pkgs.pkg-config
      ];

      mkProjectShell = name: pkgs.mkShell {
        stdenv = llvm.stdenv;
        packages = let 
          def = projectDefinition system name;
        in pkgs.mkShell (extendDerivationAttrs 
            { nativeBuildInputs = basePackages; }
            def
        );
      };

      projectDevShells = nixpkgs.lib.genAttrs projectNames mkProjectShell;
    in projectDevShells // {
        default = pkgs.mkShell {
          stdenv = llvm.stdenv;
          packages = basePackages;
        };
    });
  };
}
