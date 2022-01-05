#!/bin/bash

mkdir build
cd build

# get openwrt sdk (21.02.2 newifi D2)
wget --no-proxy https://mirrors.cloud.tencent.com/openwrt/releases/21.02.2/targets/ramips/mt7621/openwrt-sdk-21.02.2-ramips-mt7621_gcc-8.4.0_musl.Linux-x86_64.tar.xz -c
tar Juxvf openwrt-sdk-21.02.2-ramips-mt7621_gcc-8.4.0_musl.Linux-x86_64.tar.xz
cd openwrt-sdk-21.02.2-ramips-mt7621_gcc-8.4.0_musl.Linux-x86_64

mkdir package/dnsmasq_route -p 
wget "https://github.com/lshw/dnsmasq_route/raw/master/openwrt/Makefile" -O package/dnsmasq_route/Makefile

make package/dnsmasq_route/compile V=99

find bin -name "dnsmasq_route*.ipk" -exec cp {} ../.. \;
ls ../../dnsmasq_route*.ipk -l
