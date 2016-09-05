include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/package.mk

PKG_NAME:=miod
PKG_VERSION:=0.0.1
PKG_RELEASE:=1
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

define Package/miod
    SECTION:=utils
	CATEGORY:=Utilities
	DEFAULT:=y
	TITLE:=Mua I/O daemon
	DEPENDS:=+libmraa +libubus +libubox
endef

EXTRA_LDFLAGS += -lmraa -lubus -lubox
TARGET_CFLAGS+= -Wall

define Build/Prepare
	$(Build/Prepare/Default)
	$(CP) ./src/* $(PKG_BUILD_DIR)
endef

define Package/miod/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/btnd $(1)/usr/bin
endef

$(eval $(call BuildPackage,miod))
