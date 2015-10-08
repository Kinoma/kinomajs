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
	<patch prototype="KPR">
		<object name="canvas" prototype="KPR.content">
			<function name="get size" c="KPR_canvas_get_size"/>
			<function name="get width" c="KPR_canvas_get_width"/>
			<function name="get height" c="KPR_canvas_get_height"/>

			<function name="set size" c="KPR_canvas_set_size"/>
			<function name="set width" c="KPR_canvas_set_width"/>
			<function name="set height" c="KPR_canvas_set_height"/>

			<function name="getContext" params="id" c="KPR_canvas_getContext"/>
		</object>

		<object name="canvasRenderingContext2D" c="KPR_canvasRenderingContext2D_destructor">
			<null name="canvas"/>

			<!-- state -->
			<function name="save" params="" c="KPR_canvasRenderingContext2D_save" script="true"/> <!-- push state on state stack -->
			<function name="restore" params="" c="KPR_canvasRenderingContext2D_restore" script="true"/> <!-- pop state stack and restore state -->

			<!-- transformations -->
			<function name="scale" params="x, y" c="KPR_canvasRenderingContext2D_scale" script="true"/>
			<function name="rotate" params="angle" c="KPR_canvasRenderingContext2D_rotate" script="true"/>
			<function name="translate" params="x, y" c="KPR_canvasRenderingContext2D_translate" script="true"/>
			<function name="transform" params="a, b, c, d, e, f" c="KPR_canvasRenderingContext2D_transform" script="true"/>
			<function name="setTransform" params="a, b, c, d, e, f" c="KPR_canvasRenderingContext2D_setTransform" script="true"/>
			<function name="setDeviceTransform" params="a, b, c, d, e, f" c="KPR_canvasRenderingContext2D_setDeviceTransform" script="false"/>

			<!-- compositing -->
			<function name="get globalAlpha" c="KPR_canvasRenderingContext2D_get_globalAlpha" script="true"/>
			<function name="set globalAlpha" params="it" c="KPR_canvasRenderingContext2D_set_globalAlpha" script="true"/>
			<function name="get globalCompositeOperation" c="KPR_canvasRenderingContext2D_get_globalCompositeOperation" script="true"/>
			<function name="set globalCompositeOperation" params="it" c="KPR_canvasRenderingContext2D_set_globalCompositeOperation" script="true"/>

			<!-- colors and styles -->
			<function name="get strokeStyle" c="KPR_canvasRenderingContext2D_get_strokeStyle" script="true"/>
			<function name="set strokeStyle" c="KPR_canvasRenderingContext2D_set_strokeStyle" script="true"/>
			<function name="get fillStyle" c="KPR_canvasRenderingContext2D_get_fillStyle" script="true"/>
			<function name="set fillStyle" c="KPR_canvasRenderingContext2D_set_fillStyle" script="true"/>
			<function name="createLinearGradient" params="x0, y0, x1, y1" script="true">
				var result = Object.create(KPR.canvasLinearGradient);
				result.x0 = x0;
				result.y0 = y0;
				result.x1 = x1;
				result.y1 = y1;
				result.stops = [];
				return result;
			</function>
			<function name="createRadialGradient" params="x0, y0, r0, x1, y1, r1" script="true">
				var result = Object.create(KPR.canvasRadialGradient);
				result.x0 = x0;
				result.y0 = y0;
				result.r0 = r0;
				result.x1 = x1;
				result.y1 = y1;
				result.r1 = r1;
				result.stops = [];
				return result;
			</function>
			<function name="createPattern" params="image, repetition" script="true">
				var result = Object.create(KPR.canvasPattern);
				result.image = image;
				result.repetition = repetition;
				return result;
			</function>

			<!-- line caps/joins -->
			<function name="get lineWidth" c="KPR_canvasRenderingContext2D_get_lineWidth" script="true"/>
			<function name="set lineWidth" params="it" c="KPR_canvasRenderingContext2D_set_lineWidth" script="true"/>
			<function name="get lineCap" c="KPR_canvasRenderingContext2D_get_lineCap" script="true"/>
			<function name="set lineCap" params="it" c="KPR_canvasRenderingContext2D_set_lineCap" script="true"/>
			<function name="get lineJoin" c="KPR_canvasRenderingContext2D_get_lineJoin" script="true"/>
			<function name="set lineJoin" params="it" c="KPR_canvasRenderingContext2D_set_lineJoin" script="true"/>
			<function name="get miterLimit" c="KPR_canvasRenderingContext2D_get_miterLimit" script="true"/>
			<function name="set miterLimit" params="it" c="KPR_canvasRenderingContext2D_set_miterLimit" script="true"/>

			<!-- shadows -->
			<function name="get shadowOffsetX" c="KPR_canvasRenderingContext2D_get_shadowOffsetX" script="true"/>
			<function name="set shadowOffsetX" params="it" c="KPR_canvasRenderingContext2D_set_shadowOffsetX" script="true"/>
			<function name="get shadowOffsetY" c="KPR_canvasRenderingContext2D_get_shadowOffsetY" script="true"/>
			<function name="set shadowOffsetY" params="it" c="KPR_canvasRenderingContext2D_set_shadowOffsetY" script="true"/>
			<function name="get shadowBlur" c="KPR_canvasRenderingContext2D_get_shadowBlur" script="true"/>
			<function name="set shadowBlur" params="it" c="KPR_canvasRenderingContext2D_set_shadowBlur" script="true"/>
			<function name="get shadowColor" c="KPR_canvasRenderingContext2D_get_shadowColor" script="true"/>
			<function name="set shadowColor" params="it" c="KPR_canvasRenderingContext2D_set_shadowColor" script="true"/>

			<!-- rects -->
			<function name="clearRect" params="x, y, w, h" c="KPR_canvasRenderingContext2D_clearRect" script="true"/>
			<function name="fillRect" params="x, y, w, h" c="KPR_canvasRenderingContext2D_fillRect" script="true"/>
			<function name="strokeRect" params="x, y, w, h" c="KPR_canvasRenderingContext2D_strokeRect" script="true"/>

			<!-- path API -->
			<function name="beginPath" params="" c="KPR_canvasRenderingContext2D_beginPath" script="true"/>
			<function name="closePath" params="" c="KPR_canvasRenderingContext2D_closePath" script="true"/>
			<function name="moveTo" params="x, y" c="KPR_canvasRenderingContext2D_moveTo" script="true"/>
			<function name="lineTo" params="x, y" c="KPR_canvasRenderingContext2D_lineTo" script="true"/>
			<function name="quadraticCurveTo" params="cpx, cpy, x, y" c="KPR_canvasRenderingContext2D_quadraticCurveTo" script="true"/>
			<function name="bezierCurveTo" params="cp1x, cp1y, cp2x, cp2y, x, y" c="KPR_canvasRenderingContext2D_bezierCurveTo" script="true"/>
			<function name="arcTo" params="x1, y1, x2, y2, radius" c="KPR_canvasRenderingContext2D_arcTo" script="true"/>
			<function name="rect" params="x, y, w, h" c="KPR_canvasRenderingContext2D_rect" script="true"/>
			<function name="arc" params="x, y, radius, startAngle, endAngle, anticlockwise" c="KPR_canvasRenderingContext2D_arc" script="true"/>
			<function name="fill" params="fillRule" c="KPR_canvasRenderingContext2D_fill" script="true"/>
			<function name="stroke" params="" c="KPR_canvasRenderingContext2D_stroke" script="true"/>
			<function name="clip" params="path, fillRule" c="KPR_canvasRenderingContext2D_clip" script="true"/>
			<function name="resetClip" c="KPR_canvasRenderingContext2D_resetClip" script="true"/>
			<function name="isPointInPath" params="x, y" c="KPR_canvasRenderingContext2D_isPointInPath" script="true"/>
			<function name="isPointInPathStroke" params="x, y" c="KPR_canvasRenderingContext2D_isPointInPathStroke" script="true"/>

			<!-- focus management -->
			<function name="drawFocusRing" params="element, xCaret, yCaret, canDrawCustom" c="KPR_canvasRenderingContext2D_drawFocusRing" script="true"/>

			<!-- text -->
			<function name="get font" c="KPR_canvasRenderingContext2D_get_font" script="true"/>
			<function name="set font" params="it" c="KPR_canvasRenderingContext2D_set_font" script="true"/>
			<function name="get textAlign" c="KPR_canvasRenderingContext2D_get_textAlign" script="true"/>
			<function name="set textAlign" params="it" c="KPR_canvasRenderingContext2D_set_textAlign" script="true"/>
			<function name="get textBaseline" c="KPR_canvasRenderingContext2D_get_textBaseline" script="true"/>
			<function name="set textBaseline" params="it" c="KPR_canvasRenderingContext2D_set_textBaseline" script="true"/>
			<function name="fillText" params="text, x, y, maxWidth" c="KPR_canvasRenderingContext2D_fillText" script="true"/>
			<function name="strokeText" params="text, x, y, maxWidth" c="KPR_canvasRenderingContext2D_strokeText" script="true"/>
			<function name="measureText" params="text" c="KPR_canvasRenderingContext2D_measureText" script="true"/>

			<!-- drawing images -->
			<function name="drawImage" params="image, sx, sy, sw, sh, dx, dy, dw, dh" c="KPR_canvasRenderingContext2D_drawImage" script="true"/>

			<!-- pixel manipulation -->
			<function name="createImageData" params="sw, sh" c="KPR_canvasRenderingContext2D_createImageData" script="true"/>
			<function name="getImageData" params="sx, sy, sw, sh" c="KPR_canvasRenderingContext2D_getImageData" script="true"/>
			<function name="putImageData" params="imagedata, dx, dy, dirtyX, dirtyY, dirtyWidth, dirtyHeight" c="KPR_canvasRenderingContext2D_putImageData" script="true"/>

			<!-- dashes -->
			<function name="get lineDashOffset" c="KPR_canvasRenderingContext2D_get_lineDashOffset" script="true"/>
			<function name="set lineDashOffset" params="dash" c="KPR_canvasRenderingContext2D_set_lineDashOffset" script="true"/>
			<function name="getLineDash" c="KPR_canvasRenderingContext2D_getLineDash" script="true"/>
			<function name="setLineDash" params="segments" c="KPR_canvasRenderingContext2D_setLineDash" script="true"/>

			<!-- hit regions -->
			<function name="addHitRegion" params="options" c="KPR_canvasRenderingContext2D_addHitRegion" script="true"/>
			<function name="removeHitRegion" params="id" c="KPR_canvasRenderingContext2D_removeHitRegion" script="true"/>
			<function name="clearHitRegions" c="KPR_canvasRenderingContext2D_clearHitRegions" script="true"/>
			<!-- Extension functions to Canvas 2.0 for testing -->
			<!--function name="pickHitRegion" params="x, y" c="KPR_canvasRenderingContext2D_pickHitRegion" script="true"/-->

		</object>

		<object name="canvasGradientStop">
			<null name="color"/>
			<number name="offset"/>
		</object>

		<object name="canvasGradient">
			<null name="stops"/>
			<function name="addColorStop" params="offset, color" script="true">
				var stop = Object.create(KPR.canvasGradientStop);
				stop.color = color;
				stop.offset = offset;
				this.stops.push(stop);
			</function>
		</object>

		<object name="canvasLinearGradient" prototype="KPR.canvasGradient">
			<number name="x0"/>
			<number name="y0"/>
			<number name="x1"/>
			<number name="y1"/>
			<function name="setStyle" params="context, stroke" c="KPR_canvasLinearGradient_setStyle"/>
		</object>

		<object name="canvasRadialGradient" prototype="KPR.canvasGradient">
			<number name="x0"/>
			<number name="y0"/>
			<number name="r0"/>
			<number name="x1"/>
			<number name="y1"/>
			<number name="r1"/>
			<function name="setStyle" params="context, stroke" c="KPR_canvasRadialGradient_setStyle"/>
		</object>

		<object name="canvasPattern">
			<null name="image"/>
			<string name="repetition"/>
			<function name="setStyle" params="context, stroke" c="KPR_canvasPattern_setStyle"/>
		</object>

		<object name="textMetrics">
			<number name="width" script="true"/>
		</object>

		<object name="imageData" c="KPR_imageData_destructor">
			<function name="get width" c="KPR_imageData_get_width" script="true"/>
			<function name="get height" c="KPR_imageData_get_height" script="true"/>
			<function name="get data" c="KPR_imageData_get_data" script="true"/>
		</object>

		<!-- new objects in Canvas 2.x -->
		<object name="path2D" c="KPR_path2D" script="true">
			<function name="beginPath" params="" c="KPR_path2D_beginPath" script="true"/>
			<function name="closePath" params="" c="KPR_path2D_closePath" script="true"/>
			<function name="moveTo" params="x, y" c="KPR_path2D_moveTo" script="true"/>
			<function name="lineTo" params="x, y" c="KPR_path2D_lineTo" script="true"/>
			<function name="quadraticCurveTo" params="cpx, cpy, x, y" c="KPR_path2D_quadraticCurveTo" script="true"/>
			<function name="bezierCurveTo" params="cp1x, cp1y, cp2x, cp2y, x, y" c="KPR_path2D_bezierCurveTo" script="true"/>
			<function name="arcTo" params="x1, y1, x2, y2, radius" c="KPR_path2D_arcTo" script="true"/>
			<function name="rect" params="x, y, w, h" c="KPR_path2D_rect" script="true"/>
			<function name="arc" params="x, y, radius, startAngle, endAngle, anticlockwise" c="KPR_path2D_arc" script="true"/>
		</object>
	</patch>

	<function name="Canvas" params="width, height" prototype="KPR.canvas" c="KPR_Canvas"/>
	<function name="Path2D" params="width, height" prototype="KPR.path2D" c="KPR_Path2D"/>
</package>
