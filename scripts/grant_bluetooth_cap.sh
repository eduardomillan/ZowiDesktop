#!/usr/bin/env bash
# Grant CAP_NET_ADMIN to the CLI binary so it can bind RFCOMM / flash firmware
# without sudo. Re-apply after every rebuild.
#
# This is a manual fallback for the automation already in src/cli/CMakeLists.txt
# (which applies the capability via sudo -n after every build but cannot prompt
# for a password). Run it when the build prints the "NOTE: could not
# auto-apply CAP_NET_ADMIN" reminder, or after a rebuild done without cached
# sudo credentials.
#
# It is path-independent: it resolves the CLI relative to this script, so it can
# be invoked from anywhere in the repo.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
CLI="$REPO_ROOT/build/src/cli/zowi_cli"

if [ ! -f "$CLI" ]; then
    echo "FAIL: $CLI not found (build the CLI first: ./build.sh --cli)" >&2
    exit 1
fi

sudo setcap cap_net_admin+ep "$CLI"
echo "OK: CAP_NET_ADMIN granted to $CLI"
