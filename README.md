# My Computer Science 2 Monorepo
[![License](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)

By Hutzdog (Danielle Hutzley)

## Setup
This repository is managed with the Nix package manager. Nix provides fully reproducible builds/packages and development environments. This means that you can install my code from scratch with everything as it is on my system (package wise, so LLVM 22, etc).

### Branches
There are 3 main branches of Nix:
- Mainline Nix : The original Nix, it's good all around but has issues that the forks fix
- Lix : A community-driven fork of mainline. It has frozen Flake development and hasn't worked much on its replacement.
- Determinate Nix (what we're going to use) : An enterprise-grade version of Nix, featuring such additions as multithreaded builds and extensible Flake outputs.

### Installation
#### Windows
1. Set up the Windows Subsystem for Linux (any distribution should work, but I recommend AGAINST NixOS or anything non-systemd).
2. Run the Linux instructions

#### macOS
1. Run the installer from <https://install.determinate.systems/determinate-pkg/stable/Universal>
2. Run `nix profile add nixpkgs#direnv` to install direnv, which automates the development shell process
3. Follow the instructions [here](https://direnv.net/docs/hook.html) to hook Direnv into your shell

#### Linux
1. Run the following command, reviewing the output in less: `curl -fsSL https://install.determinate.systems/nix | tee nix-installer.sh | less`
2. Run `./nix-installer.sh` AFTER REVIEWING THE OUTPUT OF THE PREVIOUS COMMAND.
3. Run `nix profile add nixpkgs#direnv` to install direnv, which automates the development shell process
4. Follow the instructions [here](https://direnv.net/docs/hook.html) to hook Direnv into your shell

#### Other Systems
We don't actively support these systems, but here's the dependency list for those who want to build on them:
1. A C++23 compiler (we use LLVM version 22)
2. The Meson build system
3. (optional) the Just command runner
    - If you don't have this, build the project like any other Meson project

## Configuring Your IDE
The dev shell contains a compatible build of our language tooling for consistency. To use it, use the following instructions (after reviewing the `.envrc` and `flake.nix`):

### VSCode
We provide a VSCodium package pre-configured to work with Nix.

#### macOS
macOS requires an extra step of installing all the extensions.
1. Run `nix run .#codium`
2. Install all the recommended extensions (the workspace is configured to provide everything needed for them)
3. Restart `codium` (make sure to close it all the way)
4. Reload the environment from `direnv` (it's a little notification, it gives you everything needed to develop each project)
5. Run `just configure` to set up the ClangD compilation database

#### Anything Else
We provide a plugin bundle for WSL and Linux that has a working C++ environment built in.
1. Run `nix run .#codium`
2. Reload the environment from `direnv` (it's a little notification, it gives you everything needed to develop each project)

### Neovim
1. Install the `direnv` extension
2. In a terminal, run `direnv allow`
3. Edit as you see fit (make sure you have ClangD enabled and pointed at whatever is on the `PATH`)

## Building
This repository makes use of the Meson build system, Just command runner, and Clang toolchain.
To get access to these tools, a Direnv shell is provided. Run `direnv allow` in the source directory (see [the setup section](#setup) if you do not have `nix` and `direnv` installed) and let the system install everything for development.

### Build (Debug)
#### Assignments
To build the assignments in debug mode, run `just assignments build [<MODE>]`
This can do a variety of debug and release builds, see the `justfile` for more information

#### Projects
To build a project in debug mode, run `just projects build <PROJECT> [<MODE>]`

This can do a variety of debug and release builds, see the `justfile` for more information

### Build (Release)
- To build the packaged version of the assignments, run `nix build`. This builds all assignments into one output directory (`result/bin`). 
- To build a project, run `nix build .#<PROJECT>`.
- In either case, the result will be in `result/bin`
- Projects are built using `meson` and with library dependencies handled through `nix`.

### Testing
To build the test suites, run `nix flake check` or `just assignments test` / `just projects <PROJECT> test`

### Running / Debugging
- To run a project/assignment, use the `run` subcommand.
- To debug a project/assignment in LLDB, use the `debug` subcoommand

On macOS, you may need to set the debug server path. To do this, use the following command:
```bash
export LLDB_DEBUGSERVER_PATH="$(xcode-select -p)/Library/PrivateFrameworks/LLDB.framework/Resources/debugserver"
```
If this turns up empty, you'll need the XCode CLI tools (`xcode-select --install`)

## AI Usage Disclosure
A large portion of this repository was made with the assistance of AI. This has always taken the form of Claude (not Claude Code, just Claude) interacting with me and helping me write better code. It has not taken the form of the AI coding for me outside of commits explicitly labelled `ai:` and those have always been small single file scripts generated in a chat session.

## License
This repository is licensed under the Apache License version 2.0. A copy of the license is available [here](./LICENSE.md)
