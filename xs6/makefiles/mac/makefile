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
.NOTPARALLEL:

XS6 = $(F_HOME)/xs6
export XS6
MACOS_ARCH = -arch i386
export MACOS_ARCH
MACOS_VERSION_MIN = -mmacosx-version-min=10.7
export MACOS_VERSION_MIN

all: debug release

run:
	make debug -f xsr6.make

debug:
	make debug -f xsc6.make
	make debug -f xsl6.make
	#make debug -f xsd6.make
	make debug -f xsr6.make
	#make debug -f xsrb6.make
	make debug -f tools.make
	make debug -f xsbug.make
	make debug -f kct6.make
	echo '#!/bin/bash\nBASE=$$F_HOME/xs6/bin/mac/debug\n$$BASE/xsr6 -a $$BASE/modules/tools.xsa kprconfig "$$@"' > $(XS6)/bin/mac/debug/kprconfig6
	chmod +x $(XS6)/bin/mac/debug/kprconfig6
	echo '#!/bin/bash\nBASE=$$F_HOME/xs6/bin/mac/debug\n$$BASE/xsr6 -a $$BASE/modules/tools.xsa kpr2js "$$@"' > $(XS6)/bin/mac/debug/kpr2js6
	chmod +x $(XS6)/bin/mac/debug/kpr2js6
	echo '#!/bin/bash\nBASE=$$F_HOME/xs6/bin/mac/debug\n$$BASE/xsr6 -a $$BASE/modules/tools.xsa mcconfig "$$@"' > $(XS6)/bin/mac/debug/mcconfig
	chmod +x $(XS6)/bin/mac/debug/mcconfig
	echo '#!/bin/bash\nBASE=$$F_HOME/xs6/bin/mac/debug\n$$BASE/xsr6 -a $$BASE/modules/tools.xsa xs2js "$$@"' > $(XS6)/bin/mac/debug/xs2js6
	chmod +x $(XS6)/bin/mac/debug/xs2js6
	
release:
	make release -f xsc6.make
	make release -f xsl6.make
	#make release -f xsd6.make
	make release -f xsr6.make
	#make release -f xsrb6.make
	make release -f tools.make
	make release -f xsbug.make
	make release -f kct6.make
	echo '#!/bin/bash\nBASE=$$F_HOME/xs6/bin/mac/release\n$$BASE/xsr6 -a $$BASE/modules/tools.xsa kprconfig "$$@"' > $(XS6)/bin/mac/release/kprconfig6
	chmod +x $(XS6)/bin/mac/release/kprconfig6
	echo '#!/bin/bash\nBASE=$$F_HOME/xs6/bin/mac/release\n$$BASE/xsr6 -a $$BASE/modules/tools.xsa kpr2js "$$@"' > $(XS6)/bin/mac/release/kpr2js6
	chmod +x $(XS6)/bin/mac/release/kpr2js6
	echo '#!/bin/bash\nBASE=$$F_HOME/xs6/bin/mac/release\n$$BASE/xsr6 -a $$BASE/modules/tools.xsa mcconfig "$$@"' > $(XS6)/bin/mac/release/mcconfig
	chmod +x $(XS6)/bin/mac/release/mcconfig
	echo '#!/bin/bash\nBASE=$$F_HOME/xs6/bin/mac/release\n$$BASE/xsr6 -a $$BASE/modules/tools.xsa xs2js "$$@"' > $(XS6)/bin/mac/release/xs2js6
	chmod +x $(XS6)/bin/mac/release/xs2js6
	
clean:
	make clean -f xsc6.make
	make clean -f xsl6.make
	#make clean -f xsd6.make
	make clean -f xsr6.make
	#make clean -f xsrb6.make
	make clean -f tools.make
	make clean -f xsbug.make
	make clean -f kct6.make
	rm -f $(XS6)/bin/mac/debug/kprconfig6
	rm -f $(XS6)/bin/mac/debug/kpr2js6
	rm -f $(XS6)/bin/mac/debug/xs2js6
	rm -f $(XS6)/bin/mac/release/kprconfig6
	rm -f $(XS6)/bin/mac/release/kpr2js6
	rm -f $(XS6)/bin/mac/release/xs2js6
	
