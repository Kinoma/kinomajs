/*
 *     Copyright (C) 2010-2015 Marvell International Ltd.
 *     Copyright (C) 2002-2010 Kinoma, Inc.
 *
 *     Licensed under the Apache License, Version 2.0 (the "License");
 *     you may not use this file except in compliance with the License.
 *     You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */
#ifndef __FSK_ERRORS__
#define __FSK_ERRORS__

#ifdef __cplusplus
extern "C" {
#endif

/*
	Errors
*/

typedef enum {
	kFskErrNone					= 0,
	kFskErrMemFull				= -1,
	kFskErrOutOfSequence		= -2,
	kFskErrBadState				= -3,
	kFskErrOperationFailed		= -5,
	kFskErrIteratorComplete		= -6,
	kFskErrInvalidParameter		= -7,
	kFskErrScript				= -8,
	kFskErrUnimplemented		= -9,
	kFskErrUnsupportedPixelType	= -10,
	kFskErrDuplicateElement		= -11,
	kFskErrUnknownElement		= -12,
	kFskErrBadData				= -13,
	kFskErrShutdown				= -14,
	kFskErrIsBusy				= -15,
	kFskErrNotFound				= -16,
	kFskErrOperationCancelled	= -17,
	kFskErrExtensionNotFound	= -18,
	kFskErrCodecNotFound		= -19,
	kFskErrAudioOutReset		= -20,

	kFskErrCannotDecrypt		= -400,

	kFskErrUnknown				= -9999,

    // FILES

	kFskErrTooManyOpenFiles		= -40,
	kFskErrFileNotFound			= -41,
	kFskErrFilePermissions		= -42,
	kFskErrFileNotOpen			= -43,
	kFskErrParameterError		= -44,
	kFskErrIsDirectory			= -45,
	kFskErrNotDirectory			= -46,
	kFskErrReadOnly				= -47,
	kFskErrDiskFull				= -48,
	kFskErrEndOfFile			= -49,
	kFskErrEndOfDirectory		= -50,
	kFskErrOutOfRange			= -51,
	kFskErrFileExists			= -52,
	kFskErrFileNeedsRecovery	= -53,
	kFskErrFileRecoveryImpossible	= -54,
	kFskErrVolumeLocked			= -55,
	kFskErrVolumeUnavailable	= -56,

    // NETWORK

	kFskErrNameLookupFailed		= -100,
	kFskErrBadSocket			= -101,
	kFskErrSocketNotConnected	= -102,
	kFskErrConnectionRefused	= -103,
	kFskErrNoData				= -104,
	kFskErrAddressInUse			= -105,

	kFskErrNoNetworkInterfaces		= -106,
	kFskErrNetworkInterfaceError	= -107,
	kFskErrNetworkInterfaceNotFound	= -108,
	kFskErrTimedOut					= -109,
	kFskErrNetworkInterfacesChanged	= -110,

	kFskErrSocketFull			= -111,

	kFskErrNoMoreSockets		= -112,
	kFskErrConnectionClosed		= -113,
	kFskErrWaitingForSocket		= -114,
	kFskErrConnectionDropped	= -115,

	kFskErrSSLHandshakeFailed	= -120,
	kFskErrSSLServerAuthFailed	= -121,

	kFskErrAuthFailed			= -130,
	kFskErrAuthPending			= -131,

	kFskErrNetworkInterfaceRemoved	= -150,

	kFskErrNeedConnectionSelection	= -160,

	kFskErrNetworkErr			= -199,

    // HTTP/URL

	kFskErrBadURLNoProtocol		= -200,
	kFskErrNeedMoreData			= -201,

	kFskErrNeedMoreTime			= -202,

	kFskErrURLTooLong			= -203,
	kFskErrHostDoesntMatch		= -204,
	kFskErrProtocolDoesntMatch	= -205,
	kFskErrRequestTooLarge		= -206,
	kFskErrRequestAborted		= -207,
	kFskErrTooManyRedirects		= -208,

	kFskErrUnsupportedSchema	= -250,
	kFskErrUnsupportedMIME		= -251,
	kFskErrUnsupportedSeek		= -252,

    // DATABASE

	kFskErrItemNotFound			= -300,

    // RTSP

	kFskErrRTSPBadPacket		= -500,
	kFskErrRTSPBadPacketParser	= -501,
	kFskErrRTSPPacketParserUnsupportedFormat	= -502,
	kFskErrRTSPBadSDPParam		= -503,
	kFskErrRTSPSessionBadState	= -504,
	kFskErrRTSPReceiverReportErr= -505,
	kFskErrRTSPNoMediaStreams	= -506,
	kFskErrRTSPSocketConfigFailure	= -507,
	kFskErrRTSPSessionRedirect	= -508,
	kFskErrRTSPSessionBadURL	= -509,
	kFskErrRTSPNoUDPPackets		= -510,

    // GRAPHICS

	kFskErrEmpty				= -600,
	kFskErrFull					= -601,
	kFskErrNoSubpath			= -602,
	kFskErrBufferOverflow		= -603,
	kFskErrSingular				= -604,
	kFskErrMismatch				= -605,
	kFskErrTooMany				= -606,
	kFskErrNotAccelerated		= -607,
	kFskErrGraphicsContext		= -608,
	kFskErrTextureTooLarge		= -609,

	// Open GL, Open GL ES

	kFskErrGLInvalidEnum		= -620,
	kFskErrGLInvalidValue		= -621,
	kFskErrGLInvalidOperation	= -622,
	kFskErrGLStackOverflow		= -623,
	kFskErrGLStackUnderflow		= -624,
	kFskErrGLOutOfMemory		= -625,
	kFskErrGLShader				= -626,
	kFskErrGLProgram			= -628,
	kFskErrGLTableTooLarge		= -629,
	kFskErrGLFramebufferUnsupported					= -630,
	kFskErrGLFramebufferUndefined					= -631,
	kFskErrGLInvalidFramebufferOperation			= -632,
	kFskErrGLFramebufferIncompleteAttachment		= -633,
	kFskErrGLFramebufferIncompleteMissingAttachment	= -634,
	kFskErrGLFramebufferIncompleteDimensions		= -635,
	kFskErrGLFramebufferIncompleteFormats			= -636,
	kFskErrGLFramebufferIncompleteDrawBuffer		= -637,
	kFskErrGLFramebufferIncompleteReadBuffer		= -638,
	kFskErrGLFramebufferIncompleteLayerTargets		= -639,
	kFskErrGLFramebufferIncompleteMultisample		= -640,

	// EGL

	kFskErrEGLNotInitialized	= -650,
	kFskErrEGLBadAccess			= -651,
	kFskErrEGLBadAlloc			= -652,
	kFskErrEGLBadAttribute		= -653,
	kFskErrEGLBadConfig			= -654,
	kFskErrEGLBadContext		= -655,
	kFskErrEGLCurrentSurface	= -656,
	kFskErrEGLBadDisplay		= -657,
	kFskErrEGLBadMatch			= -658,
	kFskErrEGLBadNativePixmap	= -659,
	kFskErrEGLBadNativeWindow	= -660,
	kFskErrEGLBadParameter		= -661,
	kFskErrEGLBadSurface		= -662,
	kFskErrEGLContextLost		= -663,

	// CGL

	kFskErrCGLBadAttribute		= -670,
	kFskErrCGLBadProperty		= -671,
	kFskErrCGLBadPixelFormat	= -672,
	kFskErrCGLBadRendererInfo	= -673,
	kFskErrCGLBadContext		= -674,
	kFskErrCGLBadDrawable		= -675,
	kFskErrCGLBadDisplay		= -676,
	kFskErrCGLBadState			= -677,
	kFskErrCGLBadValue			= -678,
	kFskErrCGLBadMatch			= -679,
	kFskErrCGLBadEnumeration	= -680,
	kFskErrCGLBadOffScreen		= -681,
	kFskErrCGLBadFullScreen		= -682,
	kFskErrCGLBadWindow			= -683,
	kFskErrCGLBadAddress		= -684,
	kFskErrCGLBadCodeModule		= -685,
	kFskErrCGLBadAlloc			= -686,
	kFskErrCGLBadConnection		= -687,

	// EAGL

	kFskErrEAGLBadContext		= -690,


    /*
     *	Conditions that are not necessarily Errors
     */

    // Graphics

	kFskErrNothingRendered			= 100,		/* Everything was clipped out -- but sometimes that's OK */
	kFskErrUnalignedYUV				= 101,		/* YUV was not aligned properly, and was coerced to be so, possibly resulting in unexpected blank borders */
	kFskErrGLFramebufferComplete	= 102		/* No error. Frame buffer is complete */
} FskErr;

#ifdef __cplusplus
}
#endif

#endif

