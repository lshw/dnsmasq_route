module("luci.controller.dnsmasq_route", package.seeall)

function index()
	local page
	page = entry({"admin", "services", "dnsmasq_route"}, cbi("dnsmasq_route"), _("dnsmasq_route"), 100)
	page.i18n = "dnsmasq_route"
	page.dependent = true
end
