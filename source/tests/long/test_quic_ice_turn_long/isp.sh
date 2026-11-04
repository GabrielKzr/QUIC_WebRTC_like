#!/bin/sh
set -eu
export ICE_TEST_REQUIRE_RENEWALS=1
exec sh "$(dirname "$0")/../../nice/test_ice_udp_transport_turn/isp.sh"
