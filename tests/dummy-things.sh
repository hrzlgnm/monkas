#!/usr/bin/env bash

set -euo pipefail

if [ "${EUID:-$(id -u)}" -ne 0 ]; then
    echo "Please run as root (sudo)." >&2
    exit 1
fi

if ! [[ "${1:-}" =~ ^[0-9]+$ ]] || [ "${1:-0}" -le 0 ]; then
    echo "Usage: $0 COUNT  (COUNT must be a positive integer)" >&2
    exit 1
fi

COUNT=$1

for i in $(seq 1 "$COUNT"); do
    ip link add dummy"$i" type dummy
    ip link set dummy"$i" up
    printf "+"
    sleep 0.01
done
for i in $(seq 1 "$COUNT"); do
    ip link del dummy"$i"
    printf "-"
    sleep 0.01
done
echo " done"
