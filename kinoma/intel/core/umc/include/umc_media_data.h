/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/

#ifndef __UMC_MEDIA_DATA_H__
#define __UMC_MEDIA_DATA_H__

#include "umc_structures.h"
#include "umc_dynamic_cast.h"

namespace UMC
{

class MediaData_V51
{
    DYNAMIC_CAST_DECL_BASE(MediaData_V51)

public:

    // Default constructor
    MediaData_V51(size_t length = 0);
    // Destructor
    virtual ~MediaData_V51();

    // Release object
    virtual Status Close(void);

    // allow to obtain pointer to the buffer
    // could be not equal to pointer to the data
    virtual void *GetBufferPointer(void) { return m_pBufferPointer; };

    // allow to obtain pointer to the data
    // could be not equal to pointer to the data
    virtual void *GetDataPointer(void)   { return m_pDataPointer; };

    // return size of buffer
    virtual size_t GetBufferSize(void) {return m_nBufferSize;}

    // return size of valid data in the buffer
    virtual size_t GetDataSize(void) {return m_nDataSize;}


    // Set the pointer to a buffer allocated by the user
    // bytes define the size of buffer
    // size of data is equal to buffer size after this call
    virtual Status SetBufferPointer(vm_byte *ptr, size_t bytes);

    // Set size of valid data in the buffer.
    // Data start from the beginning of buffer!!!
    virtual Status SetDataSize(size_t bytes);

    //  Move data pointer inside and decrease or increase data size and
    virtual Status MoveDataPointer(int bytes);

    // return time stamp of media data
    virtual double GetTime(void) {return m_pts_start;}

    // return time stamp of media data, start and end
    virtual Status GetTime(double &start, double &end);

    //  Set time stamp of media data block;
    virtual Status SetTime(double start, double end = 0);

	MediaData_V51 & operator = (MediaData_V51 &);

	//***
	void SetRefCon( long idx, unsigned long refCon ) { m_refCon[idx] = refCon; }
	unsigned long GetRefCon( long idx ) { return m_refCon[idx]; }

protected:

    double  m_pts_start;                                         // (double) start media PTS
    double  m_pts_end;                                           // (double) finish media PTS
    size_t  m_nBufferSize;                                       // (size_t) size of buffer
    size_t  m_nDataSize;                                         // (size_t) quantity of data in buffer

    vm_byte* m_pBufferPointer;
    vm_byte* m_pDataPointer;


    // Actually this variable should has type bool.
    // But some compilers generate poor executable code.
    // On count of this, we use type unsigned int.
    unsigned int m_bMemoryAllocated;                             // (unsigned int) is memory owned by object
	
	//***refCon for user data
	unsigned long m_refCon[2];

};

} // namespace UMC

#endif /* __UMC_MEDIA_DATA_H__ */
