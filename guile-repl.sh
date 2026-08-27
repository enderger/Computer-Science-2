#!/usr/bin/env bash
set -xeuo pipefail

SOCKET="/tmp/guile.socket"
if [ -f "$SOCKET" ]; then rm "$SOCKET"; fi
guile --listen="$SOCKET"
rm "$SOCKET"
