#
# Copyright (c) 2021 liu shiwei <liushiwei@gmail.com>
#
# This is free software, licensed under the MIT.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=dnsmasq_route
PKG_VERSION:=2026.07.08
PKG_RELEASE:=4

PKG_LICENSE:=GPL3
PKG_LICENSE_FILES:=LICENSE
PKG_MAINTAINER:=liu shiwei

PKG_BUILD_PARALLEL:=1

include $(INCLUDE_DIR)/package.mk

define Package/dnsmasq_route
	SECTION:=net
	CATEGORY:=Network
	TITLE:=dnsmasq_route
	URL:=https://github.com/lshw/dnsmasq_route
	DEPENDS:= +luci-compat
	PKGARCH:=all
endef

define Package/dnsmasq_route/description
	use dnsmasq dns ipset, auto set route.
endef

define Build/Compile
endef

define Package/dnsmasq_route/install
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(CURDIR)/etc/init.d/dnsmasq_route $(1)/etc/init.d/dnsmasq_route

	$(INSTALL_DIR) $(1)/etc/uci-defaults
	$(INSTALL_DIR) $(1)/usr/lib/lua/luci/controller
	$(INSTALL_DIR) $(1)/usr/lib/lua/luci/model/cbi
	$(INSTALL_DIR) $(1)/usr/lib/lua/luci/i18n
	$(INSTALL_DIR) $(1)/etc/config

	$(INSTALL_CONF) $(CURDIR)/files/dnsmasq_route.config $(1)/etc/config/dnsmasq_route
	$(INSTALL_DATA) $(CURDIR)/files/luci/model/dnsmasq_route.lua $(1)/usr/lib/lua/luci/model/cbi/dnsmasq_route.lua
	$(INSTALL_DATA) $(CURDIR)/files/luci/controller/dnsmasq_route.lua $(1)/usr/lib/lua/luci/controller/dnsmasq_route.lua
	$(INSTALL_DATA) $(CURDIR)/files/luci/i18n/dnsmasq_route.zh-cn.po $(1)/usr/lib/lua/luci/i18n/dnsmasq_route.zh-cn.po
endef

$(eval $(call BuildPackage,dnsmasq_route))
