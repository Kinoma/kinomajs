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

debug:
	make debug -f xsc6.make
	make debug -f xsl6.make
	#make debug -f xsd6.make
	make debug -f xsr6.make
	#make debug -f xsrb6.make
	make debug -f tools.make
	make debug -f kct6.make
	echo '#!/bin/bash\nBASE=$$F_HOME/xs6/bin/linux/debug\n$$BASE/xsr6 -a $$BASE/modules/tools.xsa kprconfig "$$@"' > $(XS6)/bin/linux/debug/kprconfig6
	chmod +x $(XS6)/bin/linux/debug/kprconfig6
	echo '#!/bin/bash\nBASE=$$F_HOME/xs6/bin/linux/debug\n$$BASE/xsr6 -a $$BASE/modules/tools.xsa kpr2js "$$@"' > $(XS6)/bin/linux/debug/kpr2js6
	chmod +x $(XS6)/bin/linux/debug/kpr2js6
	echo '#!/bin/bash\nBASE=$$F_HOME/xs6/bin/linux/debug\n$$BASE/xsr6 -a $$BASE/modules/tools.xsa xs2js "$$@"' > $(XS6)/bin/linux/debug/xs2js6
	chmod +x $(XS6)/bin/linux/debug/xs2js6
#	make debug -f xsbug.make
	
release:
	make release -f xsc6.make
	make release -f xsl6.make
	#make release -f xsd6.make
	make release -f xsr6.make
	#make release -f xsrb6.make
	make release -f tools.make
	make release -f kct6.make
	echo '#!/bin/bash\nBASE=$$F_HOME/xs6/bin/linux/release\n$$BASE/xsr6 -a $$BASE/modules/tools.xsa kprconfig "$$@"' > $(XS6)/bin/linux/release/kprconfig6
	chmod +x $(XS6)/bin/linux/release/kprconfig6
	echo '#!/bin/bash\nBASE=$$F_HOME/xs6/bin/linux/release\n$$BASE/xsr6 -a $$BASE/modules/tools.xsa kpr2js "$$@"' > $(XS6)/bin/linux/release/kpr2js6
	chmod +x $(XS6)/bin/linux/release/kpr2js6
	echo '#!/bin/bash\nBASE=$$F_HOME/xs6/bin/linux/release\n$$BASE/xsr6 -a $$BASE/modules/tools.xsa xs2js "$$@"' > $(XS6)/bin/linux/release/xs2js6
	chmod +x $(XS6)/bin/linux/release/xs2js6
#	make release -f xsbug.make
	
clean:
	make clean -f xsc6.make
	make clean -f xsl6.make
	#make clean -f xsd6.make
	make clean -f xsr6.make
	#make clean -f xsrb6.make
	make clean -f tools.make
	make clean -f kct6.make
	rm -f $(XS6)/bin/linux/debug/kprconfig6
	rm -f $(XS6)/bin/linux/debug/kpr2js6
	rm -f $(XS6)/bin/linux/debug/xs2js6
	rm -f $(XS6)/bin/linux/release/kprconfig6
	rm -f $(XS6)/bin/linux/release/kpr2js6
	rm -f $(XS6)/bin/linux/release/xs2js6
#	make clean -f xsbug.make
