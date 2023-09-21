#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define __USE_XOPEN 1
#include <time.h>
#include <getopt.h>
#include <sys/stat.h>
#ifndef GIT_VER
#define GIT_VER "test"
#endif
void log_scan();
uint32_t ips[2048];
char skip[200];
extern char * optarg;
struct hostname {
  uint32_t ip;
  char name[21];
} hostnames[512];
void load_hostname(){
  FILE * fp;
  fp = fopen("/tmp/dhcp.leases","r");
  if(!fp) return;
  int rc;
  uint16_t count = 0;
 
  uint8_t ip[4];
  while(!feof(fp)) {
    rc = fscanf(fp,"%30s %30s %hhd.%hhd.%hhd.%hhd %20s %30s", skip, skip, &ip[0],&ip[1],&ip[2],&ip[3], hostnames[count].name, skip);
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
  system("ip ru list |grep ^290 >/tmp/dnsmasq_rule.list");
  fp=fopen("/tmp/dnsmasq_rule.list","r");
  if(fp) {
    while(!feof(fp)) {
    /*
29010:	from all to 120.121.121.0 lookup 107
     */
      rc = fscanf(fp,"%10s %10s %10s %10s %hhd.%hhd.%hhd.%hhd", skip, skip, skip, skip, &ip[0],&ip[1],&ip[2],&ip[3]);
      fgets(skip, sizeof(skip), fp);
      if(rc != 8) continue;
      ips[count] = (uint32_t) (ip[0] << 24) | (ip[1] << 16) | (ip[2] << 8) | ip[3];
      count++;
    }
  fclose(fp);
  remove("/tmp/dnsmasq_rule.list");
  }
}
extern char *  optarg;
int32_t l = 3600;
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
      case 'V':
      case 'v':
        printf("Version:%s\n",GIT_VER);
        break;
    }
  }
  if( h ) {
    printf("\r\n"
           "Usage:\r\n"
           "logread -l 10000 -e dnsmasq -S 80000 [|grep 192.168.1.2] |%s -l 600 [-s 192.168.1.2] [-v] [-V]\r\n\r\n"
           "tail -n 10000 |grep dnsmasq [|grep 192.168.1.2] |%s -l 600 [-s 192.168.1.2] [-v] [-V]\r\n\r\n"
           " -l 要显示的时长秒数\r\n"
           " -V display version\r\n\r\n",
           argv[0],
           argv[0]);
    return 1;
  }
  load_hostname();
  load_ips();
  log_scan();
  return 0;
}

void log_scan() {
  char buf[300],proc[31],domain[100],to[100],domain0[100], cname[100];
  int rc;
  uint8_t sip[4];
  char sipstr[30];
  uint8_t dip[4];
  uint32_t pid, cname_pid = 0;
  uint32_t dip32;
  uint16_t year;
  char month[20];
  struct tm tm, tm_now;
  char *ipname;
  ipname = sipstr;
  time_t now_t=time(NULL);
  localtime_r(&now_t, &tm_now);
  int32_t t0 = ((tm_now.tm_mday * 24 + tm_now.tm_hour) * 60 + tm_now.tm_min) * 60 + tm_now.tm_sec; //mktime()偶尔会出错 不用mktime
  int32_t t1;
  while(1) {
/* 
Sun Dec 26 15:29:33 2021 daemon.info dnsmasq[11997]:xxxx
Jan 15 04:01:51 route dnsmasq[1405]: 41330 192.168.3.3/56062 reply cloud.browser.360.cn is 112.64.200.152
*/
    while(1) {
      rc = scanf("%30s",skip);
      if(rc <= 0) return;
      if((skip[0] & skip[1] & skip[3] & skip[4] & skip[6] & skip[7] & 0xf0) == 0x30
      &&  skip[2] == ':'
      &&  skip[5] == ':'
      && ((skip[0] | skip[1] | skip[3] | skip[4] | skip[6] | skip[7] ) & 0xf0) == 0x30) {
        sscanf(skip,"%02d:%02d:%02d",&tm.tm_hour, &tm.tm_min, &tm.tm_sec);
        for(uint8_t i = 0; i < 5; i++) {
          rc = scanf("%30s",skip);
          if(rc <= 0) return;
          skip[7] = 0;
          if(strcmp(skip, "dnsmasq") == 0) {
            break;
          }
        }
        if(strcmp(skip, "dnsmasq") == 0) {
          break; //找到时间字段
        }
      } else if((strlen(skip) == 2) && (skip[0] <= '9') && (skip[0] >= '0') && (skip[1] <= '9') && (skip[1] >= '0')) {
        sscanf(skip,"%d",&tm.tm_mday);
      }
      if(strcmp(skip, "dnsmasq") == 0) {
        break; //找到时间字段
      }
    }
    if(tm.tm_mday > tm_now.tm_mday)
      tm.tm_mday -= 30;
    t1 = ((tm.tm_mday * 24 + tm.tm_hour) * 60 + tm.tm_min) * 60 + tm.tm_sec; //mktime()偶尔会出错 不用mktime
    if(t0 > t1 + l)  {
      continue; //长于10分钟之前的访问， 就跳过
    }
     
    /*
       dnsmasq[12670]:  177526 192.168.12.13/36330 query[A] www.google.com from 192.168.12.13
       dnsmasq[12670]:  177526 192.168.12.13/36330 forwarded www.google.com to 8.8.4.4
       dnsmasq[12670]:  177526 192.168.12.13/36330 reply www.google.com is 142.250.81.228
       dnsmasq[12670]:  177527 192.168.12.13/40934 query[AAAA] www.google.com from 192.168.12.13
       dnsmasq[12670]:  177527 192.168.12.13/40934 forwarded www.google.com to 8.8.4.4
       dnsmasq[14028]: 380 192.168.12.4/37307 reply yt3.ggpht.com is <CNAME>
       dnsmasq[14028]: 380 192.168.12.4/37307 reply photos-ugc.l.googleusercontent.com is 142.251.43.1
    */

    rc = scanf("%d %hhd.%hhd.%hhd.%hhd/%s %30s %99s %99s %30s",
	&pid, &sip[0], &sip[1], &sip[2], &sip[3], skip, proc, domain, to, skip);
    fgets(buf, sizeof(buf), stdin); //清理剩余部分
    if(rc != 10) continue; //行不完整跳过
    if(strcmp(skip,"<CNAME>")==0) {
      cname_pid = pid;
      strcpy(cname, domain);
      continue;
    }
    if(sscanf(skip,"%hhd.%hhd.%hhd.%hhd", &dip[0], &dip[1], &dip[2], &dip[3]) != 4) continue;
    if(sip[0] == 127) continue; //申请地址是本机127.0.0.1， 跳过
    if(strcmp(to, "is") != 0) continue; //只显示 is 行
    if(strcmp(domain, "localhost") == 0) continue; //要解析的域名是 localhost 跳过
    if(cname_pid > 0 && cname_pid == pid) {
      cname_pid = 0;
      strcpy(domain, cname);
    }
    if(strcmp(domain, domain0) == 0) continue;   //与上次域名不同
    uint32_t sip32 = (uint32_t) (sip[0] << 24) | (sip[1] << 16) | (sip[2] << 8) | sip[3];
    strncpy(domain0, domain, sizeof(domain));
    snprintf(sipstr, sizeof(sipstr), "%d.%d.%d.%d", sip[0], sip[1] ,sip[2], sip[3]);
    ipname = sipstr;
    bool is_route = false;
    for(uint16_t l = 1; l < 512; l++) //查dhcp.lease文件，替换ip为机器名
      if(hostnames[l].ip == sip32) {
        ipname = hostnames[l].name;
        break;
      }
    dip32 = (uint32_t) (dip[0] << 24) | (dip[1] << 16) | (dip[2] << 8) ; //目标地址改成C网段
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
       if(year < 21) {
          memset(skip, ' ', 21 - year);
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
}
