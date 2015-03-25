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
#include "kinoma_dec_huff.h"

#define USE_BETTER_TABLE_and_DELETE_OLD_TABLE_LATER

typedef struct
{
	short value;
	short len;
}KIONA_HUFF;

typedef struct
{
	int			len;
	KIONA_HUFF	*tab;
}TAB;

#define TAB_TOTAL	11
#define LZ_MAX		4
#define LO_MAX		16
#define LZ_SF_MAX	1
#define LO_SF_MAX	19

int kinoma_vlcUnsigned[] = { 0, 0, 0, 1, 1,  0,  0,  1,  1,  1,  1,  1};
//int kinoma_vlcTuple[]	 = { 0, 4, 4, 4, 4,  2,  2,  2,  2,  2,  2,  2};
//int leading_zeros_tab_num[]={ 1, 3, 1, 4, 1, 4, 1, 3, 1, 4, 4};
//int leading_ones_tab_num[] ={11, 9,15,12,13,11,12,10,15,12,12};

#ifndef USE_BETTER_TABLE_and_DELETE_OLD_TABLE_LATER
int kinoma_vlcShifts[]   = { 0, 2, 2, 3, 3, 6,   6,  6,  6,  6,  6,  6};
int kinoma_vlcOffsets[]  = { 0, 2, 2, 4, 4, 32, 32, 32, 32, 32, 32, 32};
#endif


#ifndef USE_BETTER_TABLE_and_DELETE_OLD_TABLE_LATER
#include "kinoma_aac_dec_huff_tmp.c"	
#else
#include "kinoma_aac_dec_huff_tab.c"	
#endif

unsigned char leading_zero_next_len_ary[TAB_TOTAL][LZ_MAX] =
{
	{ 0,  0,  0,  0},	//1
	{ 5,  3,  0,  0},	//2
	{ 0,  0,  0,  0},	//3
	{ 3,  2,  1,  0},	//4
	{ 0,  0,  0,  0},	//5
	{ 3,  2,  1,  0},	//6
	{ 0,  0,  0,  0},	//7
	{ 4,  2,  0,  0},	//8
	{ 0,  0,  0,  0},	//9
	{ 5,  3,  1,  0},	//10
	{ 6,  3,  1,  0}	//11
};

KIONA_HUFF *leading_zero_tab_ary[TAB_TOTAL][LZ_MAX] =
{
	{ tab1_z1,  tab1_z1,  tab1_z1,  tab1_z1},		//1
	{ tab2_z1,  tab2_z2,  tab2_z3,  tab2_z3},		//2
	{ tab3_z1,  tab3_z1,  tab3_z1,  tab3_z1},		//3
	{ tab4_z1,  tab4_z2,  tab4_z3,  tab4_z4},		//4
	{ tab5_z1,  tab5_z1,  tab5_z1,  tab5_z1},		//5
	{ tab6_z1,  tab6_z2,  tab6_z3,  tab6_z4},		//6
	{ tab7_z1,  tab7_z1,  tab7_z1,  tab7_z1},		//7
	{ tab8_z1,  tab8_z2,  tab8_z3,  tab8_z3},		//8
	{ tab9_z1,  tab9_z1,  tab9_z1,  tab9_z1},		//9
	{tab10_z1, tab10_z2, tab10_z3, tab10_z4},		//10
	{tab11_z1, tab11_z2, tab11_z3, tab11_z4}		//11
};


unsigned char leading_one_next_len_ary[TAB_TOTAL][LO_MAX] =
{
	{ 4, 5, 4, 5, 4, 4, 4, 3, 2, 1, 0, 0, 0, 0, 0, 0},		//1
	{ 5, 5, 5, 4, 4, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0},		//2
	{ 3, 4, 5, 5, 5, 4, 4, 4, 3, 3, 3, 3, 2, 1, 1, 0},		//3
	{ 4, 5, 5, 5, 5, 4, 4, 3, 2, 1, 1, 0, 0, 0, 0, 0},		//4
	{ 3, 3, 5, 5, 5, 5, 4, 4, 3, 2, 2, 1, 0, 0, 0, 0},		//5
	{ 5, 5, 5, 5, 4, 4, 3, 2, 2, 1, 0, 0, 0, 0, 0, 0},		//6
	{ 2, 4, 5, 5, 5, 4, 4, 3, 2, 2, 1, 0, 0, 0, 0, 0},		//7
	{ 5, 5, 5, 4, 4, 3, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0},		//8
	{ 2, 4, 6, 6, 6, 6, 6, 5, 4, 4, 3, 2, 2, 1, 0, 0},		//9
	{ 6, 6, 6, 6, 5, 5, 4, 3, 3, 2, 1, 0, 0, 0, 0, 0},		//10
	{ 7, 7, 7, 6, 6, 5, 4, 3, 3, 2, 1, 0, 0, 0, 0, 0}		//11
};


KIONA_HUFF *leading_one_tab_ary[TAB_TOTAL][LO_MAX] =
{
	{ tab1_o1, tab1_o2, tab1_o3, tab1_o4, tab1_o5, tab1_o6, tab1_o7, tab1_o8, tab1_o9, tab1_o10, tab1_o11, tab1_o11, tab1_o11, tab1_o11, tab1_o11, tab1_o11},	//1
	{ tab2_o1, tab2_o2, tab2_o3, tab2_o4, tab2_o5, tab2_o6, tab2_o7, tab2_o8, tab2_o9,  tab2_o9,  tab2_o9,  tab2_o9,   tab2_o9, tab2_o9,  tab2_o9,  tab2_o9},	//2
	{ tab3_o1, tab3_o2, tab3_o3, tab3_o4, tab3_o5, tab3_o6, tab3_o7, tab3_o8, tab3_o9, tab3_o10, tab3_o11, tab3_o12, tab3_o13, tab3_o14, tab3_o15, tab3_o16},	//3
	{ tab4_o1, tab4_o2, tab4_o3, tab4_o4, tab4_o5, tab4_o6, tab4_o7, tab4_o8, tab4_o9, tab4_o10, tab4_o11, tab4_o12, tab4_o12, tab4_o12, tab4_o12, tab4_o12},	//4
	{ tab5_o1, tab5_o2, tab5_o3, tab5_o4, tab5_o5, tab5_o6, tab5_o7, tab5_o8, tab5_o9, tab5_o10, tab5_o11, tab5_o12, tab5_o13, tab5_o13, tab5_o13, tab5_o13},	//5
	{ tab6_o1, tab6_o2, tab6_o3, tab6_o4, tab6_o5, tab6_o6, tab6_o7, tab6_o8, tab6_o9, tab6_o10, tab6_o11, tab6_o11, tab6_o11, tab6_o11, tab6_o11, tab6_o11},	//6
	{ tab7_o1, tab7_o2, tab7_o3, tab7_o4, tab7_o5, tab7_o6, tab7_o7, tab7_o8, tab7_o9, tab7_o10, tab7_o11, tab7_o12, tab7_o12, tab7_o12, tab7_o12, tab7_o12},	//7
	{ tab8_o1, tab8_o2, tab8_o3, tab8_o4, tab8_o5, tab8_o6, tab8_o7, tab8_o8, tab8_o9, tab8_o10, tab8_o10, tab8_o10, tab8_o10, tab8_o10, tab8_o10, tab8_o10},	//8
	{ tab9_o1, tab9_o2, tab9_o3, tab9_o4, tab9_o5, tab9_o6, tab9_o7, tab9_o8, tab9_o9, tab9_o10, tab9_o11, tab9_o12, tab9_o13, tab9_o14, tab9_o15, tab9_o15},	//9
	{tab10_o1,tab10_o2,tab10_o3,tab10_o4,tab10_o5,tab10_o6,tab10_o7,tab10_o8,tab10_o9,tab10_o10,tab10_o11,tab10_o12,tab10_o12,tab10_o12,tab10_o12,tab10_o12},	//10
	{tab11_o1,tab11_o2,tab11_o3,tab11_o4,tab11_o5,tab11_o6,tab11_o7,tab11_o8,tab11_o9,tab11_o10,tab11_o11,tab11_o12,tab11_o12,tab11_o12,tab11_o12,tab11_o12} 	//11
};

TAB huff_z_tab[TAB_TOTAL][LZ_MAX];
TAB huff_o_tab[TAB_TOTAL][LO_MAX];

TAB huff_sf_z_tab[LZ_SF_MAX] = 
{
	{0, tab0_z1}
};
TAB huff_sf_o_tab[LO_SF_MAX] = 
{
	{3,  tab0_o1},{3,  tab0_o2},{3,  tab0_o3},{4,  tab0_o4},{4,  tab0_o5},{4,  tab0_o6},
	{4,  tab0_o7},{4,  tab0_o8},{4,  tab0_o9},{4, tab0_o10},{4, tab0_o11},{5, tab0_o12}, 
	{6, tab0_o13},{5, tab0_o14},{4, tab0_o15},{3, tab0_o16},{2, tab0_o17},{1, tab0_o18}, 
	{0, tab0_o19}
};

//count leading ones, and shift left in place
#define clo_x( cw_x, max_x, lead_x )				\
{												\
	lead_x = 0;									\
	while( (cw_x>>31) != 0 && lead_x < max_x  )	\
	{											\
		lead_x++;								\
		cw_x<<=1;								\
	}											\
}


//count leading zeros, and shift left in place
#define clz_x( cw_x, max_x, lead_x )				\
{												\
	lead_x = 0;									\
	while( (cw_x>>31) == 0 && lead_x < max_x  )	\
	{											\
		lead_x++;								\
		cw_x<<=1;								\
	}											\
}

static int kinoma_aac_dec_get_tab( int idx, void **tab_z, void **tab_o )
{
	*tab_z = huff_z_tab[idx-1];
	*tab_o = huff_o_tab[idx-1];
	return 1;
}


//bits32_in needs to be left shifted
static int kinoma_dec_sf_huff( int u32, int *value_out, int *len_out )
{
	TAB			*tab_z = huff_sf_z_tab; 
	TAB			*tab_o = huff_sf_o_tab; 
	KIONA_HUFF	*tab_v; 
	int lead, len, mask, idx, pack, cw_len; 

	lead  = u32>>31;	//check bit 31
	if( lead == 0 )
	{
		cw_len = 1;
		pack   = tab_z[0].tab->value;
		goto done;
	}

	clo_x(u32, LO_SF_MAX, lead);

	len	    = tab_o[lead-1].len;
	tab_v   = tab_o[lead-1].tab;
	mask	= (1<<len)-1;
	idx		= (u32>>(32-len))&mask;
	pack	= tab_v[idx].value;
	cw_len	= tab_v[idx].len;

done:
	*value_out  = pack;
	*len_out    = cw_len;

	return 0;
}


int kinoma_aac_dec_huff_one_sf(unsigned char **bs_in, int *offset_in, short* pDst )
{
	unsigned char *bs    = *bs_in;
	int			  offset = *offset_in;
	int u32;
	int value;
	int cw_len;

	BS_LOAD32(bs, offset, u32)

	kinoma_dec_sf_huff( u32, &value, &cw_len );

	BS_RESET(bs, cw_len, offset)

	*bs_in	   = bs;
    *offset_in = offset;
	*pDst	   = value;

	return 0;
}


static int kinoma_dec_huff( int u32, void  *tab_z_in, void  *tab_o_in, int *value_out, int *len_out )
{
	TAB			*tab_z = (TAB *)tab_z_in; 
	TAB			*tab_o = (TAB *)tab_o_in; 
	KIONA_HUFF	*tab_v; 
	TAB			*tab = tab_o; 
	int  lead, len, mask, idx, pack, cw_len; 

	clo_x( u32, LO_MAX, lead )

	if( lead == 0 )
	{
		clz_x( u32, LZ_MAX, lead )
		tab = tab_z;
	}

	len	    = tab[lead-1].len;
	tab_v   = tab[lead-1].tab;
	mask	= (1<<len)-1;
	idx		= (u32>>(32-len))&mask;
	pack	= tab_v[idx].value;
	cw_len	= tab_v[idx].len;

	*value_out  = pack;
	*len_out    = cw_len;

	return 0;
}


static int kinoma_dec_aac_huff_unsigned_pair_block (unsigned char ** bs_in, int* bit_offset_in, void *tab_z, void *tab_o, short **pDst_in, int num, int idx )
{
	unsigned char   *bs	= *bs_in;
	int		bit_offset	= *bit_offset_in;
	short	*dst = *pDst_in;

#ifndef USE_BETTER_TABLE_and_DELETE_OLD_TABLE_LATER
	int     shift	= kinoma_vlcShifts[idx];
	int     mask	= (1 << (shift)) - 1;	
#endif

	int		i;

	for( i = 0; i < num; i++ )
	{
		unsigned int u32; 
		int		cw, y, z, len;
		int		bits_left;

		BS_LOAD32(bs, bit_offset, u32)

		kinoma_dec_huff( u32, tab_z, tab_o, &cw, &len );

#ifndef USE_BETTER_TABLE_and_DELETE_OLD_TABLE_LATER
		DEC_UNSIGNED_PAIR( cw, y, z, shift, mask )
#else
		DEC_UNSIGNED_PAIR_XXX( cw, y, z )
#endif

		bits_left = 32 - len;
		if(y != 0) 
		{                                                            
			if (U32_READ_ONE_BIT(u32, bits_left)) 
				y = -y;     
			len++;
		} 

		if (z != 0)  
		{                                                            
			if (U32_READ_ONE_BIT(u32, bits_left))     
				z = -z;   
			len++;
		} 

		BS_RESET(bs, len, bit_offset);

		dst[0] = y;
		dst[1] = z;
		dst += 2;
	}

	*bs_in			= bs; 
	*bit_offset_in	= bit_offset;
	*pDst_in		= dst;

	return 0;
}


static int kinoma_dec_aac_huff_signed_pair_block (unsigned char ** bs_in, int* bit_offset_in, void *tab_z, void *tab_o, short **pDst_in, int num, int idx )
{
	unsigned char   *bs	= *bs_in;
	int		bit_offset	= *bit_offset_in;
	short	*dst = *pDst_in;
	int		i;

#ifndef USE_BETTER_TABLE_and_DELETE_OLD_TABLE_LATER
	int     shift	= kinoma_vlcShifts[idx];
	int     offset  = kinoma_vlcOffsets[idx];
	int     mask	= (1 << (shift)) - 1;	
#endif

	for( i = 0; i < num; i++ )
	{
		unsigned int u32; 
		int		cw, y, z, len;

		BS_LOAD32(bs, bit_offset, u32)

		kinoma_dec_huff( u32, tab_z, tab_o, &cw, &len );

#ifndef USE_BETTER_TABLE_and_DELETE_OLD_TABLE_LATER
		DEC_SIGNED_PAIR( cw, y, z, shift, mask, offset )
#else
		DEC_SIGNED_PAIR_XXX( cw, y, z )
#endif

		BS_RESET(bs, len, bit_offset);

		dst[0] = y;
		dst[1] = z;
		dst += 2;
	}

	*bs_in			= bs; 
	*bit_offset_in	= bit_offset;
	*pDst_in		= dst;

	return 0;
}


int kinoma_aac_dec_huff_pair_block (unsigned char ** bs_in, int* bit_offset_in, short **pDst_in, int num, int idx )
{
	int	 is_unsigned_tab = kinoma_vlcUnsigned[idx];
	void *tab_z, *tab_o;
	
	kinoma_aac_dec_get_tab( idx, &tab_z, &tab_o );

	if( is_unsigned_tab )
		kinoma_dec_aac_huff_unsigned_pair_block ( bs_in, bit_offset_in, tab_z, tab_o, pDst_in, num, idx );
	else
		kinoma_dec_aac_huff_signed_pair_block (   bs_in, bit_offset_in, tab_z, tab_o, pDst_in, num, idx );

	return 0;
}


static int kinoma_dec_aac_huff_unsigned_quad_block (unsigned char ** bs_in, int* bit_offset_in, void *tab_z, void *tab_o, short **pDst_in, int num, int idx)
{
	unsigned char	*bs		= *bs_in;
	int		bit_offset = *bit_offset_in;
	short	*dst	= *pDst_in;

#ifndef USE_BETTER_TABLE_and_DELETE_OLD_TABLE_LATER
	int     shift	= kinoma_vlcShifts[idx];
	int     offset  = kinoma_vlcOffsets[idx];
	int     mask	= (1 << (shift)) - 1;	
#endif

	int		i;
	
	kinoma_aac_dec_get_tab( idx, &tab_z, &tab_o );


	for( i = 0; i < num; i++ )
	{
		unsigned int u32; 
		int		cw, w, x, y, z, len;

		BS_LOAD32(bs, bit_offset, u32)

		kinoma_dec_huff( u32, tab_z, tab_o, &cw, &len );

		{
			int		bits_left;

#ifndef USE_BETTER_TABLE_and_DELETE_OLD_TABLE_LATER
			DEC_UNSIGNED_QUAD( cw, w, x, y, z, shift, mask )
#else
			DEC_UNSIGNED_QUAD_XXX( cw, w, x, y, z )
#endif

			bits_left = 32 - len;
			if(w != 0) 
			{                                                            
				if (U32_READ_ONE_BIT(u32, bits_left))               
					w = -w;  

				len++;
			} 

			if(x != 0) 
			{                                                            
				if (U32_READ_ONE_BIT(u32, bits_left))               
					x = -x;                                                   

				len++;
			} 

			if(y != 0) 
			{                                                            
				if (U32_READ_ONE_BIT(u32, bits_left))               
					y = -y;                                                   

				len++;
			} 

			if(z != 0) 
			{                                                            
				if (U32_READ_ONE_BIT(u32, bits_left))               
					z = -z;                                                   

				len++;
			} 
		}

		BS_RESET(bs, len, bit_offset);

		dst[0] = w;
		dst[1] = x;
		dst[2] = y;
		dst[3] = z;
		dst += 4;
	}

	*bs_in			= bs; 
	*bit_offset_in	= bit_offset;
	*pDst_in = dst;

	return 0;
}


static int kinoma_dec_aac_huff_signed_quad_block (unsigned char ** bs_in, int* bit_offset_in, void *tab_z, void *tab_o, short **pDst_in, int num, int idx)
{
	unsigned char	*bs		= *bs_in;
	int		bit_offset = *bit_offset_in;
	short	*dst	= *pDst_in;

#ifndef USE_BETTER_TABLE_and_DELETE_OLD_TABLE_LATER
	int     shift	= kinoma_vlcShifts[idx];
	int     offset  = kinoma_vlcOffsets[idx];
	int     mask	= (1 << (shift)) - 1;	
#endif

	int		i;
	
	for( i = 0; i < num; i++ )
	{
		unsigned int u32; 
		int		cw, w, x, y, z, len;

		BS_LOAD32(bs, bit_offset, u32)

		kinoma_dec_huff( u32, tab_z, tab_o, &cw, &len );

#ifndef USE_BETTER_TABLE_and_DELETE_OLD_TABLE_LATER
		DEC_SIGNED_QUAD( cw, w, x, y, z, shift, mask, offset )
#else
		DEC_SIGNED_QUAD_XXX( cw, w, x, y, z )
#endif

		BS_RESET(bs, len, bit_offset);

		dst[0] = w;
		dst[1] = x;
		dst[2] = y;
		dst[3] = z;
		dst += 4;
	}

	*bs_in			= bs; 
	*bit_offset_in	= bit_offset;
	*pDst_in		= dst;

	return 0;
}


int kinoma_aac_dec_huff_quad_block (unsigned char ** bs_in, int* bit_offset_in, short **pDst_in, int num, int idx )
{
	int	 is_unsigned_tab = kinoma_vlcUnsigned[idx];
	void *tab_z, *tab_o;
	
	kinoma_aac_dec_get_tab( idx, &tab_z, &tab_o );

	if( is_unsigned_tab )
		kinoma_dec_aac_huff_unsigned_quad_block ( bs_in, bit_offset_in, tab_z, tab_o, pDst_in, num, idx );
	else
		kinoma_dec_aac_huff_signed_quad_block(   bs_in, bit_offset_in, tab_z, tab_o, pDst_in, num, idx );

	return 0;
}


int kinoma_aac_dec_huff_esc_block(unsigned char **bs_in, int* bit_offset_in, short *dst, int num, int idx)
{
	unsigned char	*bs = *bs_in;
	int				bit_offset = *bit_offset_in;

#ifndef USE_BETTER_TABLE_and_DELETE_OLD_TABLE_LATER
	int				shift	= kinoma_vlcShifts[11];
	int				offset  = kinoma_vlcOffsets[11];
	int				 mask	= (1 << (shift)) - 1;	
#endif

	int				n, y, z, len, cw, lead;
	unsigned int	u32, currBits; 
	void			*tab_z, *tab_o;
	
	kinoma_aac_dec_get_tab( 11, &tab_z, &tab_o );

	num>>=1;
	for (n = 0; n < num; n++)
	{
		int sign_y = 0, sign_z= 0;

		BS_LOAD32(bs, bit_offset, u32)
	
		kinoma_dec_huff( u32, tab_z, tab_o, &cw, &len );

#ifndef USE_BETTER_TABLE_and_DELETE_OLD_TABLE_LATER
		DEC_UNSIGNED_PAIR_ESC( cw, y, z, shift, mask )
#else
		DEC_UNSIGNED_PAIR_ESC_XXX( cw, y, z )
#endif

		u32<<=len;
		if(y)
		{
			sign_y = (u32 >> 31) & 1;   
			len++;
			u32<<=1;
		}

		if(z)
		{
			sign_z = (u32 >> 31) & 1; 
			len++;
			u32<<=1;
		}

		if (y == 16) 
		{                                                  
			BS_RESET(bs, len, bit_offset)
			BS_LOAD32(bs, bit_offset, u32)

			clo_x(u32, 19, lead);
			
			u32<<=1;
			currBits = u32 >> (32 - (lead + 4));	// 32 - (n + 4)
			y = currBits + (1<<(lead+4));
			len = 2*lead + 5;
		}  

		if( sign_y )
			y *= -1;

		if (z == 16) 
		{                                                  
			BS_RESET(bs, len, bit_offset)
			BS_LOAD32(bs, bit_offset, u32)
			
			clo_x(u32, 19, lead);
			
			u32<<=1;
			currBits = u32 >> (32 - (lead + 4));	// 32 - (n + 4)
			z = currBits + (1<<(lead+4));
			len = 2*lead + 5;
		}  

		if( sign_z )
			z *= -1;
		
		BS_RESET(bs, len, bit_offset)

		dst[0] = y;
		dst[1] = z;
		dst+= 2;
	}

	*bs_in			= bs; 
	*bit_offset_in  = bit_offset;

	return 0;
}


void kinoma_aac_dec_huff_init_tab()
{
	int i, j;

	for(i = 0; i < TAB_TOTAL; i++)
	{
		for( j = 0; j < LZ_MAX; j++ )
		{
			huff_z_tab[i][j].len = leading_zero_next_len_ary[i][j];
			huff_z_tab[i][j].tab = leading_zero_tab_ary[i][j];
		}

		for( j = 0; j < LO_MAX; j++ )
		{
			huff_o_tab[i][j].len = leading_one_next_len_ary[i][j];
			huff_o_tab[i][j].tab = leading_one_tab_ary[i][j];
		}
	}
}

