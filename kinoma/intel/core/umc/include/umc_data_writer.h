/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//       Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/

#ifndef __UMC_DATA_WRITER_H__
#define __UMC_DATA_WRITER_H__

/*
//  Class:       DataWriter
//
//  Notes:       Base abstract class of data writer. Class describes
//               the high level interface of abstract source of data.
//               All specific ( reading from file, from network, inernet, etc ) must be implemented in
//               derevied classes.
//               Splitter uses this class to obtain data
//
*/

#include "umc_structures.h"
#include "umc_dynamic_cast.h"

namespace UMC
{

class DataWriterParams
{
    DYNAMIC_CAST_DECL_BASE(DataWriterParams)

public:
    // Default constructor
    DataWriterParams(void){}
    // Destructor
    virtual ~DataWriterParams(void){}
};

class DataWriter
{
    DYNAMIC_CAST_DECL_BASE(DataWriter)
public:
    // Default constructor
    DataWriter(void){}
    // Destructor
    virtual ~DataWriter(void){}

    // Initialization abstract destination media
    virtual Status Init(DataWriterParams *InitParams) = 0;

    // Close destination media
    virtual Status Close(void) = 0;

    // Reset all internal parameters to start writing from begin
    virtual Status Reset(void) = 0;

    // Write data to output stream
    virtual Status PutData(void *data, int &nsize) = 0;

    // Set current position in destination media
    virtual Status SetPosition(vm_var32 /* nPosLow */,
                                vm_var32 * /* pnPosHigh */,
                                vm_var32 /* nMethod */)
    {   return UMC_OPERATION_FAILED;    }

    // Get current position in destination media
    virtual Status GetPosition(vm_var32 * /* pnPosLow */, vm_var32 * /* pnPosHigh */)
    {   return UMC_OPERATION_FAILED;    }

};

} // namespace UMC

#endif /* __UMC_DATA_WRITER_H__ */
