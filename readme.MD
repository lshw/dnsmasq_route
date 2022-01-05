== 域名分流

安装在openwrt , 配合dnsmasq， 实现通过域名清单来实现路由设置，  
比如有多个链路时， 可以实现， 基于域名清单的流量路由规划。  
可用于负载均衡, 网站加速, 等等。

openwrt中程序设置界面:  
![image](https://github.com/lshw/dnsmasq_route/raw/master/doc/3.jpg?raw=true)

openwrt系统设置中打开日志功能:  
![image](https://github.com/lshw/dnsmasq_route/raw/master/doc/1.jpg?raw=true)

打开dnsmasq的dns请求日志:  
![image](https://github.com/lshw/dnsmasq_route/raw/master/doc/2.jpg?raw=true)
