#!/bin/sh

remote_ip=$(uci get dnsmasq_route.config.remote_ip)
dns_server=$(uci get dnsmasq_route.config.dns_server)
ipv6_disable=$(uci get dnsmasq_route.config.ipv6_disable)
localnet_only=$(uci get dnsmasq_route.config.localnet_only)

if [ "a$localnet_only" == "a1" ] ; then
 sip=$(uci get network.lan.ipaddr)
 netmask=$(uci get network.lan.netmask)
 from_localnet_only="-s $sip/$netmask"
fi

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

if ! [ -e /etc/dnsmasq_route.ini ] ; then
 echo github.com > /etc/dnsmasq_route.ini
fi
echo "#auto gen from /etc/dnsmasq_route.ini" > /tmp/dnsmasq.d/dnsmasq_route.conf
cat /etc/dnsmasq_route.ini |grep -v  -e ^# -e ^$ |\
while read hostname note
do
 echo "server=/$hostname/$dns_server" >> /tmp/dnsmasq.d/dnsmasq_route.conf
 if [ "a$ipv6_disable" != "a0" ] ; then
  echo "address=/$hostname/::" >> /tmp/dnsmasq.d/dnsmasq_route.conf
 fi
done
/etc/init.d/dnsmasq restart
logread -l 2000 -f -S 128129 -e dnsmasq |dnsmasq_route -d $dns_server -r $remote_ip -t $table $from_localnet_only
