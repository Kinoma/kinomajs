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
<package script="true">
	<patch prototype="Chunk.prototype">
		<function name="toRawString" params="" c="xs_bin_toRawString"/>
		<function name="setRawString" params="str" c="xs_bin_setRawString"/>
		<function name="peek32" params="i" c="xs_bin_peek32"/>
		<function name="poke32" params="i, v" c="xs_bin_poke32"/>
		<function name="copy" params="offset, chunk" c="xs_bin_copy"/>
		<function name="comp" params="c" c="xs_bin_comp"/>
		<function name="ncomp" params="c, n" c="xs_bin_ncomp"/>
		<function name="getAtom" params="name" c="xs_bin_getAtom"/>
		<function name="putAtom" params="name, value" c="xs_bin_putAtom"/>
	</patch>

	<patch prototype="Crypt">
		<object name="bin">
			<object name="chunk">
				<function name="String" params="rawString" prototype="Chunk.prototype">
					Chunk.call(this);	// set the length later
					this.setRawString(rawString);
				</function>
				<function name="xor" params="c1, c2, c3" c="xs_bin_xor"/>
			</object>

			<object name="atomIterator" c="xs_bin_atomIterator_destructor">
				<function name="next" params="" c="xs_bin_atomIterator_next"/>
				<function name="get name" params="" c="xs_bin_atomIterator_getName"/>
				<function name="get value" params="" c="xs_bin_atomIterator_getValue"/>
			</object>
			<function name="AtomIterator" params="atom" prototype="Crypt.bin.atomIterator" c="xs_bin_atomIterator"/>
		</object>
	</patch>
</package>