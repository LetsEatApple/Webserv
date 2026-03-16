#!/bin/bash
# Launches webserver, proxy, and tester together.
# Usage: run_tester.sh <webserv_binary> <tester_binary> <config_file>

set -e

WEBSERV="${1:?Usage: $0 <webserv> <tester> <config>}"
TESTER="${2:?Usage: $0 <webserv> <tester> <config>}"
CONFIG="${3:?Usage: $0 <webserv> <tester> <config>}"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROXY="$SCRIPT_DIR/../../tests/proxy.py"

SERVER_PORT=8080
PROXY_PORT=9090

cleanup() {
    [ -n "$WEBSERV_PID" ] && kill "$WEBSERV_PID" 2>/dev/null || true
    [ -n "$PROXY_PID" ] && kill "$PROXY_PID" 2>/dev/null || true
    wait 2>/dev/null
}
trap cleanup EXIT

# --- Setup /tmp/www directory structure ---
mkdir -p /tmp/www
if [ ! -f /tmp/www/index.html ]; then
    echo '<html><body><h1>Welcome</h1></body></html>' > /tmp/www/index.html
fi

mkdir -p /tmp/www/YoupiBanane/nop /tmp/www/YoupiBanane/Yeah

for f in /tmp/www/YoupiBanane/youpi.bad_extension \
         /tmp/www/YoupiBanane/nop/youpi.bad_extension \
         /tmp/www/YoupiBanane/nop/other.pouic \
         /tmp/www/YoupiBanane/Yeah/not_happy.bad_extension \
         /tmp/www/YoupiBanane/index.html; do
    [ -f "$f" ] || echo "<html><body><h1>$(basename "$f")</h1></body></html>" > "$f"
done

for f in /tmp/www/YoupiBanane/youpi.bla /tmp/www/YoupiBanane/youpla.bla; do
    if [ ! -f "$f" ]; then
        cat > "$f" << 'PYEOF'
#!/usr/bin/env python3
import os, sys
body = sys.stdin.read() if os.environ.get("CONTENT_LENGTH") else ""
content = "CGI output\n"
print("Content-Type: text/html")
print(f"Content-Length: {len(content)}")
print()
print(content, end="")
PYEOF
        chmod +x "$f"
    fi
done

echo "Directory structure ready in /tmp/www/"

# --- Start webserver (keep stdin open to prevent EOF exit) ---
sleep infinity | "$WEBSERV" "$CONFIG" &
WEBSERV_PID=$!
sleep 1

if ! kill -0 "$WEBSERV_PID" 2>/dev/null; then
    echo "ERROR: webserv failed to start"
    exit 1
fi
echo "Webserv running (PID $WEBSERV_PID)"

# --- Start proxy (optional, pass --no-proxy to skip) ---
if [ "${4}" = "--no-proxy" ] || [ ! -f "$PROXY" ]; then
    echo "Running tester directly (no proxy)"
    TESTER_URL="http://127.0.0.1:$SERVER_PORT"
else
    PYTHONUNBUFFERED=1 python3 "$PROXY" "$PROXY_PORT" "$SERVER_PORT" &
    PROXY_PID=$!
    sleep 0.5
    echo "Proxy running on :$PROXY_PORT -> :$SERVER_PORT (PID $PROXY_PID)"
    TESTER_URL="http://127.0.0.1:$PROXY_PORT"
fi

# --- Run tester ---
echo ""
echo "=== Running tester ==="
printf '\n\n\n\n\n\n\n\n\n\n' | "$TESTER" "$TESTER_URL"
EXIT_CODE=$?

exit $EXIT_CODE
