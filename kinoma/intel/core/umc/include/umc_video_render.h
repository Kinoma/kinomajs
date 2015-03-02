/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//       Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/

#ifndef ___UMC_VIDEO_RENDER_H___
#define ___UMC_VIDEO_RENDER_H___


#include <memory.h>
#include "vm_debug.h"
#include "umc_media_receiver.h"
#include "vm_semaphore.h"
#include "umc_structures.h"
#include "umc_dynamic_cast.h"
#include "umc_module_context.h"

namespace UMC
{

#if defined(_WIN32) || defined(_WIN32_WCE)

void UMCRect2Rect(UMC::RECT& umcRect, ::RECT& rect);
void Rect2UMCRect(::RECT& rect, UMC::RECT& umcRect);

#endif // defined(_WIN32) || defined(_WIN32_WCE)

class VideoRenderParams_V51: public MediaReceiverParams_V51
{
    DYNAMIC_CAST_DECL(VideoRenderParams_V51, MediaReceiverParams_V51)

public:
    // Default constructor
    VideoRenderParams_V51();

    ColorFormat color_format;                                   // (ColorFormat) working color format
    ClipInfo info;                                              // (ClipInfo) video size
    UMC::RECT disp;                                             // (UMC::RECT) display position
    UMC::RECT range;                                            // (UMC::RECT) range of source video to display
    vm_var32 lFlags;                                            // (vm_var32) video render flag(s)

};

class VideoRender_V51: public MediaReceiver_V51
{
    DYNAMIC_CAST_DECL(VideoRender_V51, MediaReceiver_V51)

public:

    // Destructor
    virtual ~VideoRender_V51()
    {
        Close();
    }

    struct sCaps
    {
        double min_stretch;
        double max_stretch;
        bool   overlay_support;
        bool   colorkey_support;
    };

    // Outdated functions - don't use it, use MediaReceiver_V51 interface instead

    // Reserve a frame buffer for decoding the current frame
    virtual int ReserveBuffer(vm_byte **ppucVidMem) = 0;

    // Buffer the decoded frame
    virtual Status BufferReserved(double frame_time, FrameType frame_type) = 0;

    virtual void BufferEndOfClip() = 0;

    // VideoRender_V51 interface extension above MediaReceiver_V51

    // Peek presentation of next frame, return presentation time
    virtual double GetRenderFrame() = 0;

    // Rendering the current frame
    virtual Status RenderFrame() = 0;

    // Show the last rendered frame
    virtual Status ShowLastFrame() = 0;

    // Set/Reset full screen playback
    virtual Status SetFullScreen(ModuleContext& rContext, bool full) = 0;

    // Resize the display rectangular
    virtual Status ResizeDisplay(UMC::RECT &disp, UMC::RECT &range) = 0;

    // Show/Hide Surface
    virtual void ShowSurface() = 0;

    virtual void HideSurface() = 0;

    virtual Status PrepareForRePosition()
    {return UMC_OK;};

protected:
    virtual int LockSurface(vm_byte **vidmem) = 0;
    virtual int UnlockSurface(vm_byte**vidmem) = 0;
};

} // namespace UMC

#endif // ___UMC_VIDEO_RENDER_H___
