#!/bin/sh


case "$1" in
	init | start ) 
                ifconfig wlan0 up                                                                                   
                touch /data/entropy.bin                                                                             
                wpa_supplicant -Dnl80211 -iwlan0 -c/data/wpa.conf -e/data/entropy.bin -puse_p2p_group_interface=1 -B
                wpa_cli -a /data/wpa.sh  &                  
                                                                  
                set -x                                                                                                       
		;;
    p2p_start) 
    	if [ ! -e /var/run/dnsmasq.pid ]
    		then    	
		        ifconfig p2p0 up ; ifconfig p2p0 192.168.3.1                                   
        		sleep 1
		        hostapd -B /data/hostapd.conf                                                   
        		sleep 1
		        dnsmasq -x /var/run/dnsmasq.pid -R -n -F 192.168.3.20,192.168.3.254,12h -i p2p0
		fi
		;;
    p2p_stop) 
        ifconfig p2p0 down
        killall -9 hostapd
		kill -9 `cat /var/run/dnsmasq.pid`
		rm -rf /var/run/dnsmasq.pid
		;;
	udhcpc_restart) 
		if [ -e '/var/run/udhcpc.pid' ] 
			then
				kill -9 `cat /var/run/udhcpc.pid`
				rm -rf /var/run/udhcpc.pid
		fi
        udhcpc -i wlan0 -p /var/run/udhcpc.pid -b
        ;;
	kill ) 
		killall -9 wpa_supplicant wpa_cli
		;;
    gen_hostapd )
        ;;
	setup_wlan0_mac ) 
        ifconfig wlan0 down                                    
        #  use by tlee 
		#  ifconfig wlan0 hw ether 00:50:43:02:EF:80 
        ifconfig wlan0 hw ether 00:50:43:02:EF:81 
        ifconfig wlan0 up
		;;
esac
