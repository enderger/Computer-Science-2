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
    nix-vscode-extensions.url = "github:nix-community/nix-vscode-extensions";
    nur.url = "github:nix-community/NUR/main";
  };

  outputs = inputs@{ self, nixpkgs, flake-schemas, nix-vscode-extensions, nur }: let
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
    in lib.genAttrs supportedSystems (lib.flip lib.pipe [getSystemAttrs f]);

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
      baseTools = [ llvm.clang pkgs.meson pkgs.ninja pkgs.doctest pkgs.pkg-config ];

      mkProject = name: let
        def = projectDefinition system name;
      in llvm.stdenv.mkDerivation (extendDerivationAttrs
          { nativeBuildInputs = baseTools; }
          {
            pname = name;
            version = "0.1.0";
            src = ./projects + "/${name}";

            doCheck = true;
          } // def);

      projectPackages = nixpkgs.lib.genAttrs projectNames mkProject;

      codiumExtensions = nix-vscode-extensions.extensions.${system}.open-vsx;
      sharedCodiumExtensions = with codiumExtensions; [
        shaunlebron.vscode-parinfer
        llvm-vs-code-extensions.vscode-clangd
        mesonbuild.mesonbuild
        jnoortheen.nix-ide
        mkhl.mkhl-just
        mkhl.direnv
        evzen-wybitul.magic-racket
      ];

      racketTools = [
        pkgs.racket
        nur.legacyPackages.${system}.repos.DzmingLi.racket-langserver
      ];

      mkBaseCodium = name: base: pkgs.writeShellApplication {
        inherit name;
        runtimeInputs = racketTools;
        text = ''
            repo_root="$(git rev-parse --show-toplevel 2>/dev/null || true)"
            if [ -z "$repo_root" ] || [ ! -f "$repo_root/compsci2.code-workspace" ]; then
              echo "error: run this from within a checkout of the compsci2 repository" >&2
              exit 1
            fi
            exec "${pkgs.lib.getExe base}" "$repo_root/compsci2.code-workspace" "$@"
        '';
      };

      mkWrappedCodium = name: extraExtensions: mkBaseCodium name (
        if pkgs.stdenv.hostPlatform.isDarwin
        then pkgs.vscodium
        else pkgs.vscode-with-extensions.override {
            vscode = pkgs.vscodium;
            vscodeExtensions = sharedCodiumExtensions ++ extraExtensions;
          }
      );
    in projectPackages // {
        assignments = llvm.stdenv.mkDerivation {
          pname = "compsci2-assignments";
          version = "0.1.0";
        src = ./assignments;
        doCheck = true;
        mesonFlags = [ "-Dbuildtype=release" ];

        nativeBuildInputs = baseTools;
      };

      assignment-picker = pkgs.stdenv.mkDerivation {
        pname = "compsci2-picker";
        version = "0.1.0";
        dontUnpack = true;
        src = ./assignments/picker.scm;
        nativeBuildInputs = [ pkgs.makeWrapper ];
        installPhase = ''
          install -Dm755 "$src" "$out/bin/compsci2-picker"
          wrapProgram "$out/bin/compsci2-picker" \
            --prefix PATH : ${pkgs.lib.makeBinPath [ pkgs.guile pkgs.fzf ]} \
            --set PICKER_BIN_DIR "${self.packages.${system}.assignments}/bin"
        '';
        meta.mainProgram = "compsci2-picker";
      };

      init-assignment = pkgs.stdenv.mkDerivation {
        pname = "compsci2-init";
        version = "0.1.0";
        dontUnpack = true;
          src = ./assignments/init.rkt;
          nativeBuildInputs = [ pkgs.makeWrapper ];
          installPhase = ''
          install -Dm755 $src $out/bin/compsci2-init
          wrapProgram $out/bin/compsci2-init \
            --prefix PATH : ${pkgs.lib.makeBinPath [ pkgs.racket ]}
        '';
        meta.mainProgram = "compsci2-init";
      };

      codium = mkWrappedCodium "cs2-codium" [];
      codium-vim = mkWrappedCodium "cs2-codium-vim" [codiumExtensions.vscodevim.vim];

      default = self.packages.${system}.assignments;
    });

    apps = forEachSupportedSystem ({ pkgs, system, ... }: {
      default = {
        type = "app";
        program = pkgs.lib.getExe self.packages.${system}.assignment-picker;
      };
    });

    checks = forEachSupportedSystem ({ system, ... }: {
      assignments = self.packages.${system}.assignments.overrideAttrs (old: {
        mesonFlags = [
          "-Db_sanitize=address,undefined"
          "-Dcpp_args=-fno-sanitize-recover=undefined"
        ];
        doCheck = true;
      });
    });

    devShells = forEachSupportedSystem ({ pkgs, llvm, system, ... }: let
      basePackages = [
        llvm.clang
        llvm.clang-tools
        llvm.lldb

        self.packages.${system}.codium
        self.packages.${system}.codium-vim

        pkgs.doctest
        pkgs.just
        pkgs.meson
        pkgs.ninja
        pkgs.pkg-config
      ];

      mkProjectShell = name: let
        def = projectDefinition system name;
      in pkgs.mkShell (extendDerivationAttrs
          { nativeBuildInputs = basePackages; }
          (def // { inherit (llvm) stdenv; })
        );

      projectDevShells = nixpkgs.lib.genAttrs projectNames mkProjectShell;
    in projectDevShells // {
        default = pkgs.mkShell {
          stdenv = llvm.stdenv;
          packages = basePackages;
        };
      });
  };
}
