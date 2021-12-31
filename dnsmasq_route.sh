#!/bin/sh

if [ $1 ] ; then
 dns_server= $1
else
 dns_server= 10.7.0.1
fi

if [ $2 ] ; then
 remote_ip= $3
else
 remote_ip= 10.7.0.1
fi

if [ $3 ] ; then
 table= $3
else
 table= 107
fi

#多路由方式
if ! [ "`ip route list default table $table |grep $remote_ip`" ] ; then
 ip route add default via $remote_ip table $table
fi
logread -f -S 128000 -e dnsmasq |dnsmasq_route -d $dns_server -r $remote_ip -t $table
