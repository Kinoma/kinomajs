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
	<object name="KPR">
		<function name="dummy">
			var Object, seal, exports;
			var length;
			var x, left, width, right, y, top, height, bottom;
			var id, cache, call, apply, Function;
			var spans, display, align;
			var uri, scheme, authority, path, query, fragment;
			var Grammar, load;
			var a, r, g, b, lines, Chunk, toString;
			var senderCallback, senderTarget, receiverCallback, receiverTarget;
			var w, z, orientation, focalLength;
			var buffer;
			var JSON, stringify;
		</function>
		
		<object name="modules">
		</object>
		
		<object name="events">
			<function name="onAccept" params="content, message"/>
			<function name="onActivated" params="content, activateIt"/>
			<function name="onAdapt" params="content"/>
			<function name="onCancel" params="message"/>
			<function name="onComplete" params="message"/>
			<function name="onCreate" params="content"/>
			<function name="onDispatch" params="content, message"/>
			<function name="onDisplayed" params="content"/>
			<function name="onDisplaying" params="content"/>
			<function name="onDraw" params="content, x, y, width, height"/>
			<function name="onFinished" params="content"/>
			<function name="onFocused" params="content"/>
			<function name="onInvoke" params="content, message"/>
			<function name="onKeyDown" params="content, key, modifiers, count, ticks"/>
			<function name="onKeyUp" params="content, key, modifiers, count, ticks"/>
			<function name="onLaunch" params="content"/>
			<function name="onLoaded" params="content"/>
			<function name="onLoading" params="content"/>
			<function name="onMeasureHorizontally" params="content, width"/>
			<function name="onMeasureVertically" params="content, height"/>
			<function name="onMetadataChanged" params="content"/>
			<function name="onQuit" params="content"/>
			<function name="onQuitting" params="content"/>
			<!-- uncomment these lines if you define SUPPORT_REMOTE_NOTIFICATION as 1
			<function name="onRemoteNotificationRegistered" params="deviceToken, osType"/>
			<function name="onRemoteNotification" params="json"/>
			-->
			<function name="onScrolled" params="content"/>
			<function name="onSensorBegan" params="content, id, ticks"/>
			<function name="onSensorChanged" params="content, id, ticks"/>
			<function name="onSensorEnded" params="content, id, ticks"/>
			<function name="onStateChanged" params="content"/>
			<function name="onTimeChanged" params="content"/>
			<function name="onTouchBegan" params="content, id, x, y, ticks"/>
			<function name="onTouchCancelled" params="content, id"/>
			<function name="onTouchEnded" params="content, id, x, y, ticks"/>
			<function name="onTouchMoved" params="content, id, x, y, ticks"/>
			<function name="onTouchScrolled" params="content, dx, dy, ticks"/>
			<function name="onTransitionBeginning" params="content"/>
			<function name="onTransitionEnded" params="content"/>
			<function name="onUndisplayed" params="content"/>
			<function name="onUnfocused" params="content"/>
		</object>
		
		<object name="behavior">
			<function name="onCreate"/>
		</object>
		
		<object name="transition" c="KPR_transition">
			<null name="parameters"/>
			<function name="get duration" c="KPR_transition_get_duration"/>
			
			<function name="set duration" params="duration" c="KPR_transition_set_duration"/>
			
			<function name="onBegin" params="container"/>
			<function name="onEnd" params="container"/>
			<function name="onStep" params="fraction"/>
		</object>

		<object name="effect" c="KPR_effect">
			<function name="colorize" params="color, opacity" c="KPR_effect_colorize"/>
			<function name="gaussianBlur" params="x, y" c="KPR_effect_gaussianBlur"/>
			<function name="gray" params="dark, lite" c="KPR_effect_gray"/>
			<function name="innerGlow" params="color, opacity, blur, radius" c="KPR_effect_innerGlow"/>
			<function name="innerHilite" params="color, opacity, blur, x, y" c="KPR_effect_innerHilite"/>
			<function name="innerShadow" params="color, opacity, blur, x, y" c="KPR_effect_innerShadow"/>
			<function name="mask" params="texture" c="KPR_effect_mask"/>
			<function name="outerGlow" params="color, opacity, blur, radius" c="KPR_effect_outerGlow"/>
			<function name="outerHilite" params="color, opacity, blur, x, y" c="KPR_effect_outerHilite"/>
			<function name="outerShadow" params="color, opacity, blur, x, y" c="KPR_effect_outerShadow"/>
			<function name="shade" params="texture, opacity" c="KPR_effect_shade"/>
		</object>
		
		<object name="texture" c="KPR_texture">
			<function name="get content" c="KPR_texture_get_content"/>
			<function name="get effect" c="KPR_texture_get_effect"/>
			<function name="get scale" c="KPR_texture_get_scale"/>
			<function name="get size" c="KPR_texture_get_size"/>
			<function name="get width" c="KPR_texture_get_width"/>
			<function name="get height" c="KPR_texture_get_height"/>

			<function name="set effect" params="it" c="KPR_texture_set_effect"/>
			
			<function name="purge" c="KPR_texture_purge"/>
		</object>

		<object name="skin" c="KPR_skin">
			<function name="get borders" c="KPR_skin_get_borders"/>
			<function name="get bounds" c="KPR_skin_get_bounds"/>
			<function name="get fillColors" c="KPR_skin_get_fillColors"/>
			<function name="get margins" c="KPR_skin_get_margins"/>
			<function name="get states" c="KPR_skin_get_states"/>
			<function name="get strokeColors" c="KPR_skin_get_strokeColors"/>
			<function name="get texture" c="KPR_skin_get_texture"/>
			<function name="get tiles" c="KPR_skin_get_tiles"/>
			<function name="get variants" c="KPR_skin_get_variants"/>
			<function name="get width" c="KPR_skin_get_width"/>
			<function name="get height" c="KPR_skin_get_height"/>
		</object>
		
		<object name="sound" c="KPR_sound">
			<function name="play" c="KPR_sound_play"/>
		</object>

		<object name="style" c="KPR_style">
			<function name="get colors" c="KPR_style_get_colors"/>
			<function name="get extra" c="KPR_style_get_extra"/>
			<function name="get font" c="KPR_style_get_font"/>
			<function name="get horizontalAlignment" c="KPR_style_get_horizontalAlignment"/>
			<function name="get indentation" c="KPR_style_get_indentation"/>
			<function name="get lineCount" c="KPR_style_get_lineCount"/>
			<function name="get lineHeight" c="KPR_style_get_lineHeight"/>
			<function name="get margins" c="KPR_style_get_margins"/>
			<function name="get size" c="KPR_style_get_size"/>
			<function name="get verticalAlignment" c="KPR_style_get_verticalAlignment"/>
			
			<function name="set extra" params="it" c="KPR_style_set_extra"/>
			<function name="set font" params="it" c="KPR_style_set_font"/>
			<function name="set horizontalAlignment" params="it" c="KPR_style_set_horizontalAlignment"/>
			<function name="set indentation" params="it" c="KPR_style_set_indentation"/>
			<function name="set lineCount" params="it" c="KPR_style_set_lineCount"/>
			<function name="set lineHeight" params="it" c="KPR_style_set_lineHeight"/>
			<function name="set margins" params="it" c="KPR_style_set_margins"/>
			<function name="set size" params="it" c="KPR_style_set_size"/>
			<function name="set verticalAlignment" params="it" c="KPR_style_set_verticalAlignment"/>
			
			<function name="measure" params="text" c="KPR_style_measure"/>
		</object>
		

		<object name="content" c="KPR_content">
			<function name="get active" c="KPR_content_get_active"/>
			<function name="get backgroundTouch" c="KPR_content_get_backgroundTouch"/>
			<function name="get behavior" c="KPR_content_get_behavior"/>
			<function name="get bounds" c="KPR_content_get_bounds"/>
			<function name="get container" c="KPR_content_get_container"/>
			<function name="get coordinates" c="KPR_content_get_coordinates"/>
			<function name="get duration" c="KPR_content_get_duration"/>
			<function name="get exclusiveTouch" c="KPR_content_get_exclusiveTouch"/>
			<function name="get focused" c="KPR_content_get_focused"/>
			<function name="get fraction" c="KPR_content_get_fraction"/>
			<function name="get index" c="KPR_content_get_index"/>
			<function name="get interval" c="KPR_content_get_interval"/>
			<function name="get multipleTouch" c="KPR_content_get_multipleTouch"/>
			<function name="get name" c="KPR_content_get_name"/>
			<function name="get next" c="KPR_content_get_next"/>
			<function name="get offset" c="KPR_content_get_offset"/>
			<function name="get position" c="KPR_content_get_position"/>
			<function name="get previous" c="KPR_content_get_previous"/>
			<function name="get running" c="KPR_content_get_running"/>
			<function name="get size" c="KPR_content_get_size"/>
			<function name="get skin" c="KPR_content_get_skin"/>
			<function name="get state" c="KPR_content_get_state"/>
			<function name="get style" c="KPR_content_get_style"/>
			<function name="get time" c="KPR_content_get_time"/>
			<function name="get visible" c="KPR_content_get_visible"/>
			<function name="get variant" c="KPR_content_get_variant"/>
			<function name="get x" c="KPR_content_get_x"/>
			<function name="get y" c="KPR_content_get_y"/>
			<function name="get width" c="KPR_content_get_width"/>
			<function name="get height" c="KPR_content_get_height"/>
			
			<function name="set active" params="it" c="KPR_content_set_active"/>
			<function name="set backgroundTouch" params="it" c="KPR_content_set_backgroundTouch"/>
			<function name="set behavior" params="it" c="KPR_content_set_behavior"/>
			<function name="set coordinates" params="it" c="KPR_content_set_coordinates"/>
			<function name="set duration" c="KPR_content_set_duration"/>
			<function name="set exclusiveTouch" params="it" c="KPR_content_set_exclusiveTouch"/>
			<function name="set fraction" c="KPR_content_set_fraction"/>
			<function name="set interval" c="KPR_content_set_interval"/>
			<function name="set multipleTouch" params="it" c="KPR_content_set_multipleTouch"/>
			<function name="set name" params="it" c="KPR_content_set_name"/>
			<function name="set offset" params="it" c="KPR_content_set_offset"/>
			<function name="set position" params="it" c="KPR_content_set_position"/>
			<function name="set size" params="it" c="KPR_content_set_size"/>
			<function name="set skin" params="it" c="KPR_content_set_skin"/>
			<function name="set state" params="it" c="KPR_content_set_state"/>
			<function name="set style" params="it" c="KPR_content_set_style"/>
			<function name="set time" c="KPR_content_set_time"/>
			<function name="set variant" params="it" c="KPR_content_set_variant"/>
			<function name="set visible" params="it" c="KPR_content_set_visible"/>
			<function name="set x" c="KPR_content_set_x"/>
			<function name="set y" c="KPR_content_set_y"/>
			<function name="set width" c="KPR_content_set_width"/>
			<function name="set height" c="KPR_content_set_height"/>
			
			<function name="adjust" params="x, y" c="KPR_content_adjust"/>
			<function name="bubble" params="id" c="KPR_content_bubble"/>
			<function name="cancel" c="KPR_content_cancel"/>
			<function name="captureTouch" params="id, x, y, ticks" c="KPR_content_captureTouch"/>
			<function name="delegate" params="id" c="KPR_content_delegate"/>
			<function name="distribute" params="id" c="KPR_content_distribute"/>
			<function name="focus" c="KPR_content_focus"/>
			<function name="hit" params="x, y" c="KPR_content_hit"/>
			<function name="invoke" params="message, type" c="KPR_content_invoke"/>
			<function name="measure" c="KPR_content_measure"/>
			<function name="moveBy" params="x, y" c="KPR_content_moveBy"/>
			<function name="peek" c="KPR_content_peek"/>
			<function name="sizeBy" params="x, y" c="KPR_content_sizeBy"/>
			<function name="start" c="KPR_content_start"/>
			<function name="stop" c="KPR_content_stop"/>
			<function name="wait" params="duration" c="KPR_content_wait"/>
		</object>
		
		<program c="KPR_content_patch"/>
		
		<object name="container" prototype="KPR.content">
			<function name="get clip" c="KPR_container_get_clip"/>
			<function name="get first" c="KPR_container_get_first"/>
			<function name="get last" c="KPR_container_get_last"/>
			<function name="get length" c="KPR_container_get_length"/>
			<function name="get transitioning" c="KPR_container_get_transitioning"/>		
			<function name="set clip" params="it" c="KPR_container_set_clip"/>
			
			<function name="add" params="content" c="KPR_container_add"/>
			<function name="content" params="index" c="KPR_container_content"/>
			<function name="empty" params="start, stop" c="KPR_container_empty"/>
			<function name="firstThat" params="id" c="KPR_container_firstThat"/>
			<function name="insert" params="content, before" c="KPR_container_insert"/>
			<function name="lastThat" params="id" c="KPR_container_lastThat"/>
			<function name="peek" params="index" c="KPR_container_content"/>
			<function name="remove" params="content" c="KPR_container_remove"/>
			<function name="replace" params="content, by" c="KPR_container_replace"/>
			<function name="run" params="transition" c="KPR_container_run"/>
			<function name="swap" params="content0, content1" c="KPR_container_swap"/>
		</object>
		
		<object name="layer" prototype="KPR.container">
			<function name="get blocking" c="KPR_layer_get_blocking"/>
			<function name="get corners" c="KPR_layer_get_corners"/>
			<function name="get effect" c="KPR_layer_get_effect"/>
			<function name="get opacity" c="KPR_layer_get_opacity"/>
			<function name="get origin" c="KPR_layer_get_origin"/>
			<function name="get rotation" c="KPR_layer_get_rotation"/>
			<function name="get scale" c="KPR_layer_get_scale"/>
			<function name="get skew" c="KPR_layer_get_skew"/>
			<function name="get subPixel" c="KPR_layer_get_subPixel"/>
			<function name="get translation" c="KPR_layer_get_translation"/>
			
			<function name="set blocking" c="KPR_layer_set_blocking"/>
			<function name="set corners" params="it" c="KPR_layer_set_corners"/>
			<function name="set effect" params="it" c="KPR_layer_set_effect"/>
			<function name="set opacity" c="KPR_layer_set_opacity"/>
			<function name="set origin" c="KPR_layer_set_origin"/>
			<function name="set rotation" c="KPR_layer_set_rotation"/>
			<function name="set scale" c="KPR_layer_set_scale"/>
			<function name="set skew" c="KPR_layer_set_skew"/>
			<function name="set subPixel" c="KPR_layer_set_subPixel"/>
			<function name="set translation" c="KPR_layer_set_translation"/>
			
			<function name="attach" params="content" c="KPR_layer_attach"/>
			<function name="capture" params="content" c="KPR_layer_capture"/>
			<function name="detach" c="KPR_layer_detach"/>
			<function name="setResponseJPEG" params="message" c="KPR_layer_setResponseJPEG"/>
			<function name="transform" params="x, y, inverse" c="KPR_layer_transform"/>
			<function name="transformTouch" params="x, y" c="KPR_layer_transformTouch"/>
		</object>
		
		<object name="layout" prototype="KPR.container">
		</object>
		
		<object name="scroller" prototype="KPR.container">
			<function name="get constraint" c="KPR_scroller_get_constraint"/>
			<function name="get loop" c="KPR_scroller_get_loop"/>
			<function name="get scroll" c="KPR_scroller_get_scroll"/>
			<function name="get tracking" c="KPR_scroller_get_tracking"/>
			
			<function name="set loop" params="it" c="KPR_scroller_set_loop"/>
			<function name="set scroll" params="it" c="KPR_scroller_set_scroll"/>
			<function name="set tracking" params="it" c="KPR_scroller_set_tracking"/>
			
			<function name="predictBy" params="dx, dy" c="KPR_scroller_predictBy"/>
			<function name="predictTo" params="x, y" c="KPR_scroller_predictTo"/>
			<function name="reveal" params="bounds" c="KPR_scroller_reveal"/>
			<function name="scrollBy" params="dx, dy" c="KPR_scroller_scrollBy"/>
			<function name="scrollTo" params="x, y" c="KPR_scroller_scrollTo"/>
		</object>
		
		<object name="column" prototype="KPR.container">
		</object>
		<object name="line" prototype="KPR.container">
		</object>
		
		<object name="label" prototype="KPR.content">
			<function name="get editable" c="KPR_label_get_editable"/>
			<function name="get hidden" c="KPR_label_get_hidden"/>
			<function name="get inputType" c="KPR_label_get_inputType"/>
			<function name="get length" c="KPR_label_get_length"/>
			<function name="get selectable" c="KPR_label_get_selectable"/>
			<function name="get selectionBounds" c="KPR_label_get_selectionBounds"/>
			<function name="get selectionOffset" c="KPR_label_get_selectionOffset"/>
			<function name="get selectionLength" c="KPR_label_get_selectionLength"/>
			<function name="get string" c="KPR_label_get_string"/>
			
			<function name="set active" c="KPR_label_set_active"/>
			<function name="set editable" c="KPR_label_set_editable"/>
			<function name="set hidden" c="KPR_label_set_hidden"/>
			<function name="set inputType" params="it" c="KPR_label_set_inputType"/>
			<function name="set selectable" c="KPR_label_set_selectable"/>
			<function name="set string" params="it" c="KPR_label_set_string"/>
			
			<function name="hitOffset" params="x, y" c="KPR_label_hitOffset"/>
			<function name="insert" params="text" c="KPR_label_insert"/>
			<function name="select" params="from, to" c="KPR_label_select"/>
		</object>
		<object name="text" prototype="KPR.content">
			<function name="get editable" c="KPR_text_get_editable"/>
			<function name="get length" c="KPR_text_get_length"/>
			<function name="get selectable" c="KPR_text_get_selectable"/>
			<function name="get selectionBounds" c="KPR_text_get_selectionBounds"/>
			<function name="get selectionOffset" c="KPR_text_get_selectionOffset"/>
			<function name="get selectionLength" c="KPR_text_get_selectionLength"/>
			<function name="get string" c="KPR_text_get_string"/>

			<function name="set active" c="KPR_text_set_active"/>
			<function name="set editable" c="KPR_text_set_editable"/>
			<function name="set selectable" c="KPR_text_set_selectable"/>
			<function name="set string" params="it" c="KPR_text_set_string"/>
			
			<function name="begin" c="KPR_text_begin"/>
			<function name="beginBlock" params="style, behavior" c="KPR_text_beginBlock"/>
			<function name="beginSpan" params="style, behavior" c="KPR_text_beginSpan"/>
			<function name="concat" params="text" c="KPR_text_concat"/>
			<!--function name="concat" params="content, align" c="KPR_text_concat"/-->
			<function name="end" c="KPR_text_end"/>
			<function name="endBlock" c="KPR_text_endBlock"/>
			<function name="endSpan" c="KPR_text_endSpan"/>
			<function name="format" params="spans" c="KPR_text_format"/>
			<function name="hitOffset" params="x, y" c="KPR_text_hitOffset"/>
			<function name="insert" params="text" c="KPR_text_insert"/>
			<function name="select" params="from, to" c="KPR_text_select"/>
		</object>
		
		<object name="picture" prototype="KPR.content">
			<function name="get aspect" c="KPR_picture_get_aspect"/>
			<function name="get corners" c="KPR_layer_get_corners"/>
			<function name="get crop" c="KPR_picture_get_crop"/>
			<function name="get effect" c="KPR_layer_get_effect"/>
			<function name="get opacity" c="KPR_layer_get_opacity"/>
			<function name="get origin" c="KPR_layer_get_origin"/>
			<function name="get ready" c="KPR_picture_get_ready"/>
			<function name="get rotation" c="KPR_layer_get_rotation"/>
			<function name="get scale" c="KPR_layer_get_scale"/>
			<function name="get size" c="KPR_picture_get_size"/>
			<function name="get skew" c="KPR_layer_get_skew"/>
			<function name="get subPixel" c="KPR_layer_get_subPixel"/>
			<function name="get translation" c="KPR_layer_get_translation"/>
			<function name="get url" c="KPR_picture_get_url"/>
			<function name="get width" c="KPR_picture_get_width"/>
			<function name="get height" c="KPR_picture_get_height"/>
			
			<function name="set aspect" c="KPR_picture_set_aspect"/>
			<function name="set corners" params="it" c="KPR_layer_set_corners"/>
			<function name="set crop" params="it" c="KPR_picture_set_crop"/>
			<function name="set effect" params="it" c="KPR_layer_set_effect"/>
			<function name="set opacity" c="KPR_layer_set_opacity"/>
			<function name="set origin" c="KPR_layer_set_origin"/>
			<function name="set rotation" c="KPR_layer_set_rotation"/>
			<function name="set scale" c="KPR_layer_set_scale"/>
			<function name="set skew" c="KPR_layer_set_skew"/>
			<function name="set subPixel" c="KPR_layer_set_subPixel"/>
			<function name="set translation" c="KPR_layer_set_translation"/>
			<function name="set url" params="it" c="KPR_picture_set_url"/>
			
			<function name="load" params="url, mime" c="KPR_picture_load"/>
			<function name="transform" params="x, y, inverse" c="KPR_layer_transform"/>
			<function name="transformTouch" params="x, y" c="KPR_layer_transformTouch"/>
		</object>
		<object name="thumbnail" prototype="KPR.picture">
			<function name="get url" c="KPR_thumbnail_get_url"/>
			
			<function name="set url" params="it" c="KPR_thumbnail_set_url"/>
			
			<function name="load" params="url, mime" c="KPR_thumbnail_load"/>
		</object>
		
		<object name="media" prototype="KPR.content">
			<function name="get aspect" c="KPR_picture_get_aspect"/>
			<function name="get album" c="KPR_media_get_album"/>
			<function name="get artist" c="KPR_media_get_artist"/>
			<function name="get bitRate" c="KPR_media_get_bitRate"/>
			<function name="get duration" c="KPR_media_get_duration"/>
			<function name="get insufficientBandwidth" c="KPR_media_get_insufficientBandwidth"/>
			<function name="get progress" c="KPR_media_get_progress"/>
			<function name="get ready" c="KPR_media_get_ready"/>
			<function name="get seekableFrom" c="KPR_media_get_seekableFrom"/>
			<function name="get seekableTo" c="KPR_media_get_seekableTo"/>
			<function name="get seeking" c="KPR_media_get_seeking"/>
			<function name="get state" c="KPR_media_get_state"/>
			<function name="get time" c="KPR_media_get_time"/>
			<function name="get title" c="KPR_media_get_title"/>
			<function name="get url" c="KPR_media_get_url"/>
			<function name="get volume" c="KPR_media_get_volume"/>

			<function name="hasCover" c="KPR_media_hasCover"/>

			<function name="set aspect" c="KPR_picture_set_aspect"/>
			<function name="set local" c="KPR_media_set_local"/>
			<function name="set seeking" c="KPR_media_set_seeking"/>
			<function name="set state" c="KPR_media_set_state"/>
			<function name="set time" c="KPR_media_set_time"/>
			<function name="set url" params="it" c="KPR_media_set_url"/>
			<function name="set volume" params="it" c="KPR_media_set_volume"/>
			<function name="set bitRate" params="it" c="KPR_media_set_bitRate"/>
			
			<function name="load" params="url, mime" c="KPR_media_load"/>
			<function name="start" c="KPR_media_start"/>
			<function name="stop" c="KPR_media_stop"/>
			<function name="hibernate" c="KPR_media_hibernate"/>
		</object>
		
		<object name="port" prototype="KPR.content">
			<function name="get corners" c="KPR_layer_get_corners"/>
			<function name="get effect" c="KPR_layer_get_effect"/>
			<function name="get opacity" c="KPR_layer_get_opacity"/>
			<function name="get origin" c="KPR_layer_get_origin"/>
			<function name="get rotation" c="KPR_layer_get_rotation"/>
			<function name="get scale" c="KPR_layer_get_scale"/>
			<function name="get skew" c="KPR_layer_get_skew"/>
			<function name="get translation" c="KPR_layer_get_translation"/>
			
			<function name="set corners" params="it" c="KPR_layer_set_corners"/>
			<function name="set effect" params="it" c="KPR_layer_set_effect"/>
			<function name="set opacity" c="KPR_layer_set_opacity"/>
			<function name="set origin" c="KPR_layer_set_origin"/>
			<function name="set rotation" c="KPR_layer_set_rotation"/>
			<function name="set scale" c="KPR_layer_set_scale"/>
			<function name="set skew" c="KPR_layer_set_skew"/>
			<function name="set translation" c="KPR_layer_set_translation"/>
			
			<function name="drawImage" params="image, x, y, width, height, sx, sy, sw, sh" c="KPR_port_drawImage"/>
			<function name="drawLabel" params="string, x, y, width, height" c="KPR_port_drawLabel"/>
			<function name="drawText" params="string, x, y, width, height" c="KPR_port_drawText"/>
			<function name="fillColor" params="color, x, y, width, height"  c="KPR_port_fillColor"/>
			<function name="fillImage" params="image, x, y, width, height, sx, sy, sw, sh" c="KPR_port_fillImage"/>
			<function name="invalidate" c="KPR_port_invalidate"/>
			<function name="intersectClip" params="x, y, width, height" c="KPR_port_intersectClip"/>
			<function name="measureImage" params="image" c="KPR_port_measureImage"/>
			<function name="measureLabel" params="string" c="KPR_port_measureLabel"/>
			<function name="measureText" params="string, width" c="KPR_port_measureText"/>
			<function name="popClip" c="KPR_port_popClip"/>
			<function name="pushClip" c="KPR_port_pushClip"/>
			<function name="reset" c="KPR_port_reset"/>
			<function name="projectImage3D" params="image, billboard, camera" c="KPR_port_projectImage3D"/>
		</object>
		
		<object name="shell" prototype="KPR.container">
			<null name="menus"/>
			<function name="get acceptFiles" c="KPR_shell_get_acceptFiles" script="true"/>
			<function name="get breakOnException" c="KPR_shell_get_breakOnException" script="true"/>
			<function name="get debugging" c="KPR_shell_get_debugging" script="true"/>
			<function name="get id" c="KPR_shell_get_id" script="true"/>
			<function name="get profiling" c="KPR_shell_get_profiling" script="true"/>
			<function name="get profilingDirectory" c="KPR_shell_get_profilingDirectory" script="true"/>
			<function name="get touchMode" c="KPR_shell_get_touchMode" script="true"/>
			<function name="get url" c="KPR_shell_get_url" script="true"/>
			<function name="get windowState" c="KPR_shell_get_windowState" script="true"/>
			<function name="get windowTitle" c="KPR_shell_get_windowTitle" script="true"/>
			
			<function name="set acceptFiles" params="it" c="KPR_shell_set_acceptFiles" script="true"/>
			<function name="set breakOnException" params="it" c="KPR_shell_set_breakOnException" script="true"/>
			<function name="set debugging" params="it" c="KPR_shell_set_debugging" script="true"/>
			<function name="set profiling" params="it" c="KPR_shell_set_profiling" script="true"/>
			<function name="set profilingDirectory" params="it" c="KPR_shell_set_profilingDirectory" script="true"/>
			<function name="set touchMode" params="it" c="KPR_shell_set_touchMode" script="true"/>
			<function name="set windowState" params="it" c="KPR_shell_set_windowState" script="true"/>
			<function name="set windowTitle" params="it" c="KPR_shell_set_windowTitle" script="true"/>
		
			<function name="alert" params="icon, title, message" c="KPR_shell_alert" script="true"/>
			<function name="dump" c="KPR_shell_dump" script="true"/>
			<function name="keyDown" params="key, modifiers, count, ticks" c="KPR_shell_keyDown" script="true"/>
			<function name="keyUp" params="key, modifiers, count, ticks" c="KPR_shell_keyUp" script="true"/>
			<function name="purge" c="KPR_shell_purge" script="true"/>
			<function name="quit" c="KPR_shell_quit" script="true"/>
			<function name="sensorBegan" params="id, ticks" c="KPR_shell_sensorBegan" script="true"/>
			<function name="sensorChanged" params="id, ticks" c="KPR_shell_sensorChanged" script="true"/>
			<function name="sensorEnded" params="id, ticks" c="KPR_shell_sensorEnded" script="true"/>
			<function name="updateMenus" c="KPR_shell_updateMenus" script="true"/>
		</object>
		<object name="host" prototype="KPR.content">
			<function name="get breakOnException" c="KPR_host_get_breakOnException" script="true"/>
			<function name="get debugging" c="KPR_host_get_debugging" script="true"/>
			<function name="get di" c="KPR_host_get_di" script="true"/>
			<function name="get id" c="KPR_host_get_id" script="true"/>
			<function name="get profiling" c="KPR_host_get_profiling" script="true"/>
			<function name="get profilingDirectory" c="KPR_host_get_profilingDirectory" script="true"/>
			<function name="get rotating" c="KPR_host_get_rotating" script="true"/>
			<function name="get url" c="KPR_host_get_url" script="true"/>
			
			<function name="set breakOnException" c="KPR_host_set_breakOnException" script="true"/>
			<function name="set debugging" params="it" c="KPR_host_set_debugging" script="true"/>
			<function name="set profiling" params="it" c="KPR_host_set_profiling" script="true"/>
			<function name="set profilingDirectory" params="it" c="KPR_host_set_profilingDirectory" script="true"/>
			<function name="set rotating" params="it" c="KPR_host_set_rotating" script="true"/>

			<function name="adapt" c="KPR_host_adapt" script="true"/>
			<function name="clearAllBreakpoints" c="KPR_host_clearAllBreakpoints" script="true"/>
			<function name="clearBreakpoint" params="file, line" c="KPR_host_clearBreakpoint" script="true"/>
			<function name="debugger" c="KPR_host_debugger" script="true"/>
			<function name="launch" params="url" c="KPR_host_launch" script="true"/>
			<function name="purge" c="KPR_host_purge"/>
			<function name="quit" c="KPR_host_quit" script="true"/>
			<function name="quitting" c="KPR_host_quitting" script="true"/>
			<function name="setBreakpoint" params="file, line" c="KPR_host_setBreakpoint" script="true"/>
			<function name="trace" params="text" c="KPR_host_trace" script="true"/>
		</object>
		<object name="application" prototype="KPR.container">
			<function name="get container" c="KPR_application_get_container"/>
			<function name="get di" c="KPR_application_get_di"/>
			<function name="get id" c="KPR_application_get_id"/>
			<function name="get size" c="KPR_application_get_size"/>
			<function name="get url" c="KPR_application_get_url"/>
			<function name="get width" c="KPR_application_get_width"/>
			<function name="get height" c="KPR_application_get_height"/>
			
			<function name="dump" c="KPR_shell_dump" script="true"/>
			<function name="measure" c="KPR_application_get_size"/>
			<function name="purge" c="KPR_application_purge"/>
		</object>

		<object name="message" c="KPR_message">
			<function name="get authority" c="KPR_message_get_authority"/>
			<function name="get error" c="KPR_message_get_error"/>
			<function name="get fragment" c="KPR_message_get_fragment"/>
			<function name="get method" c="KPR_message_get_method"/>
			<function name="get name" c="KPR_message_get_name"/>
			<function name="get password" c="KPR_message_get_password"/>
			<function name="get path" c="KPR_message_get_path"/>
			<function name="get priority" c="KPR_message_get_priority"/>
			<function name="get query" c="KPR_message_get_query"/>
			<function name="get requestBuffer" c="KPR_message_get_requestBuffer"/>
			<function name="get requestChunk" c="KPR_message_get_requestChunk"/>
			<function name="get requestText" c="KPR_message_get_requestText"/>
			<function name="get responseBuffer" c="KPR_message_get_responseBuffer"/>
			<function name="get responseChunk" c="KPR_message_get_responseChunk"/>
			<function name="get responseText" c="KPR_message_get_responseText"/>
			<function name="get scheme" c="KPR_message_get_scheme"/>
			<function name="get status" c="KPR_message_get_status"/>
			<function name="get url" c="KPR_message_get_url"/>
			<function name="get user" c="KPR_message_get_user"/>

			<function name="set error" c="KPR_message_set_error"/>
			<function name="set method" c="KPR_message_set_method"/>
			<function name="set priority" c="KPR_message_set_priority"/>
			<function name="set requestBuffer" c="KPR_message_set_requestBuffer"/>
			<function name="set requestChunk" c="KPR_message_set_requestChunk"/>
			<function name="set requestText" c="KPR_message_set_requestText"/>
			<function name="set responseBuffer" c="KPR_message_set_responseBuffer"/>
			<function name="set responseChunk" c="KPR_message_set_responseChunk"/>
			<function name="set responseText" c="KPR_message_set_responseText"/>
			<function name="set status" params="status" c="KPR_message_set_status"/>
		
			<function name="cancel" c="KPR_message_cancel"/>
			<function name="clearRequestHeader" params="name" c="KPR_message_clearRequestHeader"/>
			<function name="clearResponseHeader" params="name" c="KPR_message_clearResponseHeader"/>
			<function name="invoke" c="KPR_message_invoke"/>
			<function name="getRequestHeader" params="name" c="KPR_message_getRequestHeader"/>
			<function name="getResponseHeader" params="name" c="KPR_message_getResponseHeader"/>
			<function name="setRequestCertificate" c="KPR_message_setRequestCertificate"/>
			<function name="setRequestHeader" params="name, value" c="KPR_message_setRequestHeader"/>
			<function name="setResponseHeader" params="name, value" c="KPR_message_setResponseHeader"/>
		</object>
		
		<object name="handler" c="KPR_handler">
			<function name="get behavior" c="KPR_content_get_behavior"/>
			<function name="get message" c="KPR_handler_get_message"/>
			<function name="get path" c="KPR_handler_get_path"/>
			
			<function name="set behavior" params="it" c="KPR_content_set_behavior"/>
			<function name="set duration" c="KPR_content_set_duration"/>
			<function name="set fraction" c="KPR_content_set_fraction"/>
			<function name="set interval" c="KPR_content_set_interval"/>
			<function name="set time" c="KPR_content_set_time"/>

			<function name="download" params="message, url" c="KPR_handler_download"/>
			<function name="invoke" params="message, type" c="KPR_handler_invoke"/>
			<function name="redirect" params="url, mime" c="KPR_handler_redirect"/>
			<function name="upload" params="message, url, at" c="KPR_handler_upload"/>
			<function name="uploadChunk" params="message, chunk" c="KPR_handler_uploadChunk"/>
			<function name="wait" params="duration" c="KPR_handler_wait"/>
		</object>
	</object>
	
	<program c="KPR_patch"/>

	<function name="Behavior" params="content, data, dictionary" prototype="KPR.behavior">
		if (content)
			this.onCreate(content, data, dictionary);
	</function>
	<function name="Transition" params="duration" prototype="KPR.transition" c="KPR_Transition"/>
	
	<function name="Effect" prototype="KPR.effect" c="KPR_Effect"/>
	<function name="Texture" params="url, scale" prototype="KPR.texture" c="KPR_Texture"/>
	<function name="Skin" params="texture, bounds, variants, states, tiles, margins" prototype="KPR.skin" c="KPR_Skin"/>
	<function name="Sound" params="url" prototype="KPR.sound" c="KPR_Sound"/>
	<!-- Sound.volume -->
	<!-- Sound.hibernate -->
	<program c="KPR_Sound_patch"/>
	<function name="Style" params="font, colors, horizontalAlignment, leftMargin, rightMargin, indentation, verticalAlignment, topMargin, bottomMargin, lineHeight, lineCount" prototype="KPR.style" c="KPR_Style"/>

	<function name="Content" params="coordinates, skin, style" prototype="KPR.content" c="KPR_Content"/>
	<!-- Content.combineCoordinates(base, coordinates) -->
	<program c="KPR_Content_patch"/>
	
	<function name="Container" params="coordinates, skin, style" prototype="KPR.container" c="KPR_Container"/>
	<program c="KPR_Container_patch"/>
	
	<function name="Layer" params="coordinates" prototype="KPR.layer" c="KPR_Layer"/>
	
	<function name="Layout" params="coordinates, skin, style" prototype="KPR.layout" c="KPR_Layout"/>
	
	<function name="Scroller" params="coordinates, skin, style" prototype="KPR.scroller" c="KPR_Scroller"/>
	
	<function name="Column" params="coordinates, skin, style" prototype="KPR.column" c="KPR_Column"/>
	<function name="Line" params="coordinates, skin, style" prototype="KPR.line" c="KPR_Line"/>
	
	<function name="Label" params="coordinates, skin, style, string" prototype="KPR.label" c="KPR_Label"/>
	<function name="Text" params="coordinates, skin, style, string" prototype="KPR.text" c="KPR_Text"/>
	
	<function name="Picture" params="coordinates, url, mime" prototype="KPR.picture" c="KPR_Picture"/>
	<function name="Thumbnail" params="coordinates, url, mime" prototype="KPR.thumbnail" c="KPR_Thumbnail"/>
	<!-- Thumbnail.clear(url) -->
	<program c="KPR_Thumbnail_patch"/>
	
	<object name="Event"/>
	<!-- Event.FunctionKeyPlay -->
	<!-- Event.FunctionKeyPause -->
	<!-- Event.FunctionKeyTogglePlayPause -->
	<!-- Event.FunctionKeyPreviousTrack -->
	<!-- Event.FunctionKeyNextTrack -->
	<!-- Event.FunctionKeyBeginSeekingBackward -->
	<!-- Event.FunctionKeyEndSeekingBackward -->
	<!-- Event.FunctionKeyBeginSeekingForward -->
	<!-- Event.FunctionKeyEndSeekingForward -->
	<!-- Event.FunctionKeyHome -->
	<!-- Event.FunctionKeySearch -->
	<!-- Event.FunctionKeyPower -->
	<!-- Event.FunctionKeyMenu -->
	<!-- Event.FunctionKeyVolumeUp -->
	<!-- Event.FunctionKeyVolumeDown -->
	<!-- Event.FunctionKeyMute -->
	<program c="KPR_Event_patch"/>

	<function name="Media" params="coordinates, url, mime" prototype="KPR.media" c="KPR_Media"/>
	<!-- Media.FAILED -->
	<!-- Media.PAUSED -->
	<!-- Media.PLAYING -->
	<!-- Media.WAITING -->
	<!-- Media.canPlayAudio(mime) -->
	<!-- Media.canPlayVideo(mime) -->
	<program c="KPR_Media_patch"/>
	
	<function name="Port" params="coordinates" prototype="KPR.port" c="KPR_Port"/>
	
	<!-- no shell constructor -->
	<function name="Host" params="coordinates, url" prototype="KPR.host" c="KPR_Host" script="true"/>
	<!-- no application constructor -->

	<function name="Message" params="url" prototype="KPR.message" c="KPR_Message"/>
	<!-- Message.BUFFER -->
	<!-- Message.CHUNK -->
	<!-- Message.DOM -->
	<!-- Message.JSON -->
	<!-- Message.TEXT -->
	<!-- Message.URI(path) -->
	<!-- Message.notify(message) -->
	<program c="KPR_Message_patch"/>
	<function name="Handler" params="path" prototype="KPR.handler" c="KPR_Handler"/>
	<!-- Handler.get(url) -->
	<!-- Handler.put(handler) -->
	<!-- Handler.remove(handler) -->
	<program c="KPR_Handler_patch"/>

	<function name="decodeBase64" params="url" c="KPR_decodeBase64"/>
	<function name="encodeBase64" params="url" c="KPR_encodeBase64"/>
	<function name="encodeURIComponentRFC3986" params="url" c="KPR_encodeURIComponentRFC3986"/>
	<function name="getEnvironmentVariable" params="name" c="KPR_getEnvironmentVariable"/>
	<function name="setEnvironmentVariable" params="name, value" c="KPR_setEnvironmentVariable"/>
	
	<function name="launchURI" params="url" c="KPR_launchURI"/>
	<function name="mergeURI" params="base, url" c="KPR_mergeURI"/>
	<function name="parseURI" params="url" c="KPR_parseURI"/>
	<function name="serializeURI" params="url" c="KPR_serializeURI"/>
	
	<function name="parseQuery" params="query"><![CDATA[
		var result = {};
		if (query) {
			var a = query.split("&");
			for (var i = 0, c = a.length; i < c; i++) {
				var b = a[i].split("=");
				result.sandbox[b[0]] = decodeURIComponent(b[1]);
			}
		}
		return result;
	]]></function>
	<function name="serializeQuery" params="query"><![CDATA[
		var result = "";
		var flag = false;
		for (var name in query.sandbox) {
			if (flag)
				result += "&";
			else
				flag = true;
			result += name + "=" + encodeURIComponent(query.sandbox[name]);
		}
		return result;
	]]></function>

	<function name="get controlKey" c="KPR_get_controlKey"/>
	<function name="get optionKey" c="KPR_get_optionKey"/>
	<function name="get shiftKey" c="KPR_get_shiftKey"/>
	<function name="get screenScale" c="KPR_get_screenScale"/>
	<object name="touches" c="KPR_touches">
		<function name="peek" params="id" c="KPR_touches_peek"/>
	</object>
	<object name="system">
		<function name="get SSID" c="KPR_system_get_SSID"/>
		<function name="get volume" c="KPR_system_get_volume"/>
		<function name="set volume" c="KPR_system_set_volume"/>
		<object name="bar" c="KPR_system_bar">
			<function name="get visible" c="KPR_system_bar_get_visible"/>
			<function name="set visible" c="KPR_system_bar_set_visible"/>
		</object>
		<object name="keyboard" c="KPR_system_keyboard">
			<function name="get available" c="KPR_system_keyboard_get_available"/>
			<function name="get visible" c="KPR_system_keyboard_get_visible"/>
			<function name="set visible" c="KPR_system_keyboard_set_visible"/>
		</object>
		<object name="power" c="KPR_system_power_destructor" script="false">
			<function name="close" c="KPR_system_power_close"/>
		</object>
		<function name="Power" params="what" prototype="system.power" c="KPR_system_Power"/>
		<function name="get platform" c="KPR_system_get_platform"/>
		<function name="get settings" c="KPR_system_get_settings"/>
        <function name="set timezone" params="timezone" c="KPR_system_set_timezone"/>
        <function name="set date" params="secsSinceEpoch" c="KPR_system_set_date"/>
	</object>

	<import href="kprHTTP.xs"/>
	<import href="kprCanvas.xs"/>
	<import href="kprStorage.xs"/>
	<import href="kprFile.xs"/>
	<import href="kprDOM.xs"/>	
	<import href="kprTemplates.xs"/>
	<import href="kprTemplatesGrammar.xs"/>
	
	<program c="Math_patch"/>
	<program c="Math3D_patch"/>
</package>

