<!--
     Copyright (C) 2010-2015 Marvell International Ltd.
     Copyright (C) 2002-2010 Kinoma, Inc.

     Licensed under the Apache License, Version 2.0 (the "License");
     you may not use this file except in compliance with the License.
     You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

     Unless required by applicable law or agreed to in writing, software
     distributed under the License is distributed on an "AS IS" BASIS,
     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
     See the License for the specific language governing permissions and
     limitations under the License.
-->
ANDROID_OS := $(F_HOME)libraries/android_OS/Jellybean/
ANDROID_BUILD := Jellybean_Build
#INSTALL_PATH := $(F_HOME)/build/android/inNDK/Play/project/libs/armeabi/
MY_LIB := libFskCameraAndroid_java.so
INCLUDEDIRS = -I$(ANDROID_SOURCE)frameworks/native/include/							\
			  -I$(ANDROID_SOURCE)frameworks/av/include/
LIBS =	-lgui


NDK_PLATFORM_VER := 9
ANDROID_SOURCE := $(ANDROID_OS)/myandroid/
ANDROID_LIB := $(ANDROID_OS)/system/lib/

KPRINCLUDE := \
    -I$(KPR_TMP_DIR) \
    -I$(F_HOME)/xs/includes \
    -I$(F_HOME)/core/base \
    -I$(F_HOME)/core/graphics \
    -I$(F_HOME)/core/managers \
    -I$(F_HOME)/core/network \
    -I$(F_HOME)/core/ui \
    -I$(F_HOME)/libraries/QTReader

INCLUDEDIRS +=	-I$(NDK_DIR)/platforms/android-$(NDK_PLATFORM_VER)/arch-arm/usr/include/	\
				-I$(NDK_DIR)/sources/cxx-stl/stlport/stlport/								\
				-I$(ANDROID_SOURCE)frameworks/base/include/									\
				-I$(ANDROID_SOURCE)frameworks/base/native/include/							\
				-I$(ANDROID_SOURCE)hardware/libhardware/include/							\
				-I$(ANDROID_SOURCE)system/core/include/										\
				-I$(F_HOME)kinoma/mediareader/sources/										\
				-I$(F_HOME)kinoma/kinoma-ipp-lib/											\
				-I$(F_HOME)tmp/include/														\
				-I$(F_HOME)tmp/android/														\
				-I$(F_HOME)tools/device_scan/												\

LIBDIRS =	-L$(ANDROID_LIB)														\
            -L$(INSTALL_PATH)				\
			-L$(F_HOME)/build/android/inNDK/Play/project/libs/armeabi/				\
			-L$(NDK_DIR)/platforms/android-$(NDK_PLATFORM_VER)/arch-arm/usr/lib/	\

LIBS +=						\
	-lcamera_client			\
	-lbinder				\
	-lstdc++				\
	-lutils					\
	-llog					\
	-lFsk                   \
    #
    
OBJECTBASE := Java_com_kinoma_$(KPR_APPLICATION)_
CLASSNAME := '"com/kinoma/$(KPR_APPLICATION)/FskCamera"'
   
CXXSOURCES = FskCameraAndroid.cpp camera_jni.cpp jsmn.c               # list of source files
CXXOBJECTS = $(CXXSOURCES:.cpp=.o)  # expands to list of object files
CXXFLAGS = -D__FSK_LAYER__=1 -DESRI_UNIX -O2 -fno-exceptions  -fno-rtti -Wno-multichar $(KPRINCLUDE) $(INCLUDEDIRS) -D$(ANDROID_BUILD) -DANDROID=1 -DSK_BUILD_FOR_ANDROID_NDK=1 -DHAVE_PTHREADS=1 -DCAMERA_JAVA_API -DOBJECTBASE=$(OBJECTBASE) -DCLASSNAME=$(CLASSNAME)
CXX = arm-linux-androideabi-g++


LDFLAGS := $(LIBDIRS) $(LIBS)
LDFLAGS += -shared -Wl

all: $(MY_LIB)
	cp $(MY_LIB) $(INSTALL_PATH)
	@rm -f $(CXXOBJECTS)

$(MY_LIB): $(CXXOBJECTS)
	$(CXX) -o $@ $(CXXOBJECTS) $(LDFLAGS)

FskCameraAndroid.o: FskCameraAndroid.cpp
	$(CXX) $(CXXFLAGS) -c -o FskCameraAndroid.o FskCameraAndroid.cpp

camera_jni.o: camera_jni.cpp
	$(CXX) $(CXXFLAGS) -c -o camera_jni.o camera_jni.cpp

jsmn.o: jsmn.c
	$(CXX) $(CXXFLAGS) -c -o jsmn.o jsmn.c

.PHONY : clean

clean:
	@rm -f $(CXXOBJECTS) $(MY_LIB)


install: $(MY_LIB)
	cp $(MY_LIB) $(INSTALL_PATH)