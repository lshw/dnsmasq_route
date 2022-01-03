#!/bin/sh

if [ "a$1" != "a" ] ; then
 dns_server=$1
else
 dns_server=10.7.0.1
fi

if [ "a$2" != "a" ] ; then
 remote_ip=$2
else
 remote_ip=10.7.0.1
fi

if [ "a$3" != "a" ] ; then
 table=$3
else
 table=107
fi

#多路由方式
if ! [ "`ip route list default table $table |grep $remote_ip`" ] ; then
 ip route add default via $remote_ip table $table
fi

logread -l 2000 -f -S 128129 -e dnsmasq |dnsmasq_route -d $dns_server -r $remote_ip -t $table
