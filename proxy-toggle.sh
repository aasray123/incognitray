#!/bin/bash

# Find active connection and device
CONN_NAME=$(nmcli -t -f NAME,STATE connection show --active | grep ':activated' | cut -d: -f1 | head -n 1)
DEV_NAME=$(nmcli -t -f DEVICE,STATE device | grep ':connected' | cut -d: -f1 | head -n 1)

if [ -z "$CONN_NAME" ]; then
    echo "Error: No active network connection found."
    exit 1
fi

if [ "$1" == "start" ]; then
    echo "Routing DNS through 127.0.0.1..."
    nmcli connection modify "$CONN_NAME" ipv4.dns "127.0.0.1" ipv4.ignore-auto-dns yes ipv6.dns "" ipv6.ignore-auto-dns yes
    nmcli device reapply "$DEV_NAME" 2>/dev/null || nmcli connection up "$CONN_NAME"
    echo "Proxy enabled!"

elif [ "$1" == "stop" ]; then
    echo "Restoring default DNS..."
    nmcli connection modify "$CONN_NAME" ipv4.ignore-auto-dns no ipv4.dns "" ipv6.ignore-auto-dns no ipv6.dns ""
    nmcli device reapply "$DEV_NAME" 2>/dev/null || nmcli connection up "$CONN_NAME"
    echo "Default DNS restored!"
fi
