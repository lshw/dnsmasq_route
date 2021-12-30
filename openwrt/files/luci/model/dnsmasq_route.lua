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

enable = s:option(Flag, "enabled", translate("Enable"))
enable.rmempty = false
function enable.cfgvalue(self, section)
	return luci.sys.init.enabled("dnsmsaq_route") and self.enabled or self.disabled
end

local hostname = luci.model.uci.cursor():get_first("system", "system", "hostname")

--dns_server = s:option(Value,"dns_server", translate("dns server"));
--dns_server.rmempty = false
--function dns_server.cfgvalue(


config = s:option(Value, "config", translate("configfile"), translate("This file is /etc/dnsmasq_route.ini."), "")
config.template = "cbi/tvalue"
config.rows = 5
config.wrap = "off"

function config.cfgvalue(self, section)
	return nixio.fs.readfile("/etc/dnsmasq_route.ini")
--	local t = io.popen('dnsmasq_log')
--	local a = t:read("*all")
end

function config.write(self, section, value)
	value = value:gsub("\r\n?", "\n")
	nixio.fs.writefile("/etc/dnsmasq_route.ini", value)
end

function enable.write(self, section, value)
	if value == "1" then
		luci.sys.call("/etc/init.d/dnsmasq_route enable >/dev/null")
		luci.sys.call("/etc/init.d/dnsmasq_route stop >/dev/null")
		luci.sys.call("/etc/init.d/dnsmasq_route start >/dev/null")
		luci.sys.call("/etc/init.d/dnsmasq restart >/dev/null")
	else
		luci.sys.call("/etc/init.d/dnsmasq_route stop >/dev/null")
		luci.sys.call("/etc/init.d/dnsmasq_route disable >/dev/null")
	end
	Flag.write(self, section, value)
end

return m
