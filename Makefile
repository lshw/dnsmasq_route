# 此文件参考 https://github.com/wangyu-/UDPspeeder
UPDATE0=$(shell git log --date=short -1 |grep ^Date:)
UPDATE1=$(word 2,${UPDATE0})
UPDATE=$(subst -,,${UPDATE1})

GIT_VER=$(shell git rev-parse --short HEAD)
cc_cross=arm-none-linux-gnueabi-g++
cc_local=g++
cc_mips24kc_be=/toolchains/lede-sdk-17.01.2-ar71xx-generic_gcc-5.4.0_musl-1.1.16.Linux-x86_64/staging_dir/toolchain-mips_24kc_gcc-5.4.0_musl-1.1.16/bin/mips-openwrt-linux-musl-g++
cc_mips24kc_le=/toolchains/lede-sdk-17.01.2-ramips-mt7621_gcc-5.4.0_musl-1.1.16.Linux-x86_64/staging_dir/toolchain-mipsel_24kc_gcc-5.4.0_musl-1.1.16/bin/mipsel-openwrt-linux-musl-g++
cc_arm= /toolchains/lede-sdk-17.01.2-bcm53xx_gcc-5.4.0_musl-1.1.16_eabi.Linux-x86_64/staging_dir/toolchain-arm_cortex-a9_gcc-5.4.0_musl-1.1.16_eabi/bin/arm-openwrt-linux-c++
cc_mac_cross=o64-clang++ -stdlib=libc++
cc_x86=/toolchains/lede-sdk-17.01.2-x86-generic_gcc-5.4.0_musl-1.1.16.Linux-x86_64/staging_dir/toolchain-i386_pentium4_gcc-5.4.0_musl-1.1.16/bin/i486-openwrt-linux-c++
cc_amd64=/toolchains/lede-sdk-17.01.2-x86-64_gcc-5.4.0_musl-1.1.16.Linux-x86_64/staging_dir/toolchain-x86_64_gcc-5.4.0_musl-1.1.16/bin/x86_64-openwrt-linux-c++
#cc_bcm2708=/home/wangyu/raspberry/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian/bin/arm-linux-gnueabihf-g++ 

SOURCES=dnsmasq_route.c
NAME=dnsmasq_route

FLAGS= -std=c++11 -D GIT_VER=\"${UPDATE}-${GIT_VER}\" -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter -Wno-missing-field-initializers ${OPT}

TARGETS=amd64 arm mips24kc_be x86  mips24kc_le

export STAGING_DIR=/tmp/    #just for supress warning of staging_dir not define

# targets for nativei (non-cross) compile 
all:
	rm -f ${NAME}
	${cc_local}   -o ${NAME}          -I. ${SOURCES} ${FLAGS} -lrt -ggdb -static -O2

freebsd:
	rm -f ${NAME}
	${cc_local}   -o ${NAME}          -I. ${SOURCES} ${FLAGS} -lrt -ggdb -static -O2

mac:
	rm -f ${NAME}
	${cc_local}   -o ${NAME}          -I. ${SOURCES} ${FLAGS}  -ggdb -O2

cygwin:
	rm -f ${NAME}
	${cc_local}   -o ${NAME}          -I. ${SOURCES} ${FLAGS} -lrt -ggdb -static -O2 -D_GNU_SOURCE

#targes for general cross compile

cross:
	${cc_cross}   -o ${NAME}_cross    -I. ${SOURCES} ${FLAGS} -lrt -O2

cross2:
	${cc_cross}   -o ${NAME}_cross    -I. ${SOURCES} ${FLAGS} -lrt -static -lgcc_eh -O2

cross3:
	${cc_cross}   -o ${NAME}_cross    -I. ${SOURCES} ${FLAGS} -lrt -static -O2

#targets only for debug purpose
fast: 
	rm -f ${NAME}
	${cc_local}   -o ${NAME}          -I. ${SOURCES} ${FLAGS} -lrt -ggdb
debug: 
	rm -f ${NAME}
	${cc_local}   -o ${NAME}          -I. ${SOURCES} ${FLAGS} -lrt -Wformat-nonliteral -D MY_DEBUG -ggdb
debug2: 
	rm -f ${NAME}
	${cc_local}   -o ${NAME}          -I. ${SOURCES} ${FLAGS} -lrt -Wformat-nonliteral -ggdb

mips24kc_be: 
	${cc_mips24kc_be}  -o ${NAME}_$@   -I. ${SOURCES} ${FLAGS} -lrt -lgcc_eh -static -O2

mips24kc_le: 
	${cc_mips24kc_le}  -o ${NAME}_$@   -I. ${SOURCES} ${FLAGS} -lrt -lgcc_eh -static -O2

amd64:
	${cc_amd64}   -o ${NAME}_$@    -I. ${SOURCES} ${FLAGS} -lrt -static -O2 -lgcc_eh -ggdb

x86:          #to build this you need 'g++-multilib' installed 
	${cc_x86}   -o ${NAME}_$@      -I. ${SOURCES} ${FLAGS} -lrt -static -O2 -lgcc_eh -ggdb
arm:
	${cc_arm}   -o ${NAME}_$@      -I. ${SOURCES} ${FLAGS} -lrt -static -O2 -lgcc_eh


mac_cross:   #need to install 'osxcross' first.
	${cc_mac_cross}   -o ${NAME}_mac          -I. ${SOURCES} ${FLAGS}  -ggdb -O2

clean:	
	rm -f ${TAR}
	rm -f ${NAME} ${NAME}_cross ${NAME}_mac
