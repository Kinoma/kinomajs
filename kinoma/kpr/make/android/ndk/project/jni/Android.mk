#
#     Copyright (C) 2010-2015 Marvell International Ltd.
#     Copyright (C) 2002-2010 Kinoma, Inc.
#
#     Licensed under the Apache License, Version 2.0 (the "License");
#     you may not use this file except in compliance with the License.
#     You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#     Unless required by applicable law or agreed to in writing, software
#     distributed under the License is distributed on an "AS IS" BASIS,
#     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#     See the License for the specific language governing permissions and
#     limitations under the License.
#
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libFsk
LOCAL_SRC_FILES := libFsk/mainHelper.c
OPTIMIZE_FOR_PERFORMANCE_OVER_SIZE := true
BASE_PROJECT_PATH := $(LOCAL_PATH)/../
LOCAL_CFLAGS :=							\
	-g									\
	-frtti								\
	-marm								\
	-march=armv5						\
	-DSUPPORT_XS_DEBUG=$(SUPPORT_XS_DEBUG)		\
	-DTARGET_OS_ANDROID=1				\
	-DOBJECTBASE=$(OBJECTBASE)			\
	-DUSE_JAVACLASS=$(USE_JAVACLASS)	\
	-DHAVE_ENDIAN_H=1					\
	-I$(KPR_TMP_DIR) \
	-I$(F_HOME)/kinoma/kpr/patches	\
	-I$(F_HOME)/core/base	\
	-I$(F_HOME)/core/graphics	\
	-I$(F_HOME)/core/managers	\
	-I$(F_HOME)/core/misc	\
	-I$(F_HOME)/core/network	\
	-I$(F_HOME)/core/ui	\
	-I$(F_HOME)/xs/includes

LOCAL_LDLIBS := -lEGL -lGLESv2 -ldl -llog -L$(F_HOME)/build/android/OSS/lib/ -Wl,--allow-multiple-definition -Wl,--whole-archive $(NDK_PROJECT_PATH)/obj/local/armeabi/libfsk.a -Wl,--no-whole-archive
#LOCAL_LDLIBS := -lEGL -lGLESv2 -ldl -llog -L$(F_HOME)/build/android/OSS/lib/ --whole-archive $(NDK_PROJECT_PATH)/obj/local/armeabi/libfsk.a


LOCAL_EXPORT_LDLIBS := -lFsk
include $(BUILD_SHARED_LIBRARY)


#####
#### This one is for Gingerbread
#####

include $(CLEAR_VARS)

LOCAL_MODULE    := KinomaLibG

PLAY_PROJECT_PATH := $(NDK_PROJECT_PATH)/../
OBJECTBASE := Java_#OBJECT_BASE#_
USE_JAVACLASS := 1
HAVE_SANDISK := 0
OPTIMIZE_FOR_PERFORMANCE_OVER_SIZE := true

LOCAL_SRC_FILES :=	KinomaLibCommon/KinomaLib.c			\
					KinomaLibCommon/KinomaFiles.c		\
					KinomaLibCommon/KinomaInterface.cpp	\
					KinomaLibG/gingerbreadStuff.cpp		\

LOCAL_CFLAGS :=							\
	-g									\
	-marm								\
	-DSUPPORT_XS_DEBUG=$(SUPPORT_XS_DEBUG)		\
	-DTARGET_OS_ANDROID=1				\
	-DOBJECTBASE=$(OBJECTBASE)			\
	-DUSE_JAVACLASS=$(USE_JAVACLASS)	\
	-DHAVE_ENDIAN_H=1					\
	-I$(KPR_TMP_DIR) \
	-I$(F_HOME)/kinoma/kpr/patches	\
	-I$(F_HOME)/core/base					\
	-I$(F_HOME)/core/graphics					\
	-I$(F_HOME)/core/managers					\
	-I$(F_HOME)/core/misc					\
	-I$(F_HOME)/core/network					\
	-I$(F_HOME)/core/ui					\
	-I$(F_HOME)/xs/includes

LOCAL_LDLIBS := -lEGL -lGLESv2 -ldl -llog -L$(F_HOME)/build/android/OSS/lib/ -L$(NDK_PROJECT_PATH)/obj/local/armeabi/ -landroid

# LOCAL_LDLIBS := -lGLESv1_CM -ldl -L$(F_HOME)/build/android/OSS/lib/ -lssl -lcrypto -L$(NDK_PROJECT_PATH)/obj/local/armeabi/ -landroid

LOCAL_SHARED_LIBRARIES := Fsk
include $(BUILD_SHARED_LIBRARY)



#####
#### This one is for Froyo
#####

include $(CLEAR_VARS)

LOCAL_MODULE    := KinomaLibF

OSS := $(F_HOME)/build/android/OSS/

PLAY_PROJECT_PATH := $(NDK_PROJECT_PATH)/../
OBJECTBASE := Java_#OBJECT_BASE#_
USE_JAVACLASS := 1
HAVE_SANDISK := 0
OPTIMIZE_FOR_PERFORMANCE_OVER_SIZE := true

LOCAL_SRC_FILES :=	KinomaLibCommon/KinomaLib.c			\
					KinomaLibCommon/KinomaFiles.c		\
					KinomaLibCommon/KinomaInterface.cpp	\
					KinomaLibF/froyoStuff.cpp			\

LOCAL_CFLAGS :=								\
	-g										\
	-marm									\
	-DSUPPORT_XS_DEBUG=$(SUPPORT_XS_DEBUG)			\
	-DTARGET_OS_ANDROID=1					\
	-DOBJECTBASE=$(OBJECTBASE)				\
	-DUSE_JAVACLASS=$(USE_JAVACLASS)		\
	-DHAVE_ENDIAN_H=1						\
	-I$(KPR_TMP_DIR) \
	-I$(F_HOME)/kinoma/kpr/patches	\
	-I$(F_HOME)/core/base	\
	-I$(F_HOME)/core/graphics	\
	-I$(F_HOME)/core/managers	\
	-I$(F_HOME)/core/misc	\
	-I$(F_HOME)/core/network	\
	-I$(F_HOME)/core/ui	\
	-I$(F_HOME)/xs/includes	\
    -I$(OSS)frameworks/base/include			\
    -I$(OSS)bionic/libc/arch-arm/include	\
    -I$(OSS)bionic/libc/include/			\
    -I$(OSS)bionic/libc/kernel/common/		\
    -I$(OSS)bionic/libc/kernel/arch-arm/	\
    -I$(OSS)bionic/libm/include/			\
    -I$(OSS)system/core/include/			\
    -I$(OSS)external/skia/include/

LOCAL_LDLIBS := -lEGL -lGLESv2 -L$(OSS)/lib/ -lui -ldl -llog -L$(NDK_PROJECT_PATH)/obj/local/armeabi/
# LOCAL_LDLIBS := -lGLESv1_CM -L$(OSS)/lib/ -lui -ldl -llog -lssl -lcrypto -L$(NDK_PROJECT_PATH)/obj/local/armeabi/

LOCAL_SHARED_LIBRARIES := Fsk
include $(BUILD_SHARED_LIBRARY)

