<?xml version="1.0" encoding="UTF-8"?>
<!--
|     Copyright (C) 2010-2015 Marvell International Ltd.
|     Copyright (C) 2002-2010 Kinoma, Inc.
|
|     Licensed under the Apache License, Version 2.0 (the "License");
|     you may not use this file except in compliance with the License.
|     You may obtain a copy of the License at
|
|      http://www.apache.org/licenses/LICENSE-2.0
|
|     Unless required by applicable law or agreed to in writing, software
|     distributed under the License is distributed on an "AS IS" BASIS,
|     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
|     See the License for the specific language governing permissions and
|     limitations under the License.
-->

<makefile>
<!-- Do not indent! Tabs are significant in makefiles... -->


<platform name="mac-cmake">
<common><![CDATA[

#if we're not embedded, don't do this
	set(XSB_PATH ${BUILD_TMP}/${PROJECT_NAME}.xsb)
	add_custom_command(
		OUTPUT ${XSB_PATH}
		COMMAND ${XSC} -b ${DEFAULT_XSC_OPTIONS} ${includes} ${EXT_XS_FILE}
		COMMENT generating ${PROJECT_NAME}.xsb
	)

	add_custom_target(${PROJECT_NAME} ALL DEPENDS ${XSB_PATH})

	install(FILES ${XSB_PATH} DESTINATION ${BUILD_BIN})
]]></common>
</platform>


<platform name="Linux,MacOSX,iPhone,Solaris,android">
<common><![CDATA[
ALL_XSC_OPTIONS = $(DEFAULT_XSC_OPTIONS) $(XSC_INCLUDES) $(XSC_OPTIONS) -b

ifeq "$(FSK_EXTENSION_EMBED)" "true"
build: $(TMP_DIR) $(BIN_DIR)
else
build: $(TMP_DIR) $(BIN_DIR) $(BIN_DIR)/$(PROGRAM).xsb
endif


$(TMP_DIR):
	mkdir -p $(TMP_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BIN_DIR)/$(PROGRAM).xsb: $(TMP_DIR)/$(PROGRAM).xsb
	cp $(TMP_DIR)/$(PROGRAM).xsb $(BIN_DIR)/$(PROGRAM).xsb

$(TMP_DIR)/$(PROGRAM).xsb: $(GRAMMARS)
	$(XSC) $(ALL_XSC_OPTIONS) $(PROGRAM).xs

clean:
	rm -f $(BIN_DIR)/$(PROGRAM).xsb
	rm -f $(TMP_DIR)/$(PROGRAM).*
]]></common>
</platform>



<platform name="Windows">
<common><![CDATA[
ALL_XSC_OPTIONS = $(DEFAULT_XSC_OPTIONS) $(XSC_INCLUDES) $(XSC_OPTIONS) -b

!IF "$(FSK_EXTENSION_EMBED)" == "true"
build: $(TMP_DIR) $(BIN_DIR)
!ELSE
build: $(TMP_DIR) $(BIN_DIR) $(BIN_DIR)\$(PROGRAM).xsb
!ENDIF

$(TMP_DIR):
	if not exist $(TMP_DIR)/$(NULL) mkdir $(TMP_DIR)

$(BIN_DIR):
	if not exist $(BIN_DIR)/$(NULL) mkdir $(BIN_DIR)

$(BIN_DIR)\$(PROGRAM).xsb : $(TMP_DIR)\$(PROGRAM).xsb
	copy $(TMP_DIR)\$(PROGRAM).xsb $(BIN_DIR)\$(PROGRAM).xsb

$(TMP_DIR)\$(PROGRAM).xsb : $(GRAMMARS)
	$(XSC) $(ALL_XSC_OPTIONS) $(PROGRAM).xs

clean:
	del /Q $(BIN_DIR)/$(PROGRAM).xsb
	del /Q $(TMP_DIR)/$(PROGRAM).*
]]></common>
</platform>


</makefile>