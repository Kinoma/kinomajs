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
		<null name="keyringInstance" script="false"/>	<!-- shared instance -->

		<object name="keyringInstanceProto" prototype="Crypt.persistentList">
			<function name="parse" params="b" script="false">
				return(Crypt.keyInfo.keyInfoProto.parse(b));
			</function>

			<function name="serialize" params="o" script="false">
				return Crypt.keyInfo.keyInfoProto.serialize(o);
			</function>
		</object>
		<function name="KeyringInstance" params="" prototype="Crypt.keyringInstanceProto">
			Crypt.PersistentList.call(this);
		</function>

		<object name="keyring" prototype="Crypt.persistentListClient">
			<function name="register" params="keyName, ber, parser, params">
				var key = parser.parse(ber, params);
				key.keyName = keyName;
				this.set(keyName, key);
				return(key);
			</function>
		</object>
		<function name="Keyring" params="keyringInstance" prototype="Crypt.keyring">
			Crypt.PersistentListClient.call(this, keyringInstance ? keyringInstance : Crypt.keyringInstance);
		</function>
	</patch>

	<program>
		Crypt.keyringInstance = new Crypt.KeyringInstance();
	</program>
</package>

