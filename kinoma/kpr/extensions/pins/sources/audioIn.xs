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
    <patch prototype="PINS.prototypes">
        <object name="audio" c="xs_audio">
            <function name="init" c="xs_audio_init"/>
            <function name="read" c="xs_audio_read"/>
            <function name="write" c="xs_audio_write"/>
            <function name="repeat" params="poller" c="xs_audio_repeat"/>
            <function name="start" c="xs_audio_start"/>
            <function name="stop" c="xs_audio_stop"/>
            <function name="setVolume" c="xs_audio_setVolume"/>
            <function name="close" c="xs_audio_close"/>
        </object>
    </patch>
    <patch prototype="PINS.constructors">
        <function name="Audio" params="it" prototype="PINS.prototypes.audio">
            if (!("sampleRate" in it))
                it = it.sandbox;
            this.sampleRate = it.sampleRate;
            this.channels = it.channels;
            this.direction = it.direction;
            this.realTime = ("realTime" in it) ? it.realTime : false;
        </function>
    </patch>
</package>

