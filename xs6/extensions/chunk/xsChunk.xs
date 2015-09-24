<?xml version="1.0" encoding="utf-8"?>
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
<package delete="false" enum="false" set="false" script="true">
	
	<patch prototype="xs">
		<object name="chunk" c="xs_chunk">
			<function name="get length" c="xs_chunk_get_length"/>
			<function name="set length" c="xs_chunk_set_length"/>
			<function name="append" params="chunk" c="xs_chunk_append"/>
			<function name="free" c="xs_chunk_free"/>
			<function name="peek" c="xs_chunk_peek"/>
			<function name="poke" c="xs_chunk_poke"/>
			<function name="slice" c="xs_chunk_slice"/>
			<function name="toString" c="xs_chunk_toString"/>
		</object>
	</patch>
	<function name="Chunk" params="length" prototype="xs.chunk" c="xs_Chunk"/>
	
</package>
