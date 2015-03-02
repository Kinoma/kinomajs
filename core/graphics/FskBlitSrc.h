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
#if NUM_SRC_FORMATS >= 1
	#define SrcPixelKind		SRC_KIND_0
	#define DstPixelKind		DST_KIND
	#include BLIT_PROTO_FILE
#endif /* NUM_SRC_FORMATS >= 1 */

#if NUM_SRC_FORMATS >= 2
	#define SrcPixelKind		SRC_KIND_1
	#define DstPixelKind		DST_KIND
	#include BLIT_PROTO_FILE
#endif /* NUM_SRC_FORMATS >= 2 */

#if NUM_SRC_FORMATS >= 3
	#define SrcPixelKind		SRC_KIND_2
	#define DstPixelKind		DST_KIND
	#include BLIT_PROTO_FILE
#endif /* NUM_SRC_FORMATS >= 3 */

#if NUM_SRC_FORMATS >= 4
	#define SrcPixelKind		SRC_KIND_3
	#define DstPixelKind		DST_KIND
	#include BLIT_PROTO_FILE
#endif /* NUM_SRC_FORMATS >= 4 */

#if NUM_SRC_FORMATS >= 5
	#define SrcPixelKind		SRC_KIND_4
	#define DstPixelKind		DST_KIND
	#include BLIT_PROTO_FILE
#endif /* NUM_SRC_FORMATS >= 5 */

#if NUM_SRC_FORMATS >= 6
	#define SrcPixelKind		SRC_KIND_5
	#define DstPixelKind		DST_KIND
	#include BLIT_PROTO_FILE
#endif /* NUM_SRC_FORMATS >= 6 */

#if NUM_SRC_FORMATS >= 7
	#define SrcPixelKind		SRC_KIND_6
	#define DstPixelKind		DST_KIND
	#include BLIT_PROTO_FILE
#endif /* NUM_SRC_FORMATS >= 7 */

#if NUM_SRC_FORMATS >= 8
	#define SrcPixelKind		SRC_KIND_7
	#define DstPixelKind		DST_KIND
	#include BLIT_PROTO_FILE
#endif /* NUM_SRC_FORMATS >= 8 */

#if NUM_SRC_FORMATS >= 9
	#define SrcPixelKind		SRC_KIND_8
	#define DstPixelKind		DST_KIND
	#include BLIT_PROTO_FILE
#endif /* NUM_SRC_FORMATS >= 9 */

#if NUM_SRC_FORMATS >= 10
	#define SrcPixelKind		SRC_KIND_9
	#define DstPixelKind		DST_KIND
	#include BLIT_PROTO_FILE
#endif /* NUM_SRC_FORMATS >= 10 */

#if NUM_SRC_FORMATS >= 11
	#define SrcPixelKind		SRC_KIND_10
	#define DstPixelKind		DST_KIND
	#include BLIT_PROTO_FILE
#endif /* NUM_SRC_FORMATS >= 11 */

#if NUM_SRC_FORMATS >= 12
	#define SrcPixelKind		SRC_KIND_11
	#define DstPixelKind		DST_KIND
	#include BLIT_PROTO_FILE
#endif /* NUM_SRC_FORMATS >= 12 */

#if NUM_SRC_FORMATS >= 13
	#define SrcPixelKind		SRC_KIND_12
	#define DstPixelKind		DST_KIND
	#include BLIT_PROTO_FILE
#endif /* NUM_SRC_FORMATS >= 13 */

#if NUM_SRC_FORMATS >= 14
	#define SrcPixelKind		SRC_KIND_13
	#define DstPixelKind		DST_KIND
	#include BLIT_PROTO_FILE
#endif /* NUM_SRC_FORMATS >= 14 */

#if NUM_SRC_FORMATS >= 15
	#define SrcPixelKind		SRC_KIND_14
	#define DstPixelKind		DST_KIND
	#include BLIT_PROTO_FILE
#endif /* NUM_SRC_FORMATS >= 15 */

#if NUM_SRC_FORMATS >= 16
	#define SrcPixelKind		SRC_KIND_15
	#define DstPixelKind		DST_KIND
	#include BLIT_PROTO_FILE
#endif /* NUM_SRC_FORMATS >= 16 */

#if NUM_SRC_FORMATS >= 17
	#define SrcPixelKind		SRC_KIND_16
	#define DstPixelKind		DST_KIND
	#include BLIT_PROTO_FILE
#endif /* NUM_SRC_FORMATS >= 17 */

#if NUM_SRC_FORMATS >= 18
	#define SrcPixelKind		SRC_KIND_17
	#define DstPixelKind		DST_KIND
	#include BLIT_PROTO_FILE
#endif /* NUM_SRC_FORMATS >= 18 */

#if NUM_SRC_FORMATS >= 19
	#define SrcPixelKind		SRC_KIND_18
	#define DstPixelKind		DST_KIND
	#include BLIT_PROTO_FILE
#endif /* NUM_SRC_FORMATS >= 19 */

#if NUM_SRC_FORMATS >= 20
	#define SrcPixelKind		SRC_KIND_19
	#define DstPixelKind		DST_KIND
	#include BLIT_PROTO_FILE
#endif /* NUM_SRC_FORMATS >= 20 */

#if NUM_SRC_FORMATS >= 21
	#error Unexpected large number of source pixel formats.
#endif /* NUM_SRC_FORMATS >= 21 */

#undef DST_KIND

