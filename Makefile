# 此文件参考 https://github.com/wangyu-/UDPspeeder
UPDATE0=$(shell git log --date=short -1 |grep ^Date:)
UPDATE1=$(word 2,${UPDATE0})
UPDATE=$(subst -,,${UPDATE1})

GIT_VER=$(shell git rev-parse --short HEAD)
CC?=gcc

SOURCES=dnsmasq_route.c
NAME=dnsmasq_route

FLAGS= -D GIT_VER=\"${UPDATE}-${GIT_VER}\" -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter -Wno-missing-field-initializers ${OPT}

all: cross

cross:
	rm -f ${NAME}
	${CC}   -o ${NAME}  -I. ${SOURCES} ${FLAGS} -O2
	${CC}   -o dnsmasq_log  -I. dnsmasq_log.c ${FLAGS} -O2

clean:	
	rm -f ${TAR}
	rm -f ${NAME} ${NAME}_mac
