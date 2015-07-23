<?xml version="1.0" encoding="UTF-8"?>
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
<package enum="false">
	
	<patch prototype="xs">

		<object name="infoset">
			<object name="attribute">
				<undefined name="_link"/>
				<undefined name="name"/>
				<undefined name="namespace"/>
				<undefined name="parent"/>
				<undefined name="prefix"/>
				<undefined name="value"/>
			</object>
			<object name="cdata">
				<undefined name="_link"/>
				<undefined name="parent"/>
				<undefined name="value"/>
			</object>
			<object name="comment">
				<undefined name="_link"/>
				<undefined name="parent"/>
				<undefined name="value"/>
			</object>
			<object name="document">
				<undefined name="children"/>
				<undefined name="element"/>
				<undefined name="encoding"/>
				<undefined name="version"/>
			</object>
			<object name="element">
				<undefined name="_link"/>
				<undefined name="_attributes"/>
				<undefined name="instance"/>
				<undefined name="name"/>
				<undefined name="namespace"/>
				<undefined name="parent"/>
				<undefined name="prefix"/>
				<undefined name="value"/>
				<undefined name="xmlnsAttributes"/>
				<function name="get attributes">
					return this._attributes;
				</function>
				<function name="set attributes" params="it">
					this._attributes = it;
				</function>
			</object>
			<object name="pi">
				<undefined name="_link"/>
				<undefined name="name"/>
				<undefined name="namespace"/>
				<undefined name="parent"/>
				<undefined name="prefix"/>
				<undefined name="value"/>
			</object>
			<string name="xmlnsNamespace" value="http://www.w3.org/XML/1998/namespace"/>
			<string name="xmlnsPrefix" value="xmlns"/>
			
			<function name="compareAttributes" c="xs_infoset_compareAttributes"/>
			
			<number name="PARSE_DEFAULT" value="0"/>
			<number name="PARSE_NO_ERROR" value="2"/>
			<number name="PARSE_NO_SOURCE" value="1"/>
			<number name="PARSE_NO_WARNING" value="4"/>
			<number name="PARSE_NO_REFERENCE" value="64"/>
			<function name="parse" c="xs_infoset_parse"/>
			
			<number name="PRINT_DEFAULT" value="0"/>
			<number name="PRINT_NO_COMMENT" value="1"/>
			<function name="print" c="xs_infoset_print"/>
			
			<function name="reportError" c="xs_infoset_reportError"/>
			
			<function name="scan" c="xs_infoset_scan"/>
			
			<number name="SERIALIZE_DEFAULT" value="0"/>
			<number name="SERIALIZE_NO_REFERENCE" value="64"/>
			<function name="serialize" c="xs_infoset_serialize"/>
		</object>
		
	</patch>
	
</package>
