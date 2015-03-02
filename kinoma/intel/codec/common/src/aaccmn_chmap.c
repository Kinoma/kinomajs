/*//////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2003-2006 Intel Corporation. All Rights Reserved.
//
*/

#include "aaccmn_const.h"
#include "aaccmn_chmap.h"
#include "mp4cmn_pce.h"
#include "aaccmn_adts.h"

void
chmap_create_by_pce(sProgram_config_element* p_pce,sCh_map_item map[EL_ID_MAX][EL_TAG_MAX])
{
    int i;
    int n;
    int tag;
    int ch;
    int id;

    /**********************************************************
                        Front Channels
     **********************************************************/
    i = 0;
    n = p_pce->num_front_channels;
    if (n & 1)
    {
        n --;
        tag = p_pce->front_element_tag_select[i];
        map[ID_SCE][tag].key = CH_FRONT_CENTER;
        i ++;
    }

    ch = CH_FRONT_LEFT;
    for (;i < p_pce->num_front_channel_elements; i ++)
    {
        tag = p_pce->front_element_tag_select[i];
        id = p_pce->front_element_is_cpe[i];
        map[id][tag].key = (short)ch;
        ch += (1+id);
    }

    /**********************************************************
                        Back Channels
    **********************************************************/
    i = 0;
    n = p_pce->num_back_channels;
    if (n & 1)
    {
        n --;
        tag = p_pce->back_element_tag_select[i];
        map[ID_SCE][tag].key = CH_BACK_CENTER;
        i ++;
    }
    ch = CH_BACK_LEFT;
    for (;i < p_pce->num_back_channel_elements; i ++)
    {
        tag = p_pce->back_element_tag_select[i];
        id = p_pce->back_element_is_cpe[i];
        map[id][tag].key = (short)ch;
        ch += (1+id);
    }
    /**********************************************************
                        Side Channels
    **********************************************************/
    i = 0;
    ch = CH_SIDE_LEFT;
    for (;i < p_pce->num_side_channel_elements; i ++)
    {
        tag = p_pce->side_element_tag_select[i];
        id = p_pce->side_element_is_cpe[i];
        map[id][tag].key = (short)ch;
        ch += (1+id);
    }
    /**********************************************************
                        LF Channels
    **********************************************************/
    i = 0;
    ch = CH_LOW_FREQUENCY;
    for (;i < p_pce->num_lfe_channel_elements; i ++)
    {
        tag = p_pce->lfe_element_tag_select[i];
        map[ID_LFE][tag].key = (short)ch;
        ch ++;
    }
}

int
chmap_create_by_adts(int channel_configuration,sCh_map_item chmap[EL_ID_MAX][EL_TAG_MAX],sEl_map_item elmap[],int el_num,int order[])
{
    sEl_map_item local_elmap[10];
    int i;
//    int n;
    int tag;
//    int ch;
    int id;
    int err = 0;

    if ( (el_num > 10) || (channel_configuration == 0 )) return -1;

    for( i = 0; i < el_num; i ++)
    {
        local_elmap[i] = elmap[i];
    }
    for(; i < 10; i ++)
    {
        local_elmap[i].id = -1;
    }

    switch (channel_configuration)
    {
    case 7:
        for ( i = el_num - 1; (local_elmap[i].id != ID_CPE) && (i >= 0); i --);
        for (;(local_elmap[i].id != ID_CPE) && (i >= 0); i --);
        if  (i == -1 )
        {
            err = 1;
            break;
        }
        id  = elmap[i].id;
        tag = elmap[i].tag;
        chmap[id][tag].ch = elmap[i].ch;
        chmap[id][tag].key = CH_OUTSIDE_LEFT;
        elmap[i].id = -1;
    case 6:
        for ( i = el_num - 1; (local_elmap[i].id != ID_LFE) && (i >= 0); i --);
        if  (i == -1 )
        {
            err = 1;
            break;
        }
        id  = elmap[i].id;
        tag = elmap[i].tag;
        chmap[id][tag].ch = elmap[i].ch;
        chmap[id][tag].key = CH_LOW_FREQUENCY;
    case 5:
        for ( i = el_num - 1; (local_elmap[i].id != ID_CPE) && (i >= 0); i --);
        if  (i == -1 )
        {
            err = 1;
            break;
        }
        id  = elmap[i].id;
        tag = elmap[i].tag;
        chmap[id][tag].ch = elmap[i].ch;
        chmap[id][tag].key = CH_SURROUND_LEFT;
    case 4:
        for ( i = el_num - 1; local_elmap[i].id != ID_SCE; i --);
        if  (i == -1 )
        {
            err = 1;
            break;
        }
        id  = elmap[i].id;
        tag = elmap[i].tag;
        chmap[id][tag].ch = elmap[i].ch;
        chmap[id][tag].key = CH_SURROUND_CENTER;
    case 3:
        for ( i = 0; (local_elmap[i].id != ID_SCE) && (i < el_num); i ++);
        if  (i == el_num)
        {
            err = 1;
            break;
        }
        id  = elmap[i].id;
        tag = elmap[i].tag;
        chmap[id][tag].ch = elmap[i].ch;
        chmap[id][tag].key = CH_FRONT_CENTER;
    case 2:
        for ( i = 0; (local_elmap[i].id != ID_CPE) && (i < el_num); i ++);
        if  (i == el_num)
        {
            err = 1;
            break;
        }
        id  = elmap[i].id;
        tag = elmap[i].tag;
        chmap[id][tag].ch = elmap[i].ch;
        chmap[id][tag].key = CH_FRONT_LEFT;
        break;
    case 1:
        for ( i = 0; (local_elmap[i].id != ID_SCE) && (i < el_num); i ++);
        if  (i == el_num)
        {
            err = 1;
            break;
        }
        id  = elmap[i].id;
        tag = elmap[i].tag;
        chmap[id][tag].ch = elmap[i].ch;
        chmap[id][tag].key = CH_FRONT_CENTER;
        break;
    default:
        err = 2;
        break;
    }

    return err;
}



int
chmap_order(sCh_map_item chmap[EL_ID_MAX][EL_TAG_MAX],sEl_map_item elmap[],int el_num,int order[])
{
    int i;
    int j;
    int id;
    int tag;
    int key[CH_MAX];
    int ch;
    int exch;

    ch = 0;
    for (i = 0; i < el_num; i ++)
    {
        id  = elmap[i].id;
        tag = elmap[i].tag;

        if (ID_CCE == id) continue;

        key[ch] = chmap[id][tag].key;
        order[ch] = chmap[id][tag].ch;
        ch ++;

        if (ID_CPE == id)
        {
            key[ch] = chmap[id][tag].key + 1;
            order[ch] = chmap[id][tag].ch + 1;
            ch ++;
        }
    }

    /// Sort
    for (i = 0; i < ch; i ++)
    {
        for (j = i; j < ch; j ++)
        {
            if (key[j] < key[i])
            {
                exch = key[j];
                key[j] = key[i];
                key[i] = exch;

                exch = order[j];
                order[j] = order[i];
                order[i] = exch;
            }
        }
    }
    return ch;
}
