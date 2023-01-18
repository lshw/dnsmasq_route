$!/bin/sh
if [ -x logread ] ; then
logread -l 1000 -e dnsmasq |./dnsmasq_log -V
else
tail -n 1000 /var/log/daemon.log |./dnsmasq_log -V
fi
