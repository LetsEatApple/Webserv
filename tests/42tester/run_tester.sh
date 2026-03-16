#!/bin/bash

cd "$(dirname "$0")" || exit 1

pkill -f webserv 2>/dev/null
sleep 1

cleanup() { kill "$PID" 2>/dev/null; wait "$PID" 2>/dev/null; }
trap cleanup EXIT

# Setup /tmp/www
mkdir -p /tmp/www /tmp/www/YoupiBanane/nop /tmp/www/YoupiBanane/Yeah
[ -f /tmp/www/index.html ] || echo '<html><body><h1>Welcome</h1></body></html>' > /tmp/www/index.html
[ -f /tmp/www/YoupiBanane/youpi.bad_extension ] || echo 'youpi.bad_extension' > /tmp/www/YoupiBanane/youpi.bad_extension
[ -f /tmp/www/YoupiBanane/youpi.bla ] || echo 'youpi.bla' > /tmp/www/YoupiBanane/youpi.bla
[ -f /tmp/www/YoupiBanane/youpla.bla ] || echo 'youpla.bla' > /tmp/www/YoupiBanane/youpla.bla
[ -f /tmp/www/YoupiBanane/nop/youpi.bad_extension ] || echo 'nop/youpi.bad_extension' > /tmp/www/YoupiBanane/nop/youpi.bad_extension
[ -f /tmp/www/YoupiBanane/nop/other.pouic ] || echo 'other.pouic' > /tmp/www/YoupiBanane/nop/other.pouic
[ -f /tmp/www/YoupiBanane/Yeah/not_happy.bad_extension ] || echo 'not_happy.bad_extension' > /tmp/www/YoupiBanane/Yeah/not_happy.bad_extension

# Build server (only if binary doesn't exist)
if [ ! -f ../../cmake-build-debug/webserv ]; then
    cmake -S ../.. -B ../../cmake-build-debug -DCMAKE_BUILD_TYPE=Debug > /dev/null 2>&1
    cmake --build ../../cmake-build-debug --target webserv || exit 1
fi

# Start server
sleep infinity | ../../cmake-build-debug/webserv webserv_tester.conf &
PID=$!
sleep 1

# Run tester (auto-press enter through prompts)
yes '' | ./tester http://127.0.0.1:8080
