#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#define __USE_XOPEN 1
#include <time.h>
#include <getopt.h>
#include <sys/stat.h>
#ifndef GIT_VER
#define GIT_VER "test"
#endif
bool log_scan(const char * filename);
bool v = false;
uint32_t ips[2048];
char skip[100];
extern char * optarg;
struct hostname {
uint32_t ip;
char name[31];
} hostnames[512];
void load_hostname(){
  FILE * fp;
  fp = fopen("/tmp/dhcp.leases","r");
  if(!fp) return;
  int rc;
  uint16_t count = 0;
 
  uint8_t ip[4];
  while(!feof(fp)) {
    rc = fscanf(fp,"%30s %30s %hhd.%hhd.%hhd.%hhd %30s %30s", skip, skip, &ip[0],&ip[1],&ip[2],&ip[3], hostnames[count].name, skip);
    fgets(skip,sizeof(skip),fp);
    if(rc != 8 || hostnames[count].name[0] == '*'){
      continue;
    }
    hostnames[count].ip = (uint32_t) (ip[0] << 24) | (ip[1] << 16) | (ip[2] << 8) | ip[3];
    count ++;
  }
  fclose(fp);
}
void load_ips() {
  FILE *fp;
  int rc;
  uint8_t ip[4];
  uint16_t count = 0;
  memset(ips,0,sizeof(ips));
  fp=fopen("/tmp/dnsmasq_rule.list","r");
  if(fp) {
    while(!feof(fp)) {
    /*
29010:	from all to 120.121.121.140 lookup 107
29010:	from all to 120.121.121.141 lookup 107
     */
    rc = fscanf(fp,"%10s %10s %10s %10s %hhd.%hhd.%hhd.%hhd", skip, skip, skip, skip, &ip[0],&ip[1],&ip[2],&ip[3]);
    fgets(skip, sizeof(skip), fp);
    if(rc != 8) continue;
    ips[count]=(uint32_t) (ip[0] << 24) | (ip[1] << 16) | (ip[2] << 8) | ip[3];
    if(v)
      printf("%d.%d.%d.%d:%08x,%d\n", ip[0], ip[1], ip[2], ip[3], ips[count], count);
    count++;
    }
  fclose(fp);
  }
}
char exists[2048 * 2];
bool is_exists(const uint32_t ip, char * host) {
  char buf[100];

  snprintf(buf, sizeof(buf), "%08x %s ", ip, host);
  uint16_t len = strlen(buf);
  if(strstr(exists, buf) != NULL)
    return true; //已经存在
  if(strlen(exists) + len > sizeof(exists)-1) {//溢出
    memcpy(exists, &exists[sizeof(exists) / 2], sizeof(exists) / 2);
  }
  len = strlen(exists);
  strncat(exists, buf, sizeof(exists) - 1);
  return false;
}
extern char *  optarg;
uint16_t l = 3600;
int main(int argc, char * argv[])
{
  int opt = 0;
  bool h = false;
  while((opt = getopt(argc, argv, "l:L:hHvVd:")) != -1) {
    switch(opt) {
      case 'h':
      case 'H': //帮助信息
        h = true;
        break;
      case 'l':
      case 'L': //要显示的最后秒数
        l = atoi(optarg);
        break;
      case 'v':
        v = true;
        break;
      case 'V':
        printf("Version:%s\n",GIT_VER);
        break;
    }
  }
  if( h ) {
    printf("\r\nUsage:\r\n%s -l 600 [-v] [-V]\r\n\r\n -l 要显示的时长秒数\r\n -v verbose mode\r\n -V display version\r\n\r\n",argv[0]);
    return 1;
  }
  load_hostname();
  load_ips();
  bool f1 = log_scan("/tmp/syslog.old");
  bool f2 = log_scan("/tmp/syslog");
  if(!f1 && !f2)
  printf("/tmp/syslog  not find");
  return 0;
}

bool log_scan(const char * filename) {
  char buf[300],proc[31],domain[100],to[100],domain0[100];
  int rc;
  uint8_t sip[4];
  char sipstr[30];
  uint8_t dip[4];
  uint32_t dip32;
  uint16_t year;
  char month[20];
  struct tm tm, tm_now;
  char *ipname;
  ipname = sipstr;
  FILE * fp = fopen(filename, "r");
  if(!fp)
    return false;
  time_t now_t=time(NULL);
  localtime_r(&now_t, &tm_now);
  uint32_t t0 = ((24 + tm_now.tm_hour) * 60 + tm_now.tm_min) * 60 + tm_now.tm_sec - l; //mktime()偶尔会出错 不用mktime
  uint32_t t1;
  while(!feof(fp)) {
/* 
Sun Dec 26 15:29:33 2021 daemon.info dnsmasq[11997]:xxxx
*/
    rc = fscanf(fp, "%30s %30s %d %02d:%02d:%02d %hd %30s dnsmasq%s",
	skip, month, &tm.tm_mday, &tm.tm_hour, &tm.tm_min, &tm.tm_sec, &year, skip, proc);
    if(rc != 9) {
      continue;
    }
     t1 = (tm.tm_hour * 60 + tm.tm_min) * 60 + tm.tm_sec;
     if(tm.tm_mday == tm_now.tm_mday) t1 += 24 * 3600;
     if(t1 < t0)  {
       continue; //长于10分钟之前的访问， 就跳过
     }
     
    /*
       dnsmasq[12670]:  177526 192.168.12.13/36330 query[A] www.google.com from 192.168.12.13
       dnsmasq[12670]:  177526 192.168.12.13/36330 forwarded www.google.com to 8.8.4.4
       dnsmasq[12670]:  177526 192.168.12.13/36330 reply www.google.com is 142.250.81.228
       dnsmasq[12670]:  177527 192.168.12.13/40934 query[AAAA] www.google.com from 192.168.12.13
       dnsmasq[12670]:  177527 192.168.12.13/40934 forwarded www.google.com to 8.8.4.4
       dnsmasq[12670]:  177527 192.168.12.13/40934 reply www.google.com is 2607:f8b0:4006:817::2004
       dnsmasq[2302]: 12173 192.168.12.1/55747 cached google.com is 173.194.219.101     */
    rc = fscanf(fp, "%30s %hhd.%hhd.%hhd.%hhd/%s %30s %99s %99s %hhd.%hhd.%hhd.%hhd",
	skip, &sip[0], &sip[1], &sip[2], &sip[3], skip, proc, domain, to, &dip[0], &dip[1], &dip[2], &dip[3]);
    fgets(buf,sizeof(buf),fp); //清理剩余部分
    if(rc != 13) continue; //行不完整跳过
    if(sip[0] == 127) continue; //申请地址是本机127.0.0.1， 跳过
    if(strcmp(to, "is") != 0) continue; //只显示 is 行
  //  if(strcmp(proc, "cached") != 0) continue; //不显示 cache 行
    if(strcmp(domain, "localhost") == 0) continue; //要解析的域名是 localhost 跳过
    if(strcmp(domain, domain0) == 0) continue;   //与上次域名不同
    uint32_t sip32 = (uint32_t) (sip[0] << 24) | (sip[1] << 16) | (sip[2] << 8) | sip[3];
    if(is_exists(sip32, domain)) continue;
    strncpy(domain0, domain, sizeof(domain));
    snprintf(sipstr, sizeof(sipstr), "%d.%d.%d.%d", sip[0], sip[1] ,sip[2], sip[3]);
    ipname = sipstr;
    bool is_route = false;
    for(uint16_t l = 1; l < 512; l++) //查dhcp.lease文件，替换ip为机器名
      if(hostnames[l].ip == sip32) {
        ipname = hostnames[l].name;
        break;
      }
    dip32 = (uint32_t) (dip[0] << 24) | (dip[1] << 16) | (dip[2] << 8) | dip[3];
       is_route = false;
       for(uint16_t i = 0; i< 2048; i ++) { //检查是否在 已经转路由的ip列表里面，有的话， 就不显示
         if(ips[i] == 0) break; //ips结束
         if(ips[i] == dip32) {
           is_route = true;
           break;
         }
       }
       if(is_route) continue; //已经在ips里的， 不要再显示

       memset(skip, 0, sizeof(skip));
       year = strlen(ipname);
       if(year < 30) {
          memset(skip, ' ', 30 - year);
       }
       printf("%02d:%02d:%02d %s%s \t%s\t%d.%d.%d.%d\n",
             tm.tm_hour,
             tm.tm_min,
             tm.tm_sec,
             skip,
             ipname,
             domain,
             dip[0], dip[1], dip[2], dip[3]
      );
  }
  fclose(fp);
  return true;
}
