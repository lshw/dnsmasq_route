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

localnet = s:option(Flag, "localnet_only", "只路由本地网络","只把源自内网LAN的流量，送到分流路由地址")
localnet.rmempty = false

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
        luci.sys.call("/etc/init.d/dnsmasq_route start >/dev/null")
end

function localnet.write(self, section, value)
        Flag.write(self, section, value)
        luci.sys.call("/etc/init.d/dnsmasq_route start >/dev/null")
end

return m
