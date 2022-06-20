#
# Copyright (c) 2021 liu shiwei <liushiwei@gmail.com>
#
# This is free software, licensed under the MIT.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=dnsmasq_route
PKG_VERSION:=0.20220621
PKG_RELEASE:=V1.2

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/lshw/dnsmasq_route.git
PKG_SOURCE_VERSION:=$(PKG_RELEASE)
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.xz

PKG_LICENSE:=GPL3
PKG_LICENSE_FILES:=LICENSE
PKG_MAINTAINER:=liu shiwei

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

PKG_BUILD_PARALLEL:=1

include $(INCLUDE_DIR)/package.mk

define Package/dnsmasq_route
	SECTION:=net
	CATEGORY:=Network
	TITLE:=dnsmasq_route
	URL:=https://github.com/lshw/dnsmasq_route
	DEPENDS:= +libstdcpp +coreutils-nohup
endef

define Package/dnsmasq_route/description
         use dnsmasq dns forwared log, auto set route.
endef

MAKE_FLAGS += cross

define Build/Prepare
	$(PKG_UNPACK)
	sed -i 's/cc_cross=.*/cc_cross=$(TARGET_CXX)/g' $(PKG_BUILD_DIR)/Makefile
	sed -i 's/^UPDATE=.*/UPDATE=$(PKG_VERSION)/g' $(PKG_BUILD_DIR)/Makefile
	sed -i 's/^GIT_VER=.*/GIT_VER=$(PKG_SOURCE_VERSION)/g' $(PKG_BUILD_DIR)/Makefile
	$(Build/Patch)
endef

define Package/dnsmasq_route/postinst
#!/bin/sh
log_file=$$( uci get system.cfg01e48a.log_file )
if [ "a$${log_name}" = "a" ] ; then
uci set system.cfg01e48a.log_file="/tmp/syslog"
/etc/init.d/log restart
fi

uci set dhcp.cfg01411c.logqueries='1'
uci set dhcp.cfg01411c.quietdhcp='1'
uci commit

/etc/init.d/dnsmasq_route enable
/etc/init.d/dnsmasq_route start
exit 0
endef

define Package/dnsmasq_route/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/dnsmasq_route $(1)/usr/bin/dnsmasq_route
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/dnsmasq_log $(1)/usr/bin/dnsmasq_log
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/dnsmasq_route.sh $(1)/usr/bin/dnsmasq_route.sh

#luci界面
	$(INSTALL_DIR) $(1)/etc/uci-defaults
	$(INSTALL_DIR) $(1)/usr/lib/lua/luci/controller
	$(INSTALL_DIR) $(1)/usr/lib/lua/luci/model/cbi
	$(INSTALL_DIR) $(1)/usr/lib/lua/luci/i18n
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_DIR) $(1)/etc/init.d

	$(INSTALL_BIN) $(PKG_BUILD_DIR)/etc/init.d/dnsmasq_route $(1)/etc/init.d/dnsmasq_route
	$(INSTALL_CONF) $(PKG_BUILD_DIR)/openwrt/files/dnsmasq_route.config $(1)/etc/config/dnsmasq_route
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/openwrt/files/luci/model/dnsmasq_route.lua $(1)/usr/lib/lua/luci/model/cbi/dnsmasq_route.lua
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/openwrt/files/luci/controller/dnsmasq_route.lua $(1)/usr/lib/lua/luci/controller/dnsmasq_route.lua
endef

$(eval $(call BuildPackage,dnsmasq_route))
