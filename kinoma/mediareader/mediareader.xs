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
	<import href="FskCore.xs" link="dynamic"/>

<target name="KPR_CONFIG != true">
	
	<patch prototype="Media">
		<object name="Reader">
			<object name="reader" c="xs_mediareader_destructor" script="false">
				<function name="close" c="xs_mediareader_close"/>

				<function name="set" params="property, value" c="xs_mediareader_set"/>
				<function name="get" params="property" c="xs_mediareader_get"/>
				<function name="has" params="property" c="xs_mediareader_has"/>		
				<function name="start" c="xs_mediareader_start"/>
				<function name="stop" c="xs_mediareader_stop"/>

				<function name="extract" c="xs_mediareader_extract"/>								<!-- returns Media.Reader.data -->

				<function name="getMetadata" c="xs_mediareader_getMetadata"/>

				<function name="getTrack" params="index">												// returns Media.Reader.track
					if (index < this.tracks.length)
						return this.tracks[index]
					return null
				</function>

				<function name="onStateChange"/>
				
				<null name="tracks" script="false"/>
			</object>
			
			<function name="canHandle" params="mime" c="xs_mediareader_canHandle" enum="false"/>
			
			<function name="URL" params="url, mime" c="xs_mediareader_URL" prototype="Media.Reader.reader"/>
			<function name="Stream" params="stream, mime" c="xs_mediareader_Stream" prototype="Media.Reader.reader"/>		

			<object name="track" c="xs_mediareadertrack_destructor" script="false">
				<function name="set" params="property, value" c="xs_mediareadertrack_set"/>
				<function name="get" params="property" c="xs_mediareadertrack_get"/>		
				<function name="has" params="property" c="xs_mediareadertrack_has"/>		
			</object>

			<object name="sampleInfo" script="false">
				<number name="samples" value="1"/>
				<number name="sampleSize"/>
				<number name="flags" value="1"/>					<!-- key frame by default-->
				<number name="decodeTime"/>
				<undefined name="sampleDuration"/>
				<undefined name="compositionTime"/>
			</object>
<!--
			<function name="SampleInfo" params="samples, sampleSize, flags, decodeTime, sampleDuration, compositionTime" prototype="Media.Reader.sampleInfo">
				this.samples = samples
				this.sampleSize = sampleSize
				this.flags = flags
				this.decodeTime = decodeTime
				if (undefined != sampleDuration)
					this.sampleDuration = sampleDuration
				if (undefined != compositionTime)
					this.compositionTime = compositionTime
			</function>
-->

			<object name="data" script="false">
				<null name="chunk"/>									<!-- Chunk -->
				<null name="info"/>									<!-- array of Media.Reader.sampleInfo -->
				<null name="track"/>									<!-- Media.Trader.track -->
			</object>

			<function name="Bitmap" params="chunk, format" c="xs_media_reader_bitmap" prototype="FskUI.Bitmap"/>
		</object>
	</patch>
	
</target>

<target name="KPR_CONFIG == true">

	<object name="fskMediaReader" c="xs_mediareader_destructor">
		<function name="close" c="xs_mediareader_close" script="true"/>

		<function name="set" params="property, value" c="xs_mediareader_set" script="true"/>
		<function name="get" params="property" c="xs_mediareader_get" script="true"/>
		<function name="has" params="property" c="xs_mediareader_has" script="true"/>		
		<function name="start" c="xs_mediareader_start" script="true"/>
		<function name="stop" c="xs_mediareader_stop" script="true"/>

		<function name="extract" c="xs_mediareader_extract" script="true"/> <!-- returns fskMediaReaderData -->

		<function name="getMetadata" c="xs_mediareader_getMetadata" script="true"/>

		<function name="getTrack" params="index" script="true">				// returns fskMediaReaderTrack
			if (index < this.tracks.length)
				return this.tracks[index]
			return null
		</function>

        <function name="onStateChange"/>
        <function name="onMediaDataArrived" script="true"/>

		<null name="tracks"/>
	</object>
	
	<function name="FskMediaReader" params="url, mime" c="xs_mediareader_URL" prototype="fskMediaReader" script="true"/>
	<!-- FskMediaReader.canHandle(message) -->
	<program c="FskMediaReader_patch"/>
	
	<object name="FskMediaProperty" script="true"/>
	<!-- FskMediaProperty.* -->
	<program c="FskMediaProperty_patch"/>

	<object name="fskMediaReaderTrack" c="xs_mediareadertrack_destructor">
		<function name="set" params="property, value" c="xs_mediareadertrack_set" script="true"/>
		<function name="get" params="property" c="xs_mediareadertrack_get" script="true"/>		
		<function name="has" params="property" c="xs_mediareadertrack_has" script="true"/>		
	</object>

	<object name="fskMediaReaderSampleInfo">
		<number name="samples" value="1" script="true"/>
		<number name="sampleSize" script="true"/>
		<number name="flags" value="1" script="true"/>						<!-- key frame by default-->
		<number name="decodeTime" script="true"/>
		<undefined name="sampleDuration" script="true"/>
		<undefined name="compositionTime" script="true"/>
	</object>

	<object name="fskMediaReaderData">
		<null name="chunk" script="true"/>									<!-- Chunk -->
		<null name="info" script="true"/>									<!-- array of fskMediaReaderSampleInfo -->
		<null name="track" script="true"/>									<!-- fskMediaReaderTrack -->
	</object>
	
</target>

</package>