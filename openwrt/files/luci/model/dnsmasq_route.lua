local m, s

local running=(luci.sys.call("pidof dnsmasq_route > /dev/null") == 0)
if running then
	m = Map("dnsmasq_route", translate("dnsmasq_route config"), translate("域名分流运行中"))
else
	m = Map("dnsmasq_route", translate("dnsmasq_route config"), translate("域名分流未运行"))
end

s = m:section(TypedSection, "dnsmasq_route", "域名分流")
s.addremove = false
s.anonymous = true

enable = s:option(Flag, "enabled", "启用")

s:option(Value, "dns_server", "分流dns服务器","下面的分流域名表所列域名，要到上面这个dns服务器进行解析")
s:option(Value, "remote_ip", "分流路由地址","解析回来的ip为目的地址的流量，将被送到上面这个地址")
s:option(Flag, "ipv6_disable", "禁用ipv6解析地址","IPV6公网地址，不能修改路由，所以最好禁用").rmempty=false
s:option(Flag, "localnet_only", "只路由本地网络","只把源自内网LAN的流量，送到分流路由地址").rmempty=false

config = s:option(Value, "config", "分流域名表", "需要分流的域名清单，最好是二级域名")
config.template = "cbi/tvalue"
config.rows = 10
config.wrap = "off"

function config.cfgvalue(self, section)
	return nixio.fs.readfile("/etc/dnsmasq_route.ini")
end

function config.write(self, section, value)
	value = value:gsub("\r\n?", "\n")
	nixio.fs.writefile("/etc/dnsmasq_route.ini", value)
        luci.sys.call("/etc/init.d/dnsmasq_route restart >/dev/null")
        luci.sys.call("/etc/init.d/dnsmasq restart >/dev/null")
end

function enable.write(self, section, value)
        Flag.write(self, section, value)
        if value == "1" then
                luci.sys.call("/etc/init.d/dnsmasq_route enable >/dev/null")
                luci.sys.call("uci set dhcp.cfg01411c.logqueries='1' >/dev/null")
                luci.sys.call("uci commit >/dev/null");
                luci.sys.call("/etc/init.d/dnsmasq_route restart >/dev/null")
        else
                luci.sys.call("/etc/init.d/dnsmasq_route stop >/dev/null")
                luci.sys.call("/etc/init.d/dnsmasq_route disable >/dev/null")
                luci.sys.call("uci del dhcp.cfg01411c.logqueries >/dev/null")
                luci.sys.call("uci commit >/dev/null")
                luci.sys.call("/etc/init.d/dnsmasq reload >/dev/null")
        end
end

dnslog = s:option(Value, "dns_log", "未分流dns解析清单", "最近10分钟")
dnslog.template = "cbi/tvalue"
dnslog.rows = 10
dnslog.readonly = 1
dnslog.wrap = "off"

function dnslog.cfgvalue(self, section)
	local t = io.popen('dnsmasq_log -V -l 600 | tail -n 200 |sort -r')
	return t:read("*all")
end

return m
