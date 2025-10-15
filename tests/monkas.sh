#!/usr/bin/env -S bash
# monkas.sh
# Usage: ./monkas.sh COUNT

set -euo pipefail

if [ ! -e ../build/examples/cli/monka ]; then
    echo "Error: monka binary not found or not executable. Please build the project first." >&2
    exit 1
fi

if [ $# -lt 1 ]; then
    echo "Usage: $0 COUNT" >&2
    exit 1
fi

COUNT="$1"

pids=()
declare -a launched_info=()

cleanup() {
    echo
    echo "Stopping ${#pids[@]} monkas processes..."
    for pid in "${pids[@]}"; do
        if kill -0 "$pid" >/dev/null 2>&1; then
            kill "$pid" || true
        fi
    done
    # give processes a moment to exit
    sleep 0.2
    # ensure they're dead
    for pid in "${pids[@]}"; do
        if kill -0 "$pid" >/dev/null 2>&1; then
            kill -9 "$pid" >/dev/null 2>&1 || true
        fi
    done
    echo "Done."
    exit 0
}
trap cleanup INT TERM EXIT

printf "Cleaning up old monka logs... "
rm -f /tmp/monka-*.log
for ((i = 0; i < COUNT; i++)); do
    ../build/examples/cli/monka --enum-loop 0 --log-to-file &
    pid=$!
    pids+=("$pid")
    # store info to show to user
    launched_info+=("PID=$pid")
    # small stagger to avoid starting them all exactly same nanosecond
    sleep 0.02
done

echo "Launched ${#pids[@]} monkas:"
for info in "${launched_info[@]}"; do
    echo "  $info"
done

echo
echo "${#pids[@]} Monkas will remain while this script runs. Press Ctrl+C to stop."
# wait forever (until trap fires)
wait
