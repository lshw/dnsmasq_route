#!/bin/sh
cat syslog |./dnsmasq_route -s 192.168.0.0/24 -r 10.0.0.2 -d 8.8.4.4 -t 107 -v
