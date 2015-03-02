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
NDK_PLATFORM_VER := 9
ANDROID_SOURCE := $(ANDROID_OS)/myandroid/
ANDROID_LIB := $(ANDROID_OS)/system/lib/

INCLUDEDIRS +=			-I$(ANDROID_NDK)/sources/cxx-stl/stlport/stlport/								\
				-I$(ANDROID_SOURCE)frameworks/base/include/									\
				-I$(ANDROID_SOURCE)frameworks/base/native/include/							\
				-I$(ANDROID_SOURCE)hardware/libhardware/include/							\
				-I$(ANDROID_SOURCE)system/core/include/										\
				-I$(F_HOME)kinoma/mediareader/sources/										\
				-I$(F_HOME)kinoma/kinoma-ipp-lib/											\
				-I$(F_HOME)tmp/include/														\
				-I$(F_HOME)tmp/android/														\
				-I$(F_HOME)tools/device_scan/												\
				#
				#-I$(ANDROID_SOURCE)frameworks/native/include/							\
				#-I$(ANDROID_SOURCE)frameworks/av/include/								\

LIBDIRS =	-L$(ANDROID_LIB)														\
			#

LIBS +=						\
	-lcamera_client			\
	-lbinder				\
	-lstdc++				\
	-lutils					\
	-llog					\
	-lFsk					\
	#-lsurfaceflinger_client	\
	#
	
	#-lbinder							\
	#-llog								\
	#-lstagefright						\
	#-lstdc++							\
	#-lutils							\
	#-lFsk								\
	#

CXXSOURCES = FskCameraAndroid.cpp camera_android.cpp	 # list of source files
CXXOBJECTS = $(CXXSOURCES:.cpp=.o)  # expands to list of object files
CXXFLAGS = -D__FSK_LAYER__=1 -DESRI_UNIX -O2 -fno-exceptions  -fno-rtti -Wno-multichar $(INCLUDEDIRS) -D$(ANDROID_BUILD) -DANDROID=1 -DSK_BUILD_FOR_ANDROID_NDK=1 -DHAVE_PTHREADS=1
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

.PHONY : clean

clean:
	@rm -f $(CXXOBJECTS) $(MY_LIB)


install: $(MY_LIB)
	cp $(MY_LIB) $(INSTALL_PATH)