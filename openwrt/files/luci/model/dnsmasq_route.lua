local m, s

local running=(luci.sys.call("pidof dnsmasq_route > /dev/null") == 0)
if running then
	m = Map("dnsmasq_route", translate("dnsmasq_route config"), translate("dnsmasq_route is running."))
else
	m = Map("dnsmasq_route", translate("dnsmasq_route config"), translate("dnsmasq_route is not running."))
end

s = m:section(TypedSection, "dnsmasq_route", "")
s.addremove = false
s.anonymous = true

enable = s:option(Flag, "enabled", "启用")

s:option(Value, "dns_server", "dns服务器")
s:option(Value, "remote_ip", "路由地址")

--function dns_server.cfgvalue(


config = s:option(Value, "config", "域名表", "", "")
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
        if value == "1" then
                luci.sys.call("/etc/init.d/dnsmasq_route enable >/dev/null")
                luci.sys.call("/etc/init.d/dnsmasq_route start >/dev/null")
                luci.sys.call("/etc/init.d/dnsmasq restart >/dev/null")
        else
                luci.sys.call("/etc/init.d/dnsmasq_route stop >/dev/null")
                luci.sys.call("/etc/init.d/dnsmasq_route disable >/dev/null")
        end
        Flag.write(self, section, value)
end

dnslog = s:option(Value, "dns_log", "最近dns解析清单", "")
dnslog.template = "cbi/tvalue"
dnslog.rows = 10
dnslog.readonly = 1
dnslog.wrap = "off"

function dnslog.cfgvalue(self, section)
	local t = io.popen('dnsmasq_log')
	return t:read("*all")
end

return m
