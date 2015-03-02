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
<package>
	<object name="FskStream">
		<object name="Stream" c="xscStreamDestructor">
			<string name="name"/>
			<string name="mime"/>
			<number name="status"/>		<!-- non-zero is bad news -->

			<function name="attach" params="a, b" c="xscStreamAttach" enum="false"/>
			<function name="close" c="xscStreamClose" enum="false"/>
			<function name="detach" c="xscStreamDetach" enum="false"/>
			<function name="flush" c="xscStreamFlush" enum="false"/>
			<function name="open" params="a, b" c="xscStreamOpen" enum="false"/>
			<function name="parse" params="flags" c="xscStreamParse" enum="false"/>
			<function name="readChar" c="xscStreamReadChar" enum="false"/>
			<function name="readLine" c="xscStreamReadLine" enum="false"/>
			<function name="readString" params="length" c="xscStreamReadString" enum="false"/>
			<function name="readChunk" params="size" c="xscStreamReadChunk"/>
			<function name="readCacheDispose" c="xscStreamReadCacheDispose" enum="false"/>
			<function name="reset" c="xscStreamReset" enum="false"/>
			<function name="cr" c="xscStreamReturn" enum="false"/>
			<function name="tab" c="xscStreamTab" enum="false"/>
			<function name="untab" c="xscStreamUntab" enum="false"/>
			<function name="writeChar" params="c" c="xscStreamWriteChar" enum="false"/>
			<function name="writeLine" params="c" c="xscStreamWriteLine" enum="false"/>
			<function name="writeString" params="c" c="xscStreamWriteString" enum="false"/>
			<function name="writeChunk" params="size" c="xscStreamWriteChunk"/>
			<!--function name="get bytesAvailable" c="xscStreamGetBytesAvailable" enum="false"/-->
			<function name="get bytesAvailable" enum="false">
				return this.getBytesAvailable()
			</function>
			<function name="get bytesWritable" enum="false">
				return this.getBytesWritable()
			</function>
			<function name="getBytesAvailable" c="xscStreamGetBytesAvailable" enum="false" script="false"/>
			<function name="getBytesWritable" c="xscStreamGetBytesWritable" enum="false" script="false"/>
			<function name="get bytesWritten" c="xscStreamGetBytesWritten" enum="false"/>
			<function name="seek" params="offset" c="xscStreamSeek" enum="false"/>
			<function name="serialize" params="object" c="xscStreamSerialize" enum="false"/>
			<function name="toString">
				return this.readString();
			</function>
			<function name="toChunk">
				var chunk
				var size = this.bytesAvailable
				if (size > 0) {
					while (size) {
						var c = this.readChunk(size);
						if (!c) break

						size -= c.length

						if (chunk) {
							chunk.append(c) 
							c.free()
						}
						else
							chunk = c
					}
				}
				else {
					while (true) {
						var c = this.readChunk(65536);
						if (!c) break

						if (chunk) {
							chunk.append(c) 
							c.free()
						}
						else
							chunk = c
					}
				}

				return chunk
			</function>
		</object>
	
		<object name="Chunk" prototype="FskStream.Stream">
			<function name="attachData" params="a" c="xscChunkStreamAttachData" enum="false"/>
			<function name="closeData" enum="false">
				if (this.hasOwnProperty("chunk")) {
					this.chunk.free()
					delete this.chunk
				}
			</function>
			<function name="detachData" enum="false">
				delete this.chunk
			</function>
			<function name="openData" params="a" c="xscChunkStreamAttachData" enum="false"/>
			<function name="readData" c="xscChunkStreamReadData" enum="false"/>
			<function name="rewind" c="xscChunkStreamRewind" enum="false"/>
			<function name="writeData" c="xscChunkStreamWriteData" enum="false"/>
			<function name="seek" params="offset" c="xscChunkStreamSeek" enum="false"/>
			<function name="getWriteBuffer" c="xscChunkGetWriteBuffer" enum="false"/>
			<chunk name="chunk" enum="false"/>
		</object>
		
		<object name="File" prototype="FskStream.Stream">
			<string name="filename"/>

			<function name="attachData" params="a" c="xscFileStreamAttachData" enum="false"/>
			<function name="closeData" c="xscFileStreamCloseData" enum="false"/>
			<function name="detachData" c="xscFileStreamDetachData" enum="false"/>
			<function name="openData" params="a, b" c="xscFileStreamOpenData" enum="false"/>
			<function name="readData" c="xscFileStreamReadData" enum="false"/>
			<function name="rewind" c="xscFileStreamRewind" enum="false"/>
			<function name="writeData" c="xscFileStreamWriteData" enum="false"/>
			<function name="seek" params="offset" c="xscFileStreamSeek" enum="false"/>
		</object>
		
		<object name="String" prototype="FskStream.Stream">
			<string name="mime" value="text/plain"/>

			<function name="attachData" params="a" c="xscStringStreamAttachData" enum="false"/>
			<function name="closeData" c="xscStringStreamCloseData" enum="false"/>
			<function name="detachData" c="xscStringStreamDetachData" enum="false"/>
			<function name="openData" params="a, b" c="xscStringStreamOpenData" enum="false"/>
			<function name="readData" c="xscStringStreamReadData" enum="false"/>
			<function name="rewind" c="xscStringStreamRewind" enum="false"/>
			<function name="writeData" c="xscStringStreamWriteData" enum="false"/>
		</object>
		
		<object name="GZip" prototype="FskStream.Stream">
			<function name="attachData" params="source" enum="false">			
				var chunk = source.readChunk(10)
				if ((31 != chunk.peek(0)) || (139 != chunk.peek(1)) || (8 != chunk.peek(2)) || (0 != chunk.peek(3)))
					throw new Error		// not a gz file or not flags we support

				this.source = source
			</function>
			<function name="closeData" c="xscGZipStreamCloseData" enum="false"/>
			<function name="detachData" enum="false">
				delete this.source
			</function>
			<function name="openData" params="source" enum="false">
				this.attachData(source)
			</function>
			<function name="readData" c="xscGZipStreamReadData" enum="false"/>
			<function name="rewind" c="xscGZipStreamRewind" enum="false"/>
			<function name="writeData" enum="false"/>
		</object>

		<object name="Socket" prototype="FskStream.Stream">
			<function name="attachData" params="a" c="xscSocketStreamAttachData" enum="false"/>
			<function name="closeData" c="xscSocketStreamCloseData" enum="false"/>
			<function name="detachData" c="xscSocketStreamDetachData" enum="false"/>
			<function name="openData" params="a, b" c="xscSocketStreamOpenData" enum="false"/>
			<function name="readData" c="xscSocketStreamReadData" enum="false"/>
			<function name="rewind" enum="false">
				throw new Error
			</function>
			<function name="writeData" c="xscSocketStreamWriteData" enum="false"/>
			<function name="setProperties" c="xscSocketStreamSetProperties" params="props" enum="false"/>
		</object>
	</object>
	<object name="Stream">
		<function name="File" c="xscStreamConstructor" prototype="FskStream.File" delete="false" enum="false" set="false"/>
		<function name="String" c="xscStreamConstructor" prototype="FskStream.String" delete="false" enum="false" set="false"/>
		<function name="Socket" c="xscStreamConstructor" prototype="FskStream.Socket" delete="false" enum="false" set="false"/>
		<function name="Chunk" c="xscStreamConstructor" prototype="FskStream.Chunk" delete="false" enum="false" set="false"/>
		<function name="GZip" c="xscStreamConstructor" prototype="FskStream.GZip" delete="false" enum="false" set="false"/>
	</object>
</package>