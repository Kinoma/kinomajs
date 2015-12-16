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
DEBUG = $(F_HOME)/xs6/bin/mac/debug
RELEASE = $(F_HOME)/xs6/bin/mac/release

debug:
	cd $(F_HOME)/xs6/xsbug; $(DEBUG)/xsr6 -a $(DEBUG)/modules/tools.xsa kprconfig -d -i -m -o $(F_HOME)/xs6
	
release:
	cd $(F_HOME)/xs6/xsbug; $(RELEASE)/xsr6 -a $(RELEASE)/modules/tools.xsa kprconfig -m -o $(F_HOME)/xs6
	
clean:
	rm -rf $(F_HOME)/xs6/bin/mac/debug/xsbug.app
	rm -rf $(F_HOME)/xs6/bin/mac/release/xsbug.app
	rm -rf $(F_HOME)/xs6/tmp/mac/debug/xsbug
	rm -rf $(F_HOME)/xs6/tmp/mac/release/xsbug
