/*
//
//              INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license  agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in  accordance  with the terms of that agreement.
//        Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
//
*/
#ifndef __UMC_MEDIA_DATA_EX_H__
#define __UMC_MEDIA_DATA_EX_H__

#include "umc_media_data.h"

namespace UMC
{

class MediaDataEx_V51 : public MediaData_V51
{
    DYNAMIC_CAST_DECL(MediaDataEx_V51, MediaData_V51)

public:
    class _MediaDataEx_V51{
        DYNAMIC_CAST_DECL_BASE(_MediaDataEx_V51)
        public:
        unsigned int count;
        unsigned int index;
        vm_var64     bstrm_pos;
        unsigned int *offsets;
        unsigned int *values;
        unsigned int limit;

        _MediaDataEx_V51()
        {
            count = 0;
            index = 0;
            bstrm_pos = 0;
            limit   = 2000;
            offsets = (unsigned int*)malloc(sizeof(unsigned int)*limit);
            values  = (unsigned int*)malloc(sizeof(unsigned int)*limit);
        }

		//***
		_MediaDataEx_V51(int limit_in)
        {
            count = 0;
            index = 0;
            bstrm_pos = 0;
            limit   = limit_in;
            offsets = (unsigned int*)malloc(sizeof(unsigned int)*limit);
            values  = (unsigned int*)malloc(sizeof(unsigned int)*limit);
       }

        ~_MediaDataEx_V51()
        {
            if(offsets)
            {
                free(offsets);
                offsets = 0;
            }
            if(values)
            {
                free(values);
                values = 0;
            }
            limit   = 0;
        }

    };

    // Default constructor
    MediaDataEx_V51()
    {
        m_exData = NULL;
    };
    // Destructor
    virtual ~MediaDataEx_V51(){};

    _MediaDataEx_V51* GetExData()
    {
        return m_exData;
    };

    void SetExData(_MediaDataEx_V51* pDataEx)
    {
        m_exData = pDataEx;
    };

protected:
    _MediaDataEx_V51 *m_exData;
};



class MediaDataEx_2_V51 : public MediaData_V51
{
    DYNAMIC_CAST_DECL(MediaDataEx_2_V51, MediaData_V51)

public:
    class _MediaDataEx_2_V51{
        DYNAMIC_CAST_DECL_BASE(_MediaDataEx_2_V51)
        public:
        unsigned int count;
        unsigned int index;
        vm_var64     bstrm_pos;
        unsigned int *offsets;
        unsigned int *values;

		unsigned char *nalu_type_ary;
		unsigned char *bit_offset_ary;
		unsigned long **data_ptr_ary;
		unsigned int  *size_ary;

		unsigned int limit;

        _MediaDataEx_2_V51()
        {
            count = 0;
            index = 0;
            bstrm_pos = 0;
            limit   = 2000;
            offsets = (unsigned int*)malloc(sizeof(unsigned int)*limit);
            values  = (unsigned int*)malloc(sizeof(unsigned int)*limit);

            nalu_type_ary = (unsigned char *)malloc(sizeof(unsigned char)*limit);
            bit_offset_ary = (unsigned char *)malloc(sizeof(unsigned char)*limit);
            data_ptr_ary = (unsigned long**)malloc(sizeof(unsigned long*)*limit);
            size_ary  = (unsigned int*)malloc(sizeof(unsigned int)*limit);
		}

		//***
		_MediaDataEx_2_V51(int limit_in)
        {
            count = 0;
            index = 0;
            bstrm_pos = 0;
            limit   = limit_in;
            offsets = (unsigned int*)malloc(sizeof(unsigned int)*limit);
            values  = (unsigned int*)malloc(sizeof(unsigned int)*limit);
 
            nalu_type_ary = (unsigned char *)malloc(sizeof(unsigned char)*limit);
            bit_offset_ary = (unsigned char *)malloc(sizeof(unsigned char)*limit);
            data_ptr_ary = (unsigned long**)malloc(sizeof(unsigned long*)*limit);
            size_ary  = (unsigned int*)malloc(sizeof(unsigned int)*limit);
		
		}

        ~_MediaDataEx_2_V51()
        {
            if(offsets)
            {
                free(offsets);
                offsets = 0;
            }
            if(values)
            {
                free(values);
                values = 0;
            }
            limit   = 0;
        }

    };

    // Default constructor
    MediaDataEx_2_V51()
    {
        m_exData = NULL;
    };
    // Destructor
    virtual ~MediaDataEx_2_V51(){};

    _MediaDataEx_2_V51* GetExData()
    {
        return m_exData;
    };

    void SetExData(_MediaDataEx_2_V51* pDataEx)
    {
        m_exData = pDataEx;
    };

protected:
    _MediaDataEx_2_V51 *m_exData;
};


}


class MediaData3_V51
{
	public:
	
	vm_byte* m_pBufferPointer;
    size_t   m_nBufferSize;                                       // (size_t) size of buffer

    unsigned int	count;
    unsigned int	limit;
    unsigned char	*nalu_type_ary;
    unsigned char	*bit_offset_ary;
    unsigned long	**data_ptr_ary;
    unsigned int	*size_ary;

	MediaData3_V51(int limit_in)
    {
		m_pBufferPointer= NULL;
		m_nBufferSize	= 0;     // (size_t) size of buffer

        count			= 0;
        limit			= limit_in;
        nalu_type_ary	= (unsigned char*)malloc(sizeof(unsigned char)*limit);
        bit_offset_ary  = (unsigned char*)malloc(sizeof(unsigned int)*limit);
        data_ptr_ary	= (unsigned long **)malloc(sizeof(unsigned long *)*limit);
        size_ary		= (unsigned int*)malloc(sizeof(unsigned int)*limit);

    }

    ~MediaData3_V51()
    {
		if( m_pBufferPointer)
			free( m_pBufferPointer );

        if(nalu_type_ary)
            free(nalu_type_ary);

        if(bit_offset_ary)
            free(bit_offset_ary);

		if(data_ptr_ary)
            free(data_ptr_ary);

		if(size_ary)
            free(size_ary);
    }
};


#endif //__UMC_MEDIA_DATA_EX_H__

