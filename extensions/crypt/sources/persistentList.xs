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
<package>
	<patch prototype="Crypt">
		<object name="persistentList" c="xs_persistentList_destructor">
			<function name="set" params="name, o, client" c="xs_persistentList_set"/>
			<function name="get" params="name, client" c="xs_persistentList_get"/>
			<function name="nth" params="n, client" c="xs_persistentList_nth"/>
			<function name="remove" params="name, client" c="xs_persistentList_remove"/>
			<function name="push" params="n, client" c="xs_persistentList_push"/>
			<function name="get length" params="" c="xs_persistentList_getLength"/>

			<function name="sync" params="client" c="xs_persistentList_sync" script="false"/>

			<!-- default parse/serializer -->
			<function name="parse" params="b" script="false">
				return(xs.parse(b.toRawString()));
			</function>

			<function name="serialize" params="o" script="false">
				return(new Crypt.bin.chunk.String(xs.serialize(o)));
			</function>
		</object>
		<function name="PersistentList" prototype="Crypt.persistentList" c="xs_persistentList_constructor"/>

		<object name="persistentListClient">
			<null name="persistentListInstance" script="false"/>
			<null name="cache" script="false"/>
			<number name="version" value="0" script="false"/>

			<function name="set" params="name, o">
				return(this.persistentListInstance.set(name, o, this));
			</function>

			<function name="get" params="name">
				return(this.persistentListInstance.get(name, this));
			</function>

			<function name="nth" params="n">
				return(this.persistentListInstance.nth(n, this));
			</function>

			<function name="remove" params="name">
				return(this.persistentListInstance.remove(name, this));
			</function>

			<function name="push" params="n">
				return(this.persistentListInstance.push(n, this));
			</function>

			<function name="getList" params="">
				this.persistentListInstance.sync(this);
				return(this.cache);
			</function>

			<function name="get length" params="">
				return(this.persistentListInstance.length);
			</function>
		</object>
		<function name="PersistentListClient" params="instance" prototype="Crypt.persistentListClient">
			this.persistentListInstance = instance;
			this.cache = new Array();
			this.version = 0;
		</function>
	</patch>
</package>