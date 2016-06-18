#!/bin/sh

echo "PARAM: $* "

case "$2" in
    CONNECTED)
	echo "CONNECTED"
        udhcpc -i wlan0 -p /data/udhcpc_wlan0.pid 
        ;;
    DISCONNECTED)
	echo "DISCONNECTED"
	cat /data/udhcpc_wlan0.pid
	kill -9 `cat /data/udhcpc_wlan0.pid`
        ;;
esac
