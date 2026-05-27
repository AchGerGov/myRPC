#!/bin/sh

set -eu

./scripts/build.sh

./tests/test_stream.sh
./tests/test_dgram.sh
./tests/test_denied_user.sh
./tests/test_command_error.sh

echo "All tests passed"
