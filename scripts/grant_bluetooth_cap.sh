#!/usr/bin/env bash
# Grant CAP_NET_ADMIN to the CLI binary so it can bind RFCOMM / flash
# firmware without sudo.  Re-apply after every rebuild.
exec sudo setcap cap_net_admin+ep build/src/cli/zowi_cli
