<?xml version="1.0" encoding="UTF-8"?>
<!--
|     Copyright (C) 2002-2015 Kinoma, Inc.
|
|     All rights reserved.
|
|
|
|
|
|
|
|
|
|
|
-->
<package>
	<patch prototype="Priv">
		<object name="timer" c="xs_Timer_destuctor">
			<function name="cancel" params="" c="xs_Timer_cancel" enum="false"/>
			<function name="schedule" params="duration" c="xs_Timer_schedule" enum="false"/>
			<function name="scheduleRepeating" params="duration" c="xs_Timer_scheduleRepeating" enum="false"/>
			<function name="onCallback" params="delta"/>
			<function name="close" c="xs_Timer_close" enum="false"/>
		</object>
	</patch>
	
	<function name="Timer" params="" c="xs_Timer" prototype="Priv.timer" delete="false" enum="false" set="false"/>
</package>