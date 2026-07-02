#!/bin/bash

mkdir build
cd build

# get openwrt sdk (21.02.2 newifi D2)
wget --no-proxy https://mirrors.cloud.tencent.com/openwrt/releases/25.12.5/targets/ramips/mt7621/openwrt-sdk-25.12.5-ramips-mt7621_gcc-14.3.0_musl.Linux-x86_64.tar.zst -O - |unzstd | tar x
cd openwrt-sdk-25.12.5-ramips-mt7621_gcc-14.3.0_musl.Linux-x86_64

mkdir package/dnsmasq_route -p 
cp -r ../../etc package/dnsmasq_route/
cp ../../Makefile package/dnsmasq_route/Makefile
cp -r ../../files package/dnsmasq_route/

make package/dnsmasq_route/compile V=99

find bin -name "dnsmasq_route*.ipk" -exec cp {} ../.. \;
ls ../../dnsmasq_route*.ipk -l
