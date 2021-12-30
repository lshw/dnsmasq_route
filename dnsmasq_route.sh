#!/bin/sh

#多路由方式
if ! [ "`ip route list default table 107 |grep 10.7.0.1`" ] ; then
 ip route add default table 107
fi
logread -f -S 128000 -e dnsmasq |dnsmasq_route -d 8.8.4.4 -r 10.7.0.1 -v -V -t 107
