#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <getopt.h>
#ifndef GIT_VER
#define GIT_VER "test"
#endif
#define ipv4_u32(a, b, c, d) ((uint32_t)(a<<24) | (b<<16) | (c<<8) | d)
uint32_t localnet[]={ /* 内网地址表 */
  ipv4_u32(0, 0, 0, 0), ipv4_u32(0, 255, 255, 255), /* 0.0.0.0/8 */
  ipv4_u32(10, 0, 0, 0), ipv4_u32(10, 255, 255, 255), /* 10.0.0.0/8 */
  ipv4_u32(100, 64, 0, 0), ipv4_u32(100, 127, 255, 255), /* 100.64.0.0/10 */
  ipv4_u32(127, 0, 0, 0), ipv4_u32(127, 255, 255, 255), /* 127.0.0.0/8 */
  ipv4_u32(169, 254, 0, 0), ipv4_u32(169, 254, 255, 255), /* 169.254.0.0/16 */
  ipv4_u32(172, 16, 0, 0), ipv4_u32(172, 32, 255, 255), /* 172.16.0.0/12 */
  ipv4_u32(192, 0, 0, 0), ipv4_u32(192, 0, 0, 255), /* 192.0.0.0/24 */
  ipv4_u32(192, 0, 2, 0), ipv4_u32(192, 0, 2, 255), /* 192.0.2.0/24 */
  ipv4_u32(192, 88, 99, 0), ipv4_u32(192, 88, 99, 255), /* 192.88.99.0/24 */
  ipv4_u32(192, 168, 0, 0), ipv4_u32(192, 168, 255, 255), /* 192.168.0.0/16 */
  ipv4_u32(198, 18, 0, 0), ipv4_u32(198, 19, 255, 255), /* 198.18.0.0/15 */
  ipv4_u32(198, 51, 100, 0), ipv4_u32(198, 51, 100, 255), /* 198.51.100.0/24 */
  ipv4_u32(203, 0, 113, 0), ipv4_u32(203, 0, 113, 255), /* 203.0.113.0/24 */
  ipv4_u32(224, 0, 0, 0), ipv4_u32(239, 255, 255, 255), /* 224.0.0.0/4 */
  ipv4_u32(233, 252, 0, 0), ipv4_u32(233, 252, 0, 255), /* 233.252.0.0/24 */
  /* ipv4_u32(240, 0, 0, 0), ipv4_u32(255, 255, 255, 255), // 240.0.0.0/4 */
  ipv4_u32(255, 255, 255, 255), ipv4_u32(255, 255, 255, 255), /* 255.255.255.255/32 */
  };
void log_scan();
void ip_rule(const char * dip);
#define PID_SIZE 20
uint32_t pids[PID_SIZE];
extern char * optarg;
char * buf0, * remote_ip, * dns_server, skip[100], *table;
#define RULES_SIZE 4096
struct rules {
uint8_t hour;
uint32_t ip;
} rules[RULES_SIZE];
bool v = false;
bool route_clean = false;
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

void update_rule_list() {
  FILE *dfp;
  uint16_t count = 0;
  uint32_t metric;
  uint8_t ip[4];
  system("ip rule list |grep ^290 |tr -d ':' >/tmp/dnsmasq_rule.list"); //初始化已经清单
  memset(rules, 0, sizeof(rules));
  dfp = fopen("/tmp/dnsmasq_rule.list", "r");
  if(!dfp) return;
  while(!feof(dfp)) {
    /*
29010:	from all to 120.121.121.140 lookup 107
29010:	from all to 120.121.121.141 lookup 107
     */
    int rc = fscanf(dfp,"%d %10s %10s %10s %hhd.%hhd.%hhd.%hhd", &metric, skip, skip, skip, &ip[0], &ip[1], &ip[2], &ip[3]);
    fgets(skip, sizeof(skip), dfp);
    if(rc != 8) continue;
    rules[count].hour = metric % 100;
    rules[count].ip = (uint32_t) (ip[0] << 24) | (ip[1] << 16) | (ip[2] << 8) || ip[3];
    count ++;
  }
  fclose(dfp);
  if(v)
    printf("load ip rule %d\r\n",count);
  remove("/tmp/dnsmasq_rule.list");
}

int main(int argc, char * argv[])
{
  int opt = 0;
  bool h = false;
  while((opt = getopt(argc, argv, "cChHvVd:r:t:")) != -1) {
    switch(opt) {
      case 'c':
      case 'C':
        route_clean = true;
        break;
      case 'h':
      case 'H': //帮助信息
        h = true;
        break;
      case 'v':
        v = true;
        break;
      case 'V':
        printf("Version:%s\r\n",GIT_VER);
        break;
      case 'd':
        dns_server = optarg;
        break;
      case 'r':
        remote_ip = optarg;
        break;
      case 't':
        table = optarg;
        break;
    }
  }
  if( h || table == NULL || (remote_ip == NULL && dns_server == NULL)) {
    printf("\r\nUsage:\r\nlogread -f -S 128000 |\\\r\n%s -d dns_server -r remote_ip -t 107\r\n\r\n -c 23 hours clean route\r\n -d remote dns server\r\n -r remote route ip\r\n -t route table name\r\n -v verbose mode\r\n -V display version\r\n\r\n",argv[0]);
    return 1;
  }
  if(remote_ip == NULL && dns_server != NULL)
    remote_ip = dns_server;
  if(remote_ip != NULL && dns_server == NULL)
    dns_server = remote_ip;

  if(table != NULL)  //rule方式
    update_rule_list();

  memset(pids,0,sizeof(pids));
  while(1){
    log_scan();
  }
  return 0;
}

void log_scan() {
  char buf[300],proc[31],sip[100],domain[100],to[100],dip[100];
  uint32_t pid;
  int rc;
  while(1) {
/*
Sun Dec 26 15:29:33 2021 daemon.info dnsmasq[11997]:xxxx
*/
    rc = scanf("%30s %30s %30s %30s %30s %30s %30s",
	skip, skip, skip, skip, skip, skip, proc);
    if(rc <= 0) continue;
    if(strncmp(proc,"dnsmasq[",sizeof("dnsmasq[")-1) != 0) {
      fgets(buf,sizeof(buf),stdin); //清理剩余部分
      continue;
     }
    /*
       dnsmasq[12670]:  177526 192.168.12.13/36330 query[A] www.google.com from 192.168.12.13
       dnsmasq[12670]:  177526 192.168.12.13/36330 forwarded www.google.com to 8.8.4.4
       dnsmasq[12670]:  177526 192.168.12.13/36330 reply www.google.com is 142.250.81.228
       dnsmasq[12670]:  177527 192.168.12.13/40934 query[AAAA] www.google.com from 192.168.12.13
       dnsmasq[12670]:  177527 192.168.12.13/40934 forwarded www.google.com to 8.8.4.4
       dnsmasq[12670]:  177527 192.168.12.13/40934 reply www.google.com is 2607:f8b0:4006:817::2004
       dnsmasq[2302]: 12173 192.168.12.1/55747 cached google.com is 173.194.219.101     */
    scanf("%d %99s %30s %99s %99s",
	&pid, sip, proc, domain, to);
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
	  ip_rule(dip); //路由名称不为空，就是修改rule路由规则
	}
      }
    }
  }
}

void ip_rule(const char * dip) {
  char buf[2048], dest[10];
  uint8_t ip[4];
  uint32_t dip32;
  time_t time0;
  struct tm tm;
  if(sscanf(dip, "%hhd.%hhd.%hhd.%hhd",&ip[0],&ip[1],&ip[2],&ip[3]) != 4) return;
  ip[3] = 0; //目标使用c段
  time(&time0);
  localtime_r(&time0, &tm);
  uint16_t count = 0;
  if(route_clean) { //命令行-c 需要删除
    for(count = 0; count < RULES_SIZE; count++) {
      if(rules[count].ip == 0) break;
      if(rules[count].hour == ((tm.tm_hour + 1) % 24)) { //清理23小时前的rule
        for(uint16_t i = count; i < RULES_SIZE; i ++) {
          if(rules[i + 1].ip == 0) break;
          rules[count] = rules[i + 1];
        }
        //删除路由
	snprintf(buf,sizeof(buf), "ip rule del to %s/32", dest);
	if(v)
	  printf("%s\r\n",buf);
	system(buf);
      }
    }
  }
  dip32 = (uint32_t) (ip[0] << 24) | (ip[1] << 16) | (ip[2] << 8) | ip[3];
  for(count = 0; count < RULES_SIZE; count++) {
    if(rules[count].ip == 0) break;
    if(rules[count].ip == dip32) return; //已经存在， 不需要添加
  }
  for(uint16_t i = 0; i < sizeof(localnet) / sizeof(uint32_t); i = i + 2) {
    if(localnet[i] <= dip32 && localnet[i + 1] >= dip32) return; /* 忽略内网地址 */
  }
  snprintf(buf,sizeof(buf),"ip rule add to %d.%d.%d.0/24 lookup %s pref %d", ip[0], ip[1], ip[2], table, 29000 + tm.tm_hour); //用metric 来区分每个小时添加的路由，方便定期清理
  if(v)
    printf("%s\r\n",buf);
  system(buf);
  if(count < RULES_SIZE && rules[count].ip == 0) {
    rules[count].hour = tm.tm_hour;
    rules[count].ip = dip32;
  }
}
