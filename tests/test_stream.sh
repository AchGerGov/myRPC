#!/bin/sh

set -eu

USER_NAME="$(whoami)"
SERVER_PID=""

cleanup() {
    if [ -n "$SERVER_PID" ]; then
        kill -INT "$SERVER_PID" 2>/dev/null || true
        wait "$SERVER_PID" 2>/dev/null || true
    fi
}

trap cleanup EXIT INT TERM

sudo mkdir -p /etc/myRPC

cat <<EOF | sudo tee /etc/myRPC/myRPC.conf >/dev/null
port = 1234
socket_type = stream
daemon = no
log_file = /tmp/myRPC-test.log
EOF

echo "$USER_NAME" | sudo tee /etc/myRPC/users.conf >/dev/null
sudo touch /tmp/myRPC-test.log
sudo chmod 666 /tmp/myRPC-test.log

./bin/myRPC-server &
SERVER_PID="$!"

sleep 1

OUTPUT="$(./bin/myRPC-client -h 127.0.0.1 -p 1234 -s -c "echo stream_ok")"

echo "$OUTPUT" | grep "server response code: 0"
echo "$OUTPUT" | grep "stream_ok"

echo "stream test passed"
