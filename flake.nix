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
        f { 
          pkgs = import nixpkgs { inherit system; }; 
          inherit system;
        }
      );

    llvmPackagesFor = pkgs: pkgs.llvmPackages_21;
  in {
    schemas = flake-schemas.schemas;

    packages = forEachSupportedSystem ({pkgs, system, ...}: let
      llvm = llvmPackagesFor pkgs;
    in {
      default = llvm.stdenv.mkDerivation {
        pname = "compsci2";
        version = "0.1.0";
        src = ./.;

        nativeBuildInputs = [
          pkgs.meson
          pkgs.ninja 
        ];
      };
      picker = pkgs.writers.writeGuileBin "compsci2-picker" { } ''
        (use-modules (ice-9 ftw)
                     (ice-9 popen)
                     (ice-9 rdelim)
                     (ice-9 regex))

        (define bin-dir "${self.packages.${system}.default}/bin")
        (define fzf-bin "${pkgs.fzf}/bin/fzf")  
        (define token-rx (make-regexp "[0-9]+|[^0-9]+"))

        (define (tokenize s)
          (let loop ((start 0) (acc '()))
            (let ((m (regexp-exec token-rx s start)))
              (if m
                  (loop (match:end m) (cons (match:substring m) acc))
                  (reverse acc)))))
        
        (define (token<? a b)
          (let ((na (string->number a)) (nb (string->number b)))
            (if (and na nb)
                (< na nb)          ; both numeric runs: compare as integers
                (string<? a b))))  ; at least one is text: plain lexicographic
        
        (define (natural<? a b)
          (let loop ((ta (tokenize a)) (tb (tokenize b)))
            (cond
              ((null? ta) (not (null? tb)))
              ((null? tb) #f)
              ((string=? (car ta) (car tb)) (loop (cdr ta) (cdr tb)))
              (else (token<? (car ta) (car tb))))))
    
        (define (list-executables dir)
          (sort
            (filter (lambda (f) (not (member f (list "." ".."))))
                    (or (scandir dir) (list)))
            natural<?))
    
        (define (pick candidates)
          (let* ((tmp (string-append (or (getenv "TMPDIR") "/tmp")
                                      "/compsci2-picker-" (number->string (getpid))))
                 (out (open-output-file tmp)))
            (for-each (lambda (c) (display c out) (newline out)) candidates)
            (close-port out)
            (let* ((cmd (string-append fzf-bin " --prompt='Run> ' --height=40% --reverse < " tmp))
                   (in (open-input-pipe cmd))
                   (selection (read-line in)))
              (close-pipe in)
              (delete-file tmp)
              (if (eof-object? selection) #f selection))))
    
        (define (main args)
          (let ((candidates (list-executables bin-dir)))
            (when (null? candidates)
              (format (current-error-port) "No executables found in ~a~%" bin-dir)
              (exit 1))
            (let ((selected (pick candidates)))
              (unless selected
                (format (current-error-port) "No selection made.~%")
                (exit 1))
              (let ((full-path (string-append bin-dir "/" selected)))
                (apply execlp full-path (cons full-path args))))))
    
        (main (cdr (program-arguments)))
      '';
    });

    apps = forEachSupportedSystem ({ pkgs, system, ... }: {
      default = {
        type = "app";
        program = "${pkgs.lib.getExe self.packages.${system}.picker}";
      };
    });

    devShells = forEachSupportedSystem ({ pkgs, ... }: let
      llvm = llvmPackagesFor pkgs;
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
