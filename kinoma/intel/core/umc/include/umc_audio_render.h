/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/
#ifndef __UMC_AUDIO_RENDER_H__
#define __UMC_AUDIO_RENDER_H__

#include "vm_time.h"
#include "umc_structures.h"
#include "umc_media_data.h"
#include "umc_module_context.h"
#include "umc_media_receiver.h"

/*
//  Class:       AudioRender
//
//  Notes:       Base abstract class of audio render. Class describes
//               the high level interface of abstract audio render.
//               All system specific (Windows (DirectSound, WinMM), Linux(SDL, etc)) must be implemented in
//               derevied classes.
//
*/

namespace UMC
{

class AudioRenderParams: public MediaReceiverParams
{
    DYNAMIC_CAST_DECL(AudioRenderParams, MediaReceiverParams)

public:
    // Default constructor
    AudioRenderParams(void);
    // Destructor
    virtual ~AudioRenderParams(void){};

    AudioStreamInfo  info;                                      // (AudioStreamInfo) common fields which necessary to initialize decoder
    ModuleContext *pModuleContext;                              // (ModuleContext *) platform dependent context
};

class AudioRender: public virtual MediaReceiver
{
    DYNAMIC_CAST_DECL(AudioRender, MediaReceiver)
public:
    // Default constructor
    AudioRender(void){}
    // Destructor
    virtual ~AudioRender(void){}

    // Send new portion of data to render
    virtual Status SendFrame(MediaData *in) = 0;

    // Pause(Resume) playing of soundSend new portion of data to render
    virtual Status Pause(bool pause) = 0;

    // Volume manipulating
    virtual float SetVolume(float volume) = 0;
    virtual float GetVolume(void) = 0;

    // Audio Reference Clock
    virtual double GetTime(void) = 0;

    // Estimated value of device latency
    virtual double GetDelay(void) = 0;

    virtual Status PrepareForRePosition()
    {return UMC_NOT_IMPLEMENTED;}

    virtual Status SetParameters(MediaReceiverParams *pMedia,
                                 unsigned int  trickModes = UMC_TRICK_MODES_NO)
    {return UMC_NOT_IMPLEMENTED;}

    };
} // namespace UMC

#endif /* __UMC_AUDIO_RENDER_H__ */
