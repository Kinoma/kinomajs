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
TOOLS = $(F_HOME)/xs6/tools
DEBUG = $(F_HOME)/xs6/bin/linux/debug
RELEASE = $(F_HOME)/xs6/bin/linux/release

debug: $(DEBUG)/kct6

$(DEBUG)/kct6: $(TOOLS)/kct6.js
	echo '#! /usr/bin/env node' > $(DEBUG)/kct6
	cat $(TOOLS)/kct6.js >> $(DEBUG)/kct6
	chmod +x $(DEBUG)/kct6
	
release: $(RELEASE)/kct6

$(RELEASE)/kct6: $(TOOLS)/kct6.js
	echo '#! /usr/bin/env node' > $(RELEASE)/kct6
	cat $(TOOLS)/kct6.js >> $(RELEASE)/kct6
	chmod +x $(RELEASE)/kct6
	
clean:
	rm -rf $(F_HOME)/xs6/bin/linux/debug/kct6
	rm -rf $(F_HOME)/xs6/bin/linux/release/kct6
