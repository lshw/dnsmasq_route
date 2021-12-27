#!/bin/sh
logread -f -S 128000000 |dnsmasq_route -d 8.8.4.4 -r 10.7.0.1 -v -V
