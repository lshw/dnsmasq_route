#
# Copyright (c) 2021 liu shiwei <liushiwei@gmail.com>
#
# This is free software, licensed under the MIT.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=dnsmasq_route
PKG_VERSION:=0.0.0.2
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/lshw/dnsmasq_route.git
PKG_SOURCE_VERSION:=3432025
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
	DEPENDS:= +libstdcpp
endef

define Package/dnsmasq_route/description
         use dnsmasq dns forwared log, auto set route.
endef

MAKE_FLAGS += cross

define Build/Prepare
	$(PKG_UNPACK)
	sed -i 's/cc_cross=.*/cc_cross=$(TARGET_CXX)/g' $(PKG_BUILD_DIR)/Makefile
	sed -i '/\gitversion/d' $(PKG_BUILD_DIR)/Makefile
	echo 'const char * const gitversion = "$(PKG_VERSION)";' > $(PKG_BUILD_DIR)/git_version.h
	$(Build/Patch)
endef

define Package/dnsmasq_route/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/dnsmasq_route_cross $(1)/usr/bin/dnsmasq_route
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/dnsmasq_route.sh $(1)/usr/bin/dnsmasq_route.sh
endef

$(eval $(call BuildPackage,dnsmasq_route))
