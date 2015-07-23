#
#     Copyright (C) 2010-2015 Marvell International Ltd.
#     Copyright (C) 2002-2010 Kinoma, Inc.
#
#     Licensed under the Apache License, Version 2.0 (the "License");
#     you may not use this file except in compliance with the License.
#     You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#     Unless required by applicable law or agreed to in writing, software
#     distributed under the License is distributed on an "AS IS" BASIS,
#     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#     See the License for the specific language governing permissions and
#     limitations under the License.
#
DEBUG = $(F_HOME)/xs6/bin/linux/gtk/debug
RELEASE = $(F_HOME)/xs6/bin/linux/gtk/release

GTK3_INSTALLED = $(shell expr `pkg-config --modversion gtk+-3.0 | cut -f1 -d.` \>= 3)

ifeq "$(GTK3_INSTALLED)" "1"
    TARGET_DBG += build_debug
    TARGET_RLS += build_release
endif

debug: $(TARGET_DBG)

build_debug:
	cd $(F_HOME)/xs6/xsbug; $(F_HOME)/xs6/bin/linux/debug/kprconfig6 -d -i -m -o $(F_HOME)/xs6 -p linux/gtk
	
release: $(TARGET_RLS)

build_release:
	cd $(F_HOME)/xs6/xsbug; $(F_HOME)/xs6/bin/linux/debug/kprconfig6 -i -m -o $(F_HOME)/xs6 -p linux/gtk
	
clean:
	rm -rf $(F_HOME)/xs6/bin/mac/debug/xsbug.app
	rm -rf $(F_HOME)/xs6/bin/mac/release/xsbug.app
	rm -rf $(F_HOME)/xs6/tmp/mac/debug/xsbug
	rm -rf $(F_HOME)/xs6/tmp/mac/release/xsbug
