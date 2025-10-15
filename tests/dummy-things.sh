#!/usr/bin/env bash

if [ $# -lt 1 ]; then
    echo "Usage: $0 COUNT" >&2
    exit 1
fi

COUNT=$1

for i in $(seq 1 "$COUNT"); do
    sudo ip link add dummy"$i" type dummy
    sudo ip link set dummy"$i" up
    printf "+"
    sleep 0.01
done
for i in $(seq 1 "$COUNT"); do
    sudo ip link del dummy"$i"
    printf "-"
    sleep 0.01
done
echo " done"
