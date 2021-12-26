#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
void log_scan();
void route(const char * dip, const uint8_t hour);
char hextohbin(const uint8_t dat); //把半个hex转换成bin
#define PID_SIZE 20
uint32_t pids[PID_SIZE];
void add_key(const uint32_t pid) { //将转发到8.8.4.4的请求id 记录下来，
  uint32_t dat;
  for(uint8_t i = 0; i < PID_SIZE; i++) {
    if(pids[i] == 0){
      pids[i] = pid;
      return;
    }
    if(i != 0 && pids[0] > pids[i]){
      dat = pids[0];
      pids[0] = pids[i];
      pids[i] = dat;
    }else if(i != PID_SIZE - 1 && pids[PID_SIZE - 1] < pids[i]) {
      dat = pids[PID_SIZE - 1];
      pids[PID_SIZE - 1] = pids[i];
      pids[i] = dat;
    }
  }
  if(pid > pids[PID_SIZE - 1]) //如果没有溢出就覆盖最小的pid
    pids[0] = pid;
  else  //如果溢出，就覆盖最大的pid, 这里有极小的概率（10的11次方分之一)，会丢一次回应，可以忽略。
    pids[PID_SIZE - 1] = pid;
}

bool in_key(const uint32_t pid) { //看回应的pid，是否在转发到8.8.4.4的记录中，
  for(uint8_t i = 0; i < PID_SIZE; i++)
    if(pids[i] == pid)
      return true;
  return false;
}
char * buf0, * remote_ip, * dns_server, skip[31];
void main(int argc, char * argv[])
{
  if(argc < 2) {
    printf("logread -f |%s dns_server [remote_route]\r\n",argv[0]);
    return;
  }
  if(argc >= 2) {
    remote_ip=argv[1];
    dns_server=argv[1];
  }
  if(argc >= 3)
    remote_ip=argv[2];
  memset(pids,0,sizeof(pids));
  while(1){
    log_scan();
  }
}
void log_scan() {
  char buf[300],proc[31],sip[100],domain[100],to[100],dip[100];
  uint8_t skip_i;
  uint8_t hour;
  uint32_t pid;
  while(!feof(stdin)) {
    scanf("%30s %30s %d %d:%d:%d %d %30s %30s",
	skip,skip,
	&skip_i,&hour,&skip_i,&skip_i, &skip_i, skip, proc);
    if(strncmp(proc,"dnsmasq[",sizeof("dnsmasq[")-1) != 0) continue;
    /*
       dnsmasq[12670]:  177526 192.168.12.13/36330 query[A] www.google.com from 192.168.12.13
       dnsmasq[12670]:  177526 192.168.12.13/36330 forwarded www.google.com to 8.8.4.4
       dnsmasq[12670]:  177526 192.168.12.13/36330 reply www.google.com is 142.250.81.228
       dnsmasq[12670]:  177527 192.168.12.13/40934 query[AAAA] www.google.com from 192.168.12.13
       dnsmasq[12670]:  177527 192.168.12.13/40934 forwarded www.google.com to 8.8.4.4
       dnsmasq[12670]:  177527 192.168.12.13/40934 reply www.google.com is 2607:f8b0:4006:817::2004
       dnsmasq[2302]: 12173 192.168.12.1/55747 cached google.com is 173.194.219.101     */
    scanf("%d %99s %30s %99s %99s",
	&pid,sip, proc , &domain,&to);
    fgetc(stdin);
    fgets(dip,sizeof(dip),stdin);
    buf0 = strstr(dip,"\n");
    if(buf0)
      buf0[0] = 0;
    buf0=strstr(dip,"\r");
    if(buf0)
      buf0[0]=0;
    if(strcmp(proc, "forwarded") == 0 && strncmp(dip,dns_server,strlen(dns_server)) == 0) { //找出转发到8.8.4.4的请求
      add_key(pid); //保存请求id
    }else{
      if(strcmp(to, "is") == 0) {
	if(in_key(pid)        //如果是记录的请求id的回应， 就处理
	    && !index(dip, ':')){   //去掉ipv6回应
	  route(dip, hour);
	}
      }
    }
  }
}

void route(const char * dip, const uint8_t hour) {
  FILE *dfp;
  char buf[2048], desc[10];
  uint16_t metric, metric_clean;
  metric_clean=65000 + ((hour + 1) % 24); //根据metric 清理过期路由 下一小时
  dfp = fopen("/proc/net/route", "r");
  if(!dfp) return;
  fgets(buf, sizeof(buf), dfp);//skip head
  while(!feof(dfp)) {
    fgets(buf, sizeof(buf), dfp);
    /*
       Iface   Destination     Gateway         Flags   RefCnt  Use     Metric  Mask            MTU     Window  IRTT
       eth0    00000000        01D5D2C0        0003    0       0       0       00000000        0       0       0
       */
    fscanf(dfp,"%30s %10s %10s %30s %30s %30s %d", skip, desc, skip, skip, skip, skip, metric);
    fgets(skip,sizeof(skip), dfp);
    snprintf(buf,sizeof(buf), "%d.%d.%d.%d",
	(hextohbin(desc[0]) << 8) | hextohbin(desc[1]),
	(hextohbin(desc[2]) << 8) | hextohbin(desc[3]),
	(hextohbin(desc[4]) << 8) | hextohbin(desc[5]),
	(hextohbin(desc[6]) << 8) | hextohbin(desc[7]));
    if(metric == metric_clean) { //需要清理的路由
      snprintf(buf,sizeof(buf),"ip ro del %s metric %d", dip, metric);
      system(buf);
      continue;
    }
    if(strcmp(buf,dip) == 0) return; //路由表中存在此路由
  }
  snprintf(buf,sizeof(buf),"ip ro add %s metric %d via %s",dip,hour + 65000,remote_ip); //用metric 来区分每个小时添加的路由，方便定期清理
  system(buf);
}
char hextohbin(uint8_t dat){
  const char ret[]={0,1,2,3,4,5,6,7,8,9,0xa,0xb,0xc,0xd,0xe,0xf};
  return ret[dat & 0xf];
}
