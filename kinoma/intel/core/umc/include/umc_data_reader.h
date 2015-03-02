/*
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/
#ifndef __UMC_DATA_READER_H__
#define __UMC_DATA_READER_H__

/*
//  Class:       DataReader
//
//  Notes:       Base abstract class of data reader. Class describes
//               the high level interface of abstract source of data.
//               All specific ( reading from file, from network, inernet, etc ) must be implemented in
//               derevied classes.
//               Splitter uses this class to obtain data
//
*/

#include "ippdefs.h"
#include "umc_structures.h"
#include "umc_dynamic_cast.h"

namespace UMC
{

class DataReaderParams
{
    DYNAMIC_CAST_DECL_BASE(DataReaderParams)

public:
    // Default constructor
    DataReaderParams(void){}
    // Destructor
    virtual ~DataReaderParams(void){}
};


class DataReader
{
    DYNAMIC_CAST_DECL_BASE(DataReader)
public:

    // Default constructor
    DataReader(void)
      { m_pDataPointer = 0; m_pEODPointer = 0; m_bStop = 0;}
    // Destructor
    virtual ~DataReader(void){}

    // Initialization abstract source data
    virtual Status Init(DataReaderParams *InitParams) = 0;

    // Close source data
    virtual Status Close(void) = 0;

    // Reset all internal parameters to start reading from begin
    virtual Status Reset(void) = 0;

    // Return 2 bytes
    Status GetShort(vm_var16 *data);
    Status GetShortNoSwap(vm_var16 *data);

    // Return 4 bytes
    Status GetUInt(vm_var32 *data);
    Status GetVar32NoSwap(vm_var32 *data);

    // Return 8 bytes
    Status GetVar64(vm_var64 *data);
    Status GetVar64NoSwap(vm_var64 *data);

    // Return 1 byte
    Status GetByte(vm_byte *data);

    // Get data
    Status GetData(void *data, vm_var32 *nsize);

    // Read nsize bytes and copy to data (return number bytes which was copy)
    // Cache data in case of small nsize
    virtual Status ReadData(void *data, vm_var32 *nsize) = 0;

    // Move position on npos bytes
    virtual Status MovePosition(vm_sizet npos) = 0;

    // Check byte
    Status CheckByte(vm_byte *ret_byte, size_t how_far);
    Status CheckShort(vm_var16 *ret_short, size_t how_far);
    Status CheckUInt(vm_var32 *ret_long, size_t how_far);

    // Check data
    Status CheckData(void *data, vm_var32 *nsize, int how_far);

    // Cache and check data
    virtual Status CacheData(void *data, vm_var32 *nsize, int how_far) = 0;

    // Obtain position in the stream
    virtual vm_sizet GetPosition(void) = 0;
    // Obtain size in source data
    virtual vm_sizet GetSize(void) = 0;
    // Set new position
    virtual Status SetPosition(double pos) = 0;

    virtual Status SetPosition (vm_sizet pos)
    {
        vm_sizet curr_pos = GetPosition();
        if (pos >= curr_pos)
        {
            return MovePosition((int)(pos-curr_pos));
        }
        else
        {
            SetPosition(0.0);

            return MovePosition((int)pos);
        }
    }

    Status StartReadingAfterReposition(void)
    {
        return UMC_OK;
    }

public:
    vm_byte  *m_pDataPointer;  // Pointer to the current data
    vm_byte  *m_pEODPointer;   // Pointer to the end of data

protected:

    bool m_bStop;
};

inline
Status DataReader::GetData(void *data, vm_var32 *nsize)
{
    size_t data_sz = (size_t)(*nsize);

    if (((size_t)(m_pEODPointer - m_pDataPointer)) >= data_sz)
    {
        memcpy(data, m_pDataPointer, data_sz);
        m_pDataPointer += data_sz;
        return UMC_OK;
    }

    Status umcRes = UMC_OK;

    do {
        *nsize = data_sz;
        umcRes = ReadData(data, nsize);
    } while (umcRes == UMC_NOT_ENOUGH_DATA && m_bStop == false);

    return umcRes;
} // Status DataReader::GetData(void *data, vm_var32 *nsize)

inline
Status DataReader::GetByte(vm_byte *data)
{
    vm_var32 data_sz = 1;

    return GetData(data, &data_sz);

} // Status DataReader::GetByte(vm_byte *data)

inline
Status DataReader::GetShortNoSwap(vm_var16 *data)
{
    vm_var32 data_sz = 2;
    Status ret = GetData(data, &data_sz);

    *data = BIG_ENDIAN_SWAP16(*data);

    return ret;

} // Status DataReader::GetShortNoSwap(vm_var16 *data)

inline
Status DataReader::GetShort(vm_var16 *data)
{
    vm_var32 data_sz = 2;
    Status ret = GetData(data,&data_sz);
    *data = LITTLE_ENDIAN_SWAP16(*data);

    return ret;

} // Status DataReader::GetShort(vm_var16 *data)

inline
Status DataReader::GetVar32NoSwap(vm_var32 *data)
{
    vm_var32 data_sz = 4;
    Status ret = GetData(data,&data_sz);

    *data = BIG_ENDIAN_SWAP32(*data);

    return ret;

} // Status DataReader::GetVar32NoSwap(vm_var32 *data)

inline
Status DataReader::GetUInt(vm_var32 *data)
{
    vm_var32 data_sz = 4;
    Status ret = GetData(data,&data_sz);
    *data = LITTLE_ENDIAN_SWAP32(*data);
    return ret;

} // Status DataReader::GetUInt(vm_var32 *data)

inline
Status DataReader::CheckData(void *data, vm_var32 *nsize, int how_far)
{
    size_t data_sz = (size_t)(*nsize + how_far);

    if (((size_t)(m_pEODPointer - m_pDataPointer)) >= data_sz)
    {
        memcpy(data, m_pDataPointer + how_far, *nsize);
        return UMC_OK;
    }

    return CacheData(data, nsize, how_far);
} // Status CheckData(void *data, vm_var32 *nsize, int how_far)

inline
Status DataReader::CheckByte(vm_byte *ret_byte, size_t how_far)
{
    vm_var32 data_sz = 1;
    return CheckData(ret_byte, &data_sz, (int)how_far);

} // Status DataReader::CheckByte(vm_byte *ret_byte, size_t how_far)

inline
Status DataReader::CheckShort(vm_var16 *ret_short, size_t how_far)
{
    vm_var32 data_sz = 2;
    Status ret = CheckData(ret_short, &data_sz, (int)how_far);
    *ret_short = LITTLE_ENDIAN_SWAP16(*ret_short);

    return ret;

} // Status DataReader::CheckShort(vm_var16 *ret_short, size_t how_far)

inline
Status DataReader::CheckUInt(vm_var32 *ret_long, size_t how_far)
{
    vm_var32 data_sz = 4;
    Status ret = CheckData(ret_long, &data_sz, (int)how_far);
    *ret_long = LITTLE_ENDIAN_SWAP32(*ret_long);

    return ret;

} // Status DataReader::CheckUInt(vm_var32 *ret_long, size_t how_far)

inline
Status DataReader::GetVar64NoSwap(vm_var64 *data)
{
    vm_var32 data_sz = 8;
    Status ret = GetData(data, &data_sz);

    *data = BIG_ENDIAN_SWAP64(*data);

    return ret;

} // Status DataReader::GetVar64NoSwap(vm_var64 *data)

inline
Status DataReader::GetVar64(vm_var64 *data)
{
    Status ret = GetVar64NoSwap(data);

    *data = LITTLE_ENDIAN_SWAP64(*data);

    return ret;

} // Status DataReader::GetVar64(vm_var64 *data)

} // namespace UMC

#endif /* __UIMC_DATAREADER_H__ */
