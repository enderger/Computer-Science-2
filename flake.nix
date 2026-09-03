# Copyright 2026 Danielle Hutzley
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
    git-hooks = {
      url = "https://flakehub.com/f/cachix/git-hooks.nix/0.1.1233";
      inputs.nixpkgs.follows = "nixpkgs";
    };
    hercules-ci-effects.url = "github:hercules-ci/hercules-ci-effects";
    nix-vscode-extensions.url = "github:nix-community/nix-vscode-extensions";
    nur.url = "github:nix-community/NUR/main";
  };

  outputs =
    inputs@{
      self,
      nixpkgs,
      flake-schemas,
      git-hooks,
      hercules-ci-effects,
      nix-vscode-extensions,
      nur,
    }:
    let
      supportedSystems = [
        "aarch64-darwin"
        "x86_64-darwin"

        "aarch64-linux"
        "x86_64-linux"
      ];

      getSystemAttrs = system: rec {
        pkgs = import nixpkgs { inherit system; };
        llvm = pkgs.llvmPackages_22;

        dev-tools =
          let
            racket-tools = [
              pkgs.racket
              nur.legacyPackages.${system}.repos.DzmingLi.racket-langserver
            ];
            cpp-tools = [
              llvm.clang
              llvm.clang-tools
              llvm.lldb
              pkgs.meson
              pkgs.mesonlsp
              pkgs.doctest
              pkgs.just
              pkgs.pkg-config
              pkgs.ninja
            ];
            nix-tools = [
              pkgs.nixd
            ];
          in
          racket-tools ++ cpp-tools ++ nix-tools ++ [ pkgs.git ];

        inherit system inputs;
      };

      forEachSupportedSystem =
        f:
        let
          inherit (nixpkgs) lib;
        in
        lib.genAttrs supportedSystems (
          lib.flip lib.pipe [
            getSystemAttrs
            f
          ]
        );

      projectNames = builtins.attrNames (
        nixpkgs.lib.filterAttrs (_: type: type == "directory") (builtins.readDir ./projects)
      );
      projectDefinition =
        system: name:
        let
          defFile = ./projects + "/${name}/project.nix";
        in
        if builtins.pathExists defFile then import defFile (getSystemAttrs system) else { };

      extendDerivationAttrs =
        base: extra:
        extra
        // {
          nativeBuildInputs = nixpkgs.lib.unique (
            (base.nativeBuildInputs or [ ]) ++ (extra.nativeBuildInputs or [ ])
          );

          buildInputs = nixpkgs.lib.unique ((base.buildInputs or [ ]) ++ (extra.buildInputs or [ ]));
        };
    in
    {
      inherit (flake-schemas) schemas;

      packages = forEachSupportedSystem (
        {
          pkgs,
          llvm,
          system,
          dev-tools,
          ...
        }:
        let
          baseTools = [
            llvm.clang
            pkgs.meson
            pkgs.ninja
            pkgs.doctest
            pkgs.pkg-config
          ];

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

          mkBaseSelfSignedCodium =
            name: base:
            pkgs.stdenv.mkDerivation {
              pname = name + "-self-signed";
              version = "0.1.0";
              dontUnpack = true;
              nativeBuildInputs = [
                pkgs.makeWrapper
                pkgs.darwin.sigtool
              ];
              buildPhase = ''
                mkdir -p $out
                cp -r ${base}/* $out/
                chmod -R u+w $out
                substituteInPlace "$out/Applications/VSCodium.app/Contents/Resources/app/product.json" \
                  --replace-fail \
                    '"updateUrl": "https://raw.githubusercontent.com/VSCodium/versions/refs/heads/master"' \
                    '"updateUrl": ""'
                find "$out/Applications/VSCodium.app" -type f \
                  -exec sigtool -f {} check-requires-signature \; \
                  -exec codesign -s - -f {} \;

                rm -f "$out/bin/codium"
                makeWrapper "$out/Applications/VSCodium.app/Contents/MacOS/VSCodium" "$out/bin/codium"
              '';
              dontInstall = true;
            };

          mkBaseCodium =
            name: base:
            pkgs.writeShellApplication {
              inherit name;
              runtimeInputs = dev-tools;
              text = ''
                repo_root="$(git rev-parse --show-toplevel 2>/dev/null || true)"
                if [ -z "$repo_root" ] || [ ! -f "$repo_root/compsci2.code-workspace" ]; then
                  echo "error: run this from within a checkout of the compsci2 repository" >&2
                  exit 1
                fi
                exec "${
                  if pkgs.stdenv.hostPlatform.isDarwin then mkBaseSelfSignedCodium name base else base
                }/bin/codium" "$repo_root/compsci2.code-workspace" "$@"
              '';
            };

          mkWrappedCodium =
            name: extraExtensions:
            mkBaseCodium name (
              if pkgs.stdenv.hostPlatform.isDarwin then
                pkgs.vscodium
              else
                pkgs.vscode-with-extensions.override {
                  vscode = pkgs.vscodium;
                  vscodeExtensions = sharedCodiumExtensions ++ extraExtensions;
                }
            );

          substituteDoxygen =
            {
              name,
              input ? ./templates/Doxyfile.template,
              output ? "share/doc/${name}",
              ...
            }:
            pkgs.writeShellApplication {
              name = "substitute-doxygen";
              runtimeInputs = [
                pkgs.gnused
                pkgs.doxygen
              ];
              text = ''
                doc="$1"
                srcPath="$2"
                mkdir -p "$doc/${output}"
                cd "$srcPath"
                sed -e "s|@@NAME@@|${name}|" \
                    -e "s|@@OUTPUT@@|$doc/${output}|" \
                    ${input} | doxygen -
              '';
            };

          mkProject =
            name:
            let
              def = projectDefinition system name;
            in
            llvm.stdenv.mkDerivation (
              extendDerivationAttrs
                { nativeBuildInputs = baseTools ++ [ (substituteDoxygen { inherit name; }) ]; }
                (
                  {
                    pname = name;
                    version = "0.1.0";
                    outputs = [
                      "out"
                      "doc"
                    ];
                    src = ./projects + "/${name}";

                    doCheck = true;
                    postBuild = ''
                      substitute-doxygen "$doc" "$src"
                    '';
                  }
                  // def
                )
            );
        in
        projectPackages
        // {
          assignments = llvm.stdenv.mkDerivation {
            pname = "compsci2-assignments";
            version = "0.1.0";
            outputs = [
              "out"
              "doc"
            ];
            src = ./assignments;
            doCheck = true;
            mesonFlags = [ "--buildtype=release" ];

            nativeBuildInputs = baseTools ++ [ (substituteDoxygen { name = "assignments"; }) ];
            postBuild = ''
              substitute-doxygen "$doc" "$src"
            '';
          };

          assignment-picker = pkgs.stdenv.mkDerivation {
            pname = "compsci2-picker";
            version = "0.1.0";
            dontUnpack = true;
            src = ./assignments/picker.rkt;
            nativeBuildInputs = [ pkgs.makeWrapper ];
            installPhase = ''
              install -Dm755 "$src" "$out/bin/compsci2-picker"
              wrapProgram "$out/bin/compsci2-picker" \
                --prefix PATH : ${
                  pkgs.lib.makeBinPath [
                    pkgs.racket
                    pkgs.fzf
                  ]
                } \
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
                --prefix PATH : ${
                  pkgs.lib.makeBinPath [
                    pkgs.racket
                    pkgs.meson
                    llvm.clang-tools
                  ]
                }
            '';
            meta.mainProgram = "compsci2-init";
          };

          lint = pkgs.writeShellApplication {
            name = "compsci2-lint";
            runtimeInputs = [
              self.formatter.${system}

              pkgs.deadnix
              pkgs.git
              pkgs.racket
              pkgs.statix
            ];

            text = ''
              status=0
              cd "$(git rev-parse --show-toplevel)"

              # General
              compsci2-fmt --check || status=1

              # Nix
              git ls-files -z "*.nix" | xargs -0 -r deadnix -f || status=1
              statix check . || status=1

              # Racket
              git ls-files -z "*.rkt" | xargs -0 -r raco make || status=1

              exit "$status"
            '';
          };

          codium = mkWrappedCodium "cs2-codium" [ ];
          codium-vim = mkWrappedCodium "cs2-codium-vim" [ codiumExtensions.vscodevim.vim ];

          default = self.packages.${system}.assignments;
        }
      );

      apps = forEachSupportedSystem (
        { pkgs, system, ... }: {
          default = {
            type = "app";
            program = pkgs.lib.getExe self.packages.${system}.assignment-picker;
          };
        }
      );

      checks = forEachSupportedSystem (
        { pkgs, system, ... }: {
          assignments = self.packages.${system}.assignments.overrideAttrs (_: {
            mesonFlags = [
              "-Db_sanitize=address,undefined"
              "-Dcpp_args=-fno-sanitize-recover=undefined"
              "-Db_lundef=false"
            ];
            doCheck = true;
          });

          scripts = pkgs.runCommand "compsci2-script-tests" { nativeBuildInputs = [ pkgs.racket ]; } ''
            raco test ${./assignments}/init.rkt
            raco test ${./assignments}/picker.rkt
            touch $out
          '';
        }
      );

      formatter = forEachSupportedSystem (
        { pkgs, llvm, ... }:
        pkgs.writeShellApplication {
          name = "compsci2-fmt";
          runtimeInputs = [
            pkgs.git
            llvm.clang-tools
            pkgs.meson
            pkgs.just
            pkgs.nixfmt
          ];
          text = ''
            status=0
            if [ "''${1:-}" = "--check" ]; then
              clang_format_args=(--dry-run -Werror)
              meson_args=(--check-only)
              just_args=(--check)
              nixfmt_args=(--check)
            else
              clang_format_args=(-i)
              meson_args=(-i)
              just_args=()
              nixfmt_args=()
            fi

            cd "$(git rev-parse --show-toplevel)"
            EXCLUDE_GLOB=':(exclude,glob)**/template*/**'
            git ls-files -z '*.cpp' '*.hpp' '*.cppm' '*.c' "$EXCLUDE_GLOB" | xargs -0 -r clang-format "''${clang_format_args[@]}" || status=1
            git ls-files -z '*meson.build' "$EXCLUDE_GLOB" | xargs -0 -r -n1 meson format "''${meson_args[@]}" || status=1
            git ls-files -z '*justfile' "$EXCLUDE_GLOB" | xargs -0 -r -n1 just --fmt "''${just_args[@]}" -f || status=1
            git ls-files -z '*.nix' "$EXCLUDE_GLOB" | xargs -0 -r nixfmt "''${nixfmt_args[@]}" || status=1
            exit "$status"
          '';
        }
      );

      herculesCI = hercules-ci-effects.lib.mkHerculesCI { inherit inputs; } {
        herculesCI = {
          ciSystems = [
            "x86_64-linux"
            "aarch64-darwin"
          ];
        };
      };

      devShells = forEachSupportedSystem (
        {
          pkgs,
          llvm,
          system,
          dev-tools,
          ...
        }:
        let
          basePackages = dev-tools ++ [
            self.formatter.${system}
            self.packages.${system}.lint
          ];

          mkProjectShell =
            name:
            let
              def = projectDefinition system name;
            in
            pkgs.mkShell.override { inherit (llvm) stdenv; } (
              extendDerivationAttrs { nativeBuildInputs = basePackages; } def
            );

          projectDevShells = nixpkgs.lib.genAttrs projectNames mkProjectShell;
        in
        projectDevShells
        // {
          default =
            pkgs.mkShell.override
              {
                inherit (llvm) stdenv;
              }
              {
                nativeBuildInputs = basePackages;
              };
        }
      );
    };
}
