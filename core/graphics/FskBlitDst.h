/*
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
*/
#if NUM_DST_FORMATS >= 1
	#define SrcPixelKind		SRC_KIND
	#define DstPixelKind		DST_KIND_0
	#include BLIT_PROTO_FILE
#endif /* NUM_DST_FORMATS >= 1 */

#if NUM_DST_FORMATS >= 2
	#define SrcPixelKind		SRC_KIND
	#define DstPixelKind		DST_KIND_1
	#include BLIT_PROTO_FILE
#endif /* NUM_DST_FORMATS >= 2 */

#if NUM_DST_FORMATS >= 3
	#define SrcPixelKind		SRC_KIND
	#define DstPixelKind		DST_KIND_2
	#include BLIT_PROTO_FILE
#endif /* NUM_DST_FORMATS >= 3 */

#if NUM_DST_FORMATS >= 4
	#define SrcPixelKind		SRC_KIND
	#define DstPixelKind		DST_KIND_3
	#include BLIT_PROTO_FILE
#endif /* NUM_DST_FORMATS >= 4 */

#if NUM_DST_FORMATS >= 5
	#define SrcPixelKind		SRC_KIND
	#define DstPixelKind		DST_KIND_4
	#include BLIT_PROTO_FILE
#endif /* NUM_DST_FORMATS >= 5 */

#if NUM_DST_FORMATS >= 6
	#define SrcPixelKind		SRC_KIND
	#define DstPixelKind		DST_KIND_5
	#include BLIT_PROTO_FILE
#endif /* NUM_DST_FORMATS >= 6 */

#if NUM_DST_FORMATS >= 7
	#define SrcPixelKind		SRC_KIND
	#define DstPixelKind		DST_KIND_6
	#include BLIT_PROTO_FILE
#endif /* NUM_DST_FORMATS >= 7 */

#if NUM_DST_FORMATS >= 8
	#define SrcPixelKind		SRC_KIND
	#define DstPixelKind		DST_KIND_7
	#include BLIT_PROTO_FILE
#endif /* NUM_DST_FORMATS >= 8 */

#if NUM_DST_FORMATS >= 9
	#define SrcPixelKind		SRC_KIND
	#define DstPixelKind		DST_KIND_8
	#include BLIT_PROTO_FILE
#endif /* NUM_DST_FORMATS >= 9 */

#if NUM_DST_FORMATS >= 10
	#define SrcPixelKind		SRC_KIND
	#define DstPixelKind		DST_KIND_9
	#include BLIT_PROTO_FILE
#endif /* NUM_DST_FORMATS >= 10 */

#if NUM_DST_FORMATS >= 11
	#define SrcPixelKind		SRC_KIND
	#define DstPixelKind		DST_KIND_10
	#include BLIT_PROTO_FILE
#endif /* NUM_DST_FORMATS >= 11 */

#if NUM_DST_FORMATS >= 12
	#define SrcPixelKind		SRC_KIND
	#define DstPixelKind		DST_KIND_11
	#include BLIT_PROTO_FILE
#endif /* NUM_DST_FORMATS >= 12 */

#if NUM_DST_FORMATS >= 13
	#define SrcPixelKind		SRC_KIND
	#define DstPixelKind		DST_KIND_12
	#include BLIT_PROTO_FILE
#endif /* NUM_DST_FORMATS >= 13 */

#if NUM_DST_FORMATS >= 14
	#define SrcPixelKind		SRC_KIND
	#define DstPixelKind		DST_KIND_13
	#include BLIT_PROTO_FILE
#endif /* NUM_DST_FORMATS >= 14 */

#if NUM_DST_FORMATS >= 15
	#define SrcPixelKind		SRC_KIND
	#define DstPixelKind		DST_KIND_14
	#include BLIT_PROTO_FILE
#endif /* NUM_DST_FORMATS >= 15 */

#if NUM_DST_FORMATS >= 16
	#define SrcPixelKind		SRC_KIND
	#define DstPixelKind		DST_KIND_15
	#include BLIT_PROTO_FILE
#endif /* NUM_DST_FORMATS >= 16 */

#if NUM_DST_FORMATS >= 17
	#error Unexpected large number of destination pixel formats
#endif /* NUM_DST_FORMATS >= 17 */

#ifdef SRC_KIND
	#undef SRC_KIND
#endif /* SRC_KIND */

