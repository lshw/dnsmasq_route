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
void log_scan();
void ip_rule(const char * dip);
#define PID_SIZE 20
uint32_t pids[PID_SIZE];
bool v = false;
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
  system("ip rule list |grep ^290 |tr -d ':' >/tmp/dnsmasq_rule.list"); //初始化已经清单
}
extern char * optarg;
char * buf0, * remote_ip, * dns_server, skip[100], *table;
int main(int argc, char * argv[])
{
  int opt = 0;
  bool h = false;
  while((opt = getopt(argc, argv, "hHvVd:r:t:")) != -1) {
    switch(opt) {
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
    printf("\r\nUsage:\r\nlogread -f -S 128000 |\\\r\n%s -d dns_server -r remote_ip -t 107\r\n\r\n -d remote dns server\r\n -r remote route ip\r\n -t route table name\r\n -v verbose mode\r\n -V display version\r\n\r\n",argv[0]);
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
  FILE *dfp;
  char buf[2048], dest[10];
  uint8_t ip[20];
  char dip0[30];
  bool del = false, is_exists = false;
  int metric, metric_clean;
  time_t time0;
  struct tm tm;
  time(&time0);
  localtime_r(&time0, &tm);

  metric_clean=29000 + ((tm.tm_hour + 1) % 24); //根据metric 清理过期路由 下一小时
  sscanf(dip,"%hhd.%hhd.%hhd.%hhd",&ip[0],&ip[1],&ip[2],&ip[3]);
  snprintf(dip0, sizeof(dip0), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  if(strcmp(dip0, dip) != 0) //ip格式不对，退出
    return;
  dfp = fopen("/tmp/dnsmasq_rule.list", "r");
  if(!dfp) return;
  while(!feof(dfp)) {
    /*
29010:	from all to 120.121.121.140 lookup 107
29010:	from all to 120.121.121.141 lookup 107
     */
    metric = 0;
    int rc = fscanf(dfp,"%d %10s %10s %10s %30s", &metric, skip, skip, skip, dest);
    fgets(skip, sizeof(skip), dfp);
    if(rc <= 0) continue;
    if(v)
      printf("metric:%d,metric_clean:%d,dest:%s\r\n", metric, metric_clean, dest);
    if(metric == metric_clean) { //需要清理的路由
      snprintf(buf,sizeof(buf), "ip rule del to %s/32", dest);
      if(v)
	printf("%s\r\n",buf);
      system(buf);
      del = true;
      continue;
    }
    if(strncmp((char *)dip, dest,strlen(dest)) == 0){
      is_exists = true;
      continue; //路由表中存在此路由
    }
  }
  fclose(dfp);
  if(!is_exists) {
    snprintf(buf,sizeof(buf),"ip rule add to %s lookup %s pref %d", dip, table, 29000 + tm.tm_hour); //用metric 来区分每个小时添加的路由，方便定期清理
    if(v)
      printf("%s\r\n",buf);
    system(buf);
    if(!del) {
      dfp=fopen("/tmp/dnsmasq_rule.list","a");
      if(dfp) {
	snprintf(buf, sizeof(buf), "%d from all to %s lookup %s\n", 29000 + tm.tm_hour, dip, table);
	fputs(buf, dfp);
	fclose(dfp);
      }
    }
  }
  if(del)
    update_rule_list();
}
