#!/bin/bash

# Automatically find the name of the currently active connection
CONN_NAME=$(nmcli -t -f NAME,STATE connection show --active | grep ':activated' | cut -d: -f1 | head -n 1)

if [ -z "$CONN_NAME" ]; then
    echo "Error: Could not detect an active network connection."
    exit 1
fi

if [ "$1" == "start" ]; then
    echo "Routing DNS for '$CONN_NAME' through 127.0.0.1..."
    sudo nmcli connection modify "$CONN_NAME" ipv4.dns "127.0.0.1"
    sudo nmcli connection modify "$CONN_NAME" ipv4.ignore-auto-dns yes
    sudo nmcli connection modify "$CONN_NAME" ipv6.dns ""
    sudo nmcli connection modify "$CONN_NAME" ipv6.ignore-auto-dns yes
    sudo nmcli connection up "$CONN_NAME"
    echo "Proxy enabled!"

elif [ "$1" == "stop" ]; then
    echo "Restoring default DNS for '$CONN_NAME'..."
    sudo  nmcli connection modify "$CONN_NAME" ipv4.ignore-auto-dns no ipv4.dns "" ipv6.ignore-auto-dns no ipv6.dns ""
    sudo nmcli connection up "$CONN_NAME"
    echo "Default DNS restored!"

else
    echo "Usage: ./proxy-toggle.sh [start|stop]"
fi
