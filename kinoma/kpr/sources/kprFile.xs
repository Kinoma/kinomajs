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
<package script="true">
	<object name="Files">
		<string name="directoryType" value="directory"/>
		<string name="fileType" value="file"/>
		<string name="linkType" value="link"/>
		
		<function name="get documentsDirectory" c="KPR_Files_get_documentsDirectory"/>
		<function name="get picturesDirectory" c="KPR_Files_get_picturesDirectory"/>
		<function name="get preferencesDirectory" c="KPR_Files_get_preferencesDirectory"/>
		<function name="get temporaryDirectory" c="KPR_Files_get_temporaryDirectory"/>
		
		<function name="deleteDirectory" params="url" c="KPR_Files_deleteDirectory"/>
		<function name="deleteFile" params="url" c="KPR_Files_deleteFile"/>
		<function name="ensureDirectory" params="url" c="KPR_Files_ensureDirectory"/>
		<function name="exists" params="url" c="KPR_Files_exists"/>
		<function name="getInfo" params="url" c="KPR_Files_getInfo"/>
		<function name="getVolumeInfo" params="url" c="KPR_Files_getVolumeInfo"/>
		
		<function name="readChunk" params="url" c="KPR_Files_readChunk"/>
		<function name="readJSON" params="url" c="KPR_Files_readJSON"/>
		<function name="readText" params="url" c="KPR_Files_readText"/>
		<function name="readXML" params="url" c="KPR_Files_readXML"/>
		
		<function name="renameDirectory" params="url, name" c="KPR_Files_renameDirectory"/>
		<function name="renameFile" params="url, name" c="KPR_Files_renameFile"/>
		
		<function name="writeChunk" params="url, data" c="KPR_Files_writeChunk"/>
		<function name="writeJSON" params="url, data" c="KPR_Files_writeJSON"/>
		<function name="writeText" params="url, data" c="KPR_Files_writeText"/>
		<function name="writeXML" params="url, data" c="KPR_Files_writeXML"/>
		
		<function name="appendText" params="url, data" c="KPR_Files_appendText"/>
		<function name="appendChunk" params="url, data" c="KPR_Files_appendChunk"/>
		
		<object name="iterator" c="KPR_Files_iterator">
			<function name="getNext" c="KPR_Files_iterator_getNext"/>
		</object>
		<function name="Iterator" params="url" prototype="Files.iterator" c="KPR_Files_Iterator"/>
		
		<object name="volumeIterator" c="KPR_Files_volume_iterator">
			<function name="getNext" c="KPR_Files_volume_iterator_getNext"/>
		</object>
		<function name="VolumeIterator" prototype="Files.volumeIterator" c="KPR_Files_Volume_Iterator"/>
		
		<object name="directoryNotifier" c="KPR_Files_directoryNotifier">
			<null name="callback"/>
			<function name="close" c="KPR_Files_directoryNotifier_close"/>
		</object>
		<function name="DirectoryNotifier" params="url, callback" prototype="Files.directoryNotifier" c="KPR_Files_DirectoryNotifier"/>
	</object>
</package>
