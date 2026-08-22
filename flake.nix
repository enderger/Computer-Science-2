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
