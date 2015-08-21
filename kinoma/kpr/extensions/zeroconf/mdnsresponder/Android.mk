LOCAL_PATH := $(call my-dir)

commonSources := \
    mDNSShared/dnssd_clientlib.c  \
    mDNSShared/dnssd_clientstub.c \
    mDNSShared/dnssd_ipc.c

commonLibs := libcutils liblog

commonFlags := \
    -O2 -g \
    -fno-strict-aliasing \
    -D_GNU_SOURCE \
    -DHAVE_IPV6 \
    -DHAVE_LINUX \
    -DNOT_HAVE_SA_LEN \
    -DPLATFORM_NO_RLIMIT \
    -DTARGET_OS_LINUX \
    -DUSES_NETLINK \
    -DMDNS_DEBUGMSGS=0 \
    -DMDNS_UDS_SERVERPATH=\"/dev/socket/mdnsd\" \
    -DMDNS_USERNAME=\"mdnsr\" \
    -W \
    -Wall \
    -Wextra \
    -Wno-array-bounds \
    -Wno-pointer-sign \
    -Wno-unused \
    -Wno-unused-but-set-variable \
    -Wno-unused-parameter \
    -Werror=implicit-function-declaration \

#########################

include $(CLEAR_VARS)
LOCAL_SRC_FILES :=  mDNSPosix/PosixDaemon.c    \
                    mDNSPosix/mDNSPosix.c      \
                    mDNSPosix/mDNSUNP.c        \
                    mDNSCore/mDNS.c            \
                    mDNSCore/DNSDigest.c       \
                    mDNSCore/uDNS.c            \
                    mDNSCore/DNSCommon.c       \
                    mDNSShared/uds_daemon.c    \
                    mDNSShared/mDNSDebug.c     \
                    mDNSShared/dnssd_ipc.c     \
                    mDNSShared/GenLinkedList.c \
                    mDNSShared/PlatformCommon.c

LOCAL_MODULE := mdnsd
LOCAL_MODULE_TAGS := optional

LOCAL_C_INCLUDES := external/mdnsresponder/mDNSPosix \
                    external/mdnsresponder/mDNSCore  \
                    external/mdnsresponder/mDNSShared

LOCAL_CFLAGS := $(commonFlags)

LOCAL_STATIC_LIBRARIES := $(commonLibs) libc
LOCAL_FORCE_STATIC_EXECUTABLE := true
include $(BUILD_EXECUTABLE)

##########################

include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(commonSources)
LOCAL_MODULE := libmdnssd
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := $(commonFlags)
LOCAL_SYSTEM_SHARED_LIBRARIES := libc
LOCAL_SHARED_LIBRARIES := $(commonLibs)
LOCAL_EXPORT_C_INCLUDE_DIRS := external/mdnsresponder/mDNSShared
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(commonSources)
LOCAL_MODULE := libmdnssd
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := $(commonFlags)
LOCAL_STATIC_LIBRARIES := $(commonLibs)
LOCAL_EXPORT_C_INCLUDE_DIRS := external/mdnsresponder/mDNSShared
include $(BUILD_STATIC_LIBRARY)

############################

include $(CLEAR_VARS)
LOCAL_SRC_FILES := Clients/dns-sd.c Clients/ClientCommon.c
LOCAL_MODULE := dnssd
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := $(commonFlags)
LOCAL_SYSTEM_SHARED_LIBRARIES := libc
LOCAL_SHARED_LIBRARIES := libmdnssd libcutils liblog
include $(BUILD_EXECUTABLE)

############################
# This builds an mDns that is embeddable within GmsCore for the nearby connections API

### STATIC LIB ###
include $(CLEAR_VARS)

LOCAL_SDK_VERSION := 8
LOCAL_MODULE    := libmdns_jni_static
LOCAL_SRC_FILES :=  /mDNSCore/mDNS.c \
                    /mDNSCore/DNSDigest.c \
                    /mDNSCore/uDNS.c \
                    /mDNSCore/DNSCommon.c \
                    /mDNSPosix/mDNSPosix.c \
                    /mDNSPosix/mDNSUNP.c \
                    /mDNSShared/mDNSDebug.c \
                    /mDNSShared/dnssd_clientlib.c \
                    /mDNSShared/dnssd_clientshim.c \
                    /mDNSShared/dnssd_ipc.c \
                    /mDNSShared/GenLinkedList.c \
                    /mDNSShared/PlatformCommon.c

LOCAL_C_INCLUDES := external/mdnsresponder/mDNSPosix \
                    external/mdnsresponder/mDNSCore  \
                    external/mdnsresponder/mDNSShared

LOCAL_CFLAGS += -Os -fvisibility=hidden
LOCAL_CFLAGS += $(commonFlags) \
                -UMDNS_DEBUGMSGS \
                -DMDNS_DEBUGMSGS=0 \
                -DSO_REUSEADDR \
                -DUNICAST_DISABLED

ifeq ($(TARGET_BUILD_TYPE),debug)
  LOCAL_CFLAGS += -O0 -UNDEBUG -fno-omit-frame-pointer
endif

include $(BUILD_STATIC_LIBRARY)
