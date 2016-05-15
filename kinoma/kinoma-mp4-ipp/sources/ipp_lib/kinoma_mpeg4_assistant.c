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

#include "kinoma_mpeg4_assistant.h"
#include "ipps.h"

#pragma warning (disable:4047) 


/*---------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------/
 Decode the intra predicated DC size
*/
static int dc_lum_tbl[8][2] = 
{
	{0, 0}, {3, 4}, {3, 3}, {3, 0},
	{2, 2}, {2, 2}, {2, 1}, {2, 1},	
};

Ipp32u
VlcDecIntraDCPredSize(Ipp8u **ppBitStream, int *pBitOffset, int blockType)
{
	Ipp32u  code;
	int i;

	if( IPPVC_BLOCK_LUMA == blockType ) /* luminance block */
	{
		/*code = k_mp4_ShowBits (ppBitStream,pBitOffset, 11);*/
		code = k_mp4_ShowBits11 (ppBitStream,pBitOffset);
		for (i = 11; i > 3; i--)
		{
			if ( code == 1)
			{
				k_mp4_FlushBits(ppBitStream,pBitOffset, i);
				return (i + 1);
			}
			code >>= 1;
		}
		k_mp4_FlushBits(ppBitStream,pBitOffset,dc_lum_tbl[code][0]);
		return dc_lum_tbl[code][1];
	}
	else /* chrominance block */
	{
		/*code = k_mp4_ShowBits (ppBitStream, pBitOffset,12);*/
		code = k_mp4_ShowBits12 (ppBitStream, pBitOffset);
		for (i = 12; i > 2; i--)
		{
			if ( code == 1)
			{
				k_mp4_FlushBits(ppBitStream,pBitOffset, i);
				return i;
			}
			code >>= 1;
		}
		/*return (3 - k_mp4_GetBits(ppBitStream, pBitOffset,2));*/
		return (3 - k_mp4_GetBit2(ppBitStream, pBitOffset));

	}    
}	/* VlcDecIntraDCPredSize */



Tcoef
VlcDecodeIntraTCOEF(Ipp8u **ppBitStream, int *pBitOffset)
{
	Ipp32u		code;
	Ipp32s		*tab;
	Tcoef		tcoef =	{0, 0, 0};

	/*code = k_mp4_ShowBits (ppBitStream,pBitOffset,12);*/
	code = k_mp4_ShowBits12 (ppBitStream,pBitOffset);

	if (code > 511)
		tab = &DCT3Dtab3[(code >> 5) - 16];
	else if (code > 127)
		tab = &DCT3Dtab4[(code >> 2) - 32];
	else if (code > 7)
		tab = &DCT3Dtab5[(code >> 0) - 8];
	else
	{
		return tcoef;
	}

	k_mp4_FlushBits(ppBitStream,pBitOffset, tab[1]);
	if (tab[0] != 7167)	
	{
		tcoef.run = (tab[0] >> 8) & 255;
		tcoef.level = tab[0] & 255;
		tcoef.last = (tab[0] >> 16) & 1;
		/*tcoef.sign = k_mp4_GetBits(ppBitStream, pBitOffset, 1);        */
		tcoef.sign = k_mp4_GetBit1(ppBitStream, pBitOffset);        
	}
	
	/* the following is modified for 3-mode escape -- boon */
	else		/* if (tab[0] == 7167)	ESCAPE */
	{
		int level_offset;
		/*level_offset = k_mp4_GetBits(ppBitStream, pBitOffset, 1);*/
		level_offset = k_mp4_GetBit1(ppBitStream, pBitOffset);

		if (!level_offset) 
		{
			/* first escape mode. level is offset */
			/*code = k_mp4_ShowBits (ppBitStream,pBitOffset, 12);*/
			code = k_mp4_ShowBits12 (ppBitStream,pBitOffset);

			if (code > 511)
				tab = &DCT3Dtab3[(code >> 5) - 16];
			else if (code >= 128)
				tab = &DCT3Dtab4[(code >> 2) - 32];
			else if (code >= 8)
				tab = &DCT3Dtab5[(code >> 0) - 8];
			else
			{
				return tcoef;
			}

			k_mp4_FlushBits(ppBitStream,pBitOffset, tab[1]);
			tcoef.run = (tab[0] >> 8) & 255;
			tcoef.level = tab[0] & 255;
			tcoef.last = (tab[0] >> 16) & 1;

			/* need to add back the max level */
			tcoef.level = tcoef.level + intra_max_level[tcoef.last][tcoef.run];

			/* sign bit */
			/*tcoef.sign = k_mp4_GetBits(ppBitStream, pBitOffset, 1);       */
			tcoef.sign = k_mp4_GetBit1(ppBitStream, pBitOffset);       
		} 
		else 
		{
			int run_offset;
			/*run_offset = k_mp4_GetBits(ppBitStream, pBitOffset, 1);*/
			run_offset = k_mp4_GetBit1(ppBitStream, pBitOffset);

			if (!run_offset) 
			{
				/* second escape mode. run is offset */
				/*code = k_mp4_ShowBits (ppBitStream,pBitOffset, 12);*/
				code = k_mp4_ShowBits12 (ppBitStream,pBitOffset);

				if (code > 511)
					tab = &DCT3Dtab3[(code >> 5) - 16];
				else if (code > 127)
					tab = &DCT3Dtab4[(code >> 2) - 32];
				else if (code > 7)
					tab = &DCT3Dtab5[(code >> 0) - 8];
				else
				{
					return tcoef;
				}

				k_mp4_FlushBits(ppBitStream,pBitOffset, tab[1]);

				tcoef.run = (tab[0] >> 8) & 255;
				tcoef.level = tab[0] & 255;
				tcoef.last = (tab[0] >> 16) & 1;

				/* need to add back the max run */
				if (tcoef.last)
					tcoef.run = tcoef.run + intra_max_run1[tcoef.level]+1;
				else
					tcoef.run = tcoef.run + intra_max_run0[tcoef.level]+1;				 
				/* sign bit */
				/*tcoef.sign = k_mp4_GetBits(ppBitStream, pBitOffset, 1);  */      
				tcoef.sign = k_mp4_GetBit1(ppBitStream, pBitOffset);        
			} 
			else 
			{
				/* third escape mode. flc */
				/*tcoef.last = k_mp4_GetBits(ppBitStream, pBitOffset, 1);*/
				tcoef.last = k_mp4_GetBit1(ppBitStream, pBitOffset);

				/*tcoef.run = k_mp4_GetBits(ppBitStream, pBitOffset, 6);*/
				tcoef.run = k_mp4_GetBit6(ppBitStream, pBitOffset);

				/* 11.08.98 Sven Brandau: "insert marker_bit" due to N2339, Clause 2.1.21 */
				k_mp4_FlushBits(ppBitStream, pBitOffset, 1);

				/*tcoef.level = k_mp4_GetBits(ppBitStream, pBitOffset, 12);*/
				tcoef.level = k_mp4_GetBit12(ppBitStream, pBitOffset);

				/* 11.08.98 Sven Brandau: "insert marker_bit" due to N2339, Clause 2.1.21 */
				k_mp4_FlushBits(ppBitStream, pBitOffset, 1);

				if (tcoef.level > 2047)
				{
					tcoef.sign = 1;
					tcoef.level = 4096 - tcoef.level;
				}
				else
				{
					tcoef.sign = 0;
				}

			} /* flc */
		}
	}

	return tcoef;
}

#if 0
Tcoef
VlcDecodeInterTCOEF(Ipp8u **ppBitStream, int *pBitOffset, int short_video_header, int h263_flv)
{
	Ipp32u    code;
	Ipp32s  *tab;
	Tcoef   tcoef =	{0, 0, 0};

	/*code = k_mp4_ShowBits (ppBitStream,pBitOffset,12);*/
	code = k_mp4_ShowBits12 (ppBitStream,pBitOffset);

	if (code > 511)
		tab = &DCT3Dtab0[(code >> 5) - 16];
	else if (code > 127)
		tab = &DCT3Dtab1[(code >> 2) - 32];
	else if (code > 7)
		tab = &DCT3Dtab2[(code >> 0) - 8];
	else
	{
		return tcoef;
	}

	k_mp4_FlushBits(ppBitStream,pBitOffset, tab[1]);

	if (tab[0] != 7167)
	{
		tcoef.run = (tab[0] >> 4) & 255;
		tcoef.level = tab[0] & 15;
		tcoef.last = (tab[0] >> 12) & 1;
		/*tcoef.sign = k_mp4_GetBits(ppBitStream, pBitOffset, 1); */
		tcoef.sign = k_mp4_GetBit1(ppBitStream, pBitOffset);
		return tcoef;
	}
	/*if (tab[0] == 7167) ESCAPE */
	if (short_video_header) 
	{              
		if( h263_flv > 1 )
		{
			int is11 = k_mp4_GetBit1(ppBitStream, pBitOffset);;

			tcoef.last = k_mp4_GetBit1(ppBitStream, pBitOffset);          /*  1 bit LAST   */
			tcoef.run  = k_mp4_GetBit6(ppBitStream, pBitOffset);        /*  6 bit RUN    */
			//i+= tcoef.run;                                                  /* go forward "run" digits */
			
			if( is11 )
			{
				tcoef.level = k_mp4_GetBit11(ppBitStream, pBitOffset);       /*  11 bit LEVEL  */
				tcoef.sign = tcoef.level & 1024;                          /* bit 8 is sign bit ==> sign = sign(level); */
				if(tcoef.sign)
				{
					tcoef.sign = 1;
					tcoef.level ^= 0x07ff;              /* == 1s complement */
					tcoef.level ++;                     /* 2s complement if level is negative ==> level = abs(level); */
				}
			}
			else
			{
				tcoef.level = k_mp4_GetBits(ppBitStream, pBitOffset,7);       /*  7 bit LEVEL  */
				tcoef.sign = tcoef.level & 64;                          /* bit 8 is sign bit ==> sign = sign(level); */
				if(tcoef.sign)
				{
					tcoef.sign = 1;
					tcoef.level ^= 0x007f;              /* == 1s complement */
					tcoef.level ++;                     /* 2s complement if level is negative ==> level = abs(level); */
				}
			}
		}
		else
		{

			/* escape mode 4 - H.263 type */
			/*tcoef.last = k_mp4_GetBits(ppBitStream, pBitOffset, 1); */
			tcoef.last = k_mp4_GetBit1(ppBitStream, pBitOffset);

			/*tcoef.run = k_mp4_GetBits(ppBitStream, pBitOffset, 6); */
			tcoef.run = k_mp4_GetBit6(ppBitStream, pBitOffset); 

			/*tcoef.level = k_mp4_GetBits(ppBitStream, pBitOffset, 8); */
			tcoef.level = k_mp4_GetBit8(ppBitStream, pBitOffset);

			if (tcoef.level == 0 || tcoef.level == 128) 
			{
	#ifdef _DEBUG
				printf ("Illegal LEVEL for ESCAPE mode 4: 0 or 128\n");
	#endif
				return tcoef;
			}

			if (tcoef.level > 127) 
			{ 
				tcoef.sign = 1; 
				tcoef.level = 256 - tcoef.level; 
			} 
			else 
			{ 
				tcoef.sign = 0; 
			}
		}
	}
	else 
	{   /* not escape mode 4 */
		int level_offset;
		/*level_offset = k_mp4_GetBits(ppBitStream, pBitOffset, 1);*/
		level_offset = k_mp4_GetBit1(ppBitStream, pBitOffset);

		if (!level_offset) 
		{
			/* first escape mode. level is offset */
			/*code = k_mp4_ShowBits(ppBitStream,pBitOffset, 12);*/
			code = k_mp4_ShowBits12(ppBitStream,pBitOffset);

			if (code > 511)
				tab = &DCT3Dtab0[(code >> 5) - 16];
			else if (code > 127)
				tab = &DCT3Dtab1[(code >> 2) - 32];
			else if (code > 7)
				tab = &DCT3Dtab2[(code >> 0) - 8];
			else
			{
				return tcoef;
			}

			k_mp4_FlushBits(ppBitStream,pBitOffset, tab[1]);

			tcoef.run = (tab[0]>> 4) & 255;
			tcoef.level = tab[0] & 15;
			tcoef.last = (tab[0] >> 12) & 1;

			/* need to add back the max level */
			tcoef.level = tcoef.level + inter_max_level[tcoef.last][tcoef.run];

			/* sign bit */
			/*tcoef.sign = k_mp4_GetBits(ppBitStream, pBitOffset, 1);       */
			tcoef.sign = k_mp4_GetBit1(ppBitStream, pBitOffset);       

		}
		else
		{
			int run_offset;
			/*run_offset = k_mp4_GetBits(ppBitStream, pBitOffset, 1);*/
			run_offset = k_mp4_GetBit1(ppBitStream, pBitOffset);

			if (!run_offset) 
			{
				/* second escape mode. run is offset */
				/*code = k_mp4_ShowBits (ppBitStream,pBitOffset, 12);*/
				code = k_mp4_ShowBits12 (ppBitStream,pBitOffset);

				if (code > 511)
					tab = &DCT3Dtab0[(code >> 5) - 16];
				else if (code > 127)
					tab = &DCT3Dtab1[(code >> 2) - 32];
				else if (code > 7)
					tab = &DCT3Dtab2[(code >> 0) - 8];
				else
				{
					return tcoef;
				}

				k_mp4_FlushBits(ppBitStream,pBitOffset, tab[1]);

				tcoef.run = (tab[0] >> 4) & 255;
				tcoef.level = tab[0] & 15;
				tcoef.last = (tab[0] >> 12) & 1;

				/* need to add back the max run */
				if (tcoef.last)
					tcoef.run = tcoef.run + inter_max_run1[tcoef.level]+1;
				else
					tcoef.run = tcoef.run + inter_max_run0[tcoef.level]+1;

				/* sign bit */
				/*tcoef.sign = k_mp4_GetBits(ppBitStream, pBitOffset, 1);        */
				tcoef.sign = k_mp4_GetBit1(ppBitStream, pBitOffset);        

			}
			else
			{
				/* third escape mode. flc */
				/*tcoef.last = k_mp4_GetBits(ppBitStream, pBitOffset, 1);*/
				tcoef.last = k_mp4_GetBit1(ppBitStream, pBitOffset);

				/*tcoef.run = k_mp4_GetBits(ppBitStream, pBitOffset, 6);*/
				tcoef.run = k_mp4_GetBit6(ppBitStream, pBitOffset);

				k_mp4_FlushBits(ppBitStream, pBitOffset, 1);

				/*tcoef.level = k_mp4_GetBits(ppBitStream, pBitOffset, 12);*/
				tcoef.level = k_mp4_GetBit12(ppBitStream, pBitOffset);

				k_mp4_FlushBits(ppBitStream, pBitOffset, 1);

				if (tcoef.level > 2047)
				{
					tcoef.sign = 1;
					tcoef.level = 4096 - tcoef.level;
				}
				else
				{
					tcoef.sign = 0;
				}

			} /* flc */
		}
	}

	return tcoef;
}
#endif

Tcoef
RvlcDecodeIntraTCOEF(Ipp8u **ppBitStream, int *pBitOffset)
{
	Ipp32u		code, mask; 
	Ipp32s		*tab;
	Tcoef		tcoef =	{0, 0, 0};

    int count, len;
 
    mask = 0x4000;      /* mask  100000000000000   */
    /*code = k_mp4_ShowBits (ppBitStream, pBitOffset,15);*/
    code = k_mp4_ShowBits15 (ppBitStream, pBitOffset);
    len = 1;
     
 
	if (code & mask) {
		count = 1;
		while (count > 0) {
			mask = mask >> 1;
			if (code & mask) 
				count--;
			len++;
		}
	}
	else {
		count = 2;
		while (count > 0) {
			mask = mask >> 1;
			if (!(code & mask))
				count--;
			len++;
		}
	}
    
    code = code & 0x7fff;
    code = code >> (15 - (len + 1));
    
	switch(code) {

	case 0x0:
		tab = &RvlcDCT3Dtab0[169];
		break;

	case 0x1: 
		tab = &RvlcDCT3Dtab0[27];
		break;

	case 0x4: 
		tab = &RvlcDCT3Dtab0[40];
		break;

	case 0x5:
		tab = &RvlcDCT3Dtab0[51];
		break;

	case 0x6:
		tab = &RvlcDCT3Dtab0[0];
		break;

	case 0x7:
		tab = &RvlcDCT3Dtab0[1];
		break;

	case 0x8:
		tab = &RvlcDCT3Dtab0[28];
		break;

	case 0x9:
		tab = &RvlcDCT3Dtab0[3];
		break;

	case 0xa:
		tab = &RvlcDCT3Dtab0[2];
		break;

	case 0xb:
		tab = &RvlcDCT3Dtab0[103];
		break;

	case 0xc:
		tab = &RvlcDCT3Dtab0[60];
		break;

	case 0xd:
		tab = &RvlcDCT3Dtab0[66];
		break;

	case 0x12:
		tab = &RvlcDCT3Dtab0[108];
		break;

	case 0x13:
		tab = &RvlcDCT3Dtab0[113];
		break;

	case 0x14:
		tab = &RvlcDCT3Dtab0[4];
		break;

	case 0x15:
		tab = &RvlcDCT3Dtab0[5];
		break;

	case 0x18:
		tab = &RvlcDCT3Dtab0[116];
		break;

	case 0x19:
		tab = &RvlcDCT3Dtab0[118];
		break;

	case 0x1c:
		tab = &RvlcDCT3Dtab0[72];
		break;

	case 0x1d:
		tab = &RvlcDCT3Dtab0[77];
		break;

	case 0x22:
		tab = &RvlcDCT3Dtab0[120];
		break;

	case 0x23:
		tab = &RvlcDCT3Dtab0[122];
		break;

	case 0x2c:
		tab = &RvlcDCT3Dtab0[41];
		break;

	case 0x2d:
		tab = &RvlcDCT3Dtab0[29];
		break;

	case 0x34:
		tab = &RvlcDCT3Dtab0[6];
		break;

	case 0x35:
		tab = &RvlcDCT3Dtab0[124];
		break;

	case 0x38:
		tab = &RvlcDCT3Dtab0[126];
		break;

	case 0x39:
		tab = &RvlcDCT3Dtab0[128];
		break;

	case 0x3c:
		tab = &RvlcDCT3Dtab0[82];
		break;

	case 0x3d:
		tab = &RvlcDCT3Dtab0[86];
		break;

	case 0x42:
		tab = &RvlcDCT3Dtab0[130];
		break;

	case 0x43:
		tab = &RvlcDCT3Dtab0[132];
		break;

	case 0x5c:
		tab = &RvlcDCT3Dtab0[52];
		break;

	case 0x5d:
		tab = &RvlcDCT3Dtab0[61];
		break;

	case 0x6c:
		tab = &RvlcDCT3Dtab0[30];
		break;

	case 0x6d:
		tab = &RvlcDCT3Dtab0[31];
		break;

	case 0x74:
		tab = &RvlcDCT3Dtab0[7];
		break;

	case 0x75:
		tab = &RvlcDCT3Dtab0[8];
		break;

	case 0x78:
		tab = &RvlcDCT3Dtab0[104];
		break;

	case 0x79:
		tab = &RvlcDCT3Dtab0[134];
		break;

	case 0x7c:
		tab = &RvlcDCT3Dtab0[90];
		break;

	case 0x7d:
		tab = &RvlcDCT3Dtab0[67];
		break;

	case 0x82:
		tab = &RvlcDCT3Dtab0[136];
		break;

	case 0x83:
		tab = &RvlcDCT3Dtab0[138];
		break;

	case 0xbc:
		tab = &RvlcDCT3Dtab0[42];
		break;

	case 0xbd:
		tab = &RvlcDCT3Dtab0[53];
		break;

	case 0xdc:
		tab = &RvlcDCT3Dtab0[32];
		break;

	case 0xdd:
		tab = &RvlcDCT3Dtab0[9];
		break;

	case 0xec:
		tab = &RvlcDCT3Dtab0[10];
		break;

	case 0xed:
		tab = &RvlcDCT3Dtab0[109];
		break;

	case 0xf4:
		tab = &RvlcDCT3Dtab0[139];
		break;

	case 0xf5:
		tab = &RvlcDCT3Dtab0[140];
		break;

	case 0xf8:
		tab = &RvlcDCT3Dtab0[141];
		break;

	case 0xf9:
		tab = &RvlcDCT3Dtab0[142];
		break;

	case 0xfc:
		tab = &RvlcDCT3Dtab0[92];
		break;

	case 0xfd:
		tab = &RvlcDCT3Dtab0[94];
		break;

	case 0x102:
		tab = &RvlcDCT3Dtab0[143];
		break;

	case 0x103:
		tab = &RvlcDCT3Dtab0[144];
		break;

	case 0x17c:
		tab = &RvlcDCT3Dtab0[73];
		break;

	case 0x17d:
		tab = &RvlcDCT3Dtab0[78];
		break;

	case 0x1bc:
		tab = &RvlcDCT3Dtab0[83];
		break;

	case 0x1bd:
		tab = &RvlcDCT3Dtab0[62];
		break;

	case 0x1dc:
		tab = &RvlcDCT3Dtab0[43];
		break;

	case 0x1dd:
		tab = &RvlcDCT3Dtab0[33];
		break;

	case 0x1ec:
		tab = &RvlcDCT3Dtab0[11];
		break;

	case 0x1ed:
		tab = &RvlcDCT3Dtab0[12];
		break;

	case 0x1f4:
		tab = &RvlcDCT3Dtab0[13];
		break;

	case 0x1f5:
		tab = &RvlcDCT3Dtab0[145];
		break;

	case 0x1f8:
		tab = &RvlcDCT3Dtab0[146];
		break;

	case 0x1f9:
		tab = &RvlcDCT3Dtab0[147];
		break;

	case 0x1fc:
		tab = &RvlcDCT3Dtab0[96];
		break;

	case 0x1fd:
		tab = &RvlcDCT3Dtab0[87];
		break;

	case 0x202:
		tab = &RvlcDCT3Dtab0[148];
		break;

	case 0x203:
		tab = &RvlcDCT3Dtab0[149];
		break;

	case 0x2fc:
		tab = &RvlcDCT3Dtab0[68];
		break;

	case 0x2fd:
		tab = &RvlcDCT3Dtab0[74];
		break;

	case 0x37c:
		tab = &RvlcDCT3Dtab0[79];
		break;

	case 0x37d:
		tab = &RvlcDCT3Dtab0[54];
		break;

	case 0x3bc:
		tab = &RvlcDCT3Dtab0[44];
		break;

	case 0x3bd:
		tab = &RvlcDCT3Dtab0[45];
		break;

	case 0x3dc:
		tab = &RvlcDCT3Dtab0[34];
		break;

	case 0x3dd:
		tab = &RvlcDCT3Dtab0[35];
		break;

	case 0x3ec:
		tab = &RvlcDCT3Dtab0[14];
		break;

	case 0x3ed:
		tab = &RvlcDCT3Dtab0[15];
		break;

	case 0x3f4:
		tab = &RvlcDCT3Dtab0[16];
		break;

	case 0x3f5:
		tab = &RvlcDCT3Dtab0[105];
		break;

	case 0x3f8:
		tab = &RvlcDCT3Dtab0[114];
		break;

	case 0x3f9:
		tab = &RvlcDCT3Dtab0[150];
		break;

	case 0x3fc:
		tab = &RvlcDCT3Dtab0[91];
		break;

	case 0x3fd:
		tab = &RvlcDCT3Dtab0[63];
		break;

	case 0x402:
		tab = &RvlcDCT3Dtab0[151];
		break;

	case 0x403:
		tab = &RvlcDCT3Dtab0[152];
		break;

	case 0x5fc:
		tab = &RvlcDCT3Dtab0[69];
		break;

	case 0x5fd:
		tab = &RvlcDCT3Dtab0[75];
		break;

	case 0x6fc:
		tab = &RvlcDCT3Dtab0[55];
		break;

	case 0x6fd:
		tab = &RvlcDCT3Dtab0[64];
		break;

	case 0x77c:
		tab = &RvlcDCT3Dtab0[36];
		break;

	case 0x77d:
		tab = &RvlcDCT3Dtab0[17];
		break;

	case 0x7bc:
		tab = &RvlcDCT3Dtab0[18];
		break;

	case 0x7bd:
		tab = &RvlcDCT3Dtab0[21];
		break;

	case 0x7dc:
		tab = &RvlcDCT3Dtab0[110];
		break;

	case 0x7dd:
		tab = &RvlcDCT3Dtab0[117];
		break;

	case 0x7ec:
		tab = &RvlcDCT3Dtab0[119];
		break;

	case 0x7ed:
		tab = &RvlcDCT3Dtab0[153];
		break;

	case 0x7f4:
		tab = &RvlcDCT3Dtab0[154];
		break;

	case 0x7f5:
		tab = &RvlcDCT3Dtab0[155];
		break;

	case 0x7f8:
		tab = &RvlcDCT3Dtab0[156];
		break;

	case 0x7f9:
		tab = &RvlcDCT3Dtab0[157];
		break;

	case 0x7fc:
		tab = &RvlcDCT3Dtab0[97];
		break;

	case 0x7fd:
		tab = &RvlcDCT3Dtab0[98];
		break;

	case 0x802:
		tab = &RvlcDCT3Dtab0[158];
		break;

	case 0x803:
		tab = &RvlcDCT3Dtab0[159];
		break;

	case 0xbfc:
		tab = &RvlcDCT3Dtab0[93];
		break;

	case 0xbfd:
		tab = &RvlcDCT3Dtab0[84];
		break;

	case 0xdfc:
		tab = &RvlcDCT3Dtab0[88];
		break;

	case 0xdfd:
		tab = &RvlcDCT3Dtab0[80];
		break;

	case 0xefc:
		tab = &RvlcDCT3Dtab0[56];
		break;

	case 0xefd:
		tab = &RvlcDCT3Dtab0[46];
		break;

	case 0xf7c:
		tab = &RvlcDCT3Dtab0[47];
		break;

	case 0xf7d:
		tab = &RvlcDCT3Dtab0[48];
		break;

	case 0xfbc:
		tab = &RvlcDCT3Dtab0[37];
		break;

	case 0xfbd:
		tab = &RvlcDCT3Dtab0[19];
		break;

	case 0xfdc:
		tab = &RvlcDCT3Dtab0[20];
		break;

	case 0xfdd:
		tab = &RvlcDCT3Dtab0[22];
		break;

	case 0xfec:
		tab = &RvlcDCT3Dtab0[106];
		break;

	case 0xfed:
		tab = &RvlcDCT3Dtab0[121];
		break;

	case 0xff4:
		tab = &RvlcDCT3Dtab0[123];
		break;

	case 0xff5:
		tab = &RvlcDCT3Dtab0[125];
		break;

	case 0xff8:
		tab = &RvlcDCT3Dtab0[127];
		break;

	case 0xff9:
		tab = &RvlcDCT3Dtab0[129];
		break;

	case 0xffc:
		tab = &RvlcDCT3Dtab0[99];
		break;

	case 0xffd:
		tab = &RvlcDCT3Dtab0[100];
		break;

	case 0x1002:
		tab = &RvlcDCT3Dtab0[160];
		break;

	case 0x1003:
		tab = &RvlcDCT3Dtab0[161];
		break;

	case 0x17fc:
		tab = &RvlcDCT3Dtab0[101];
		break;

	case 0x17fd:
		tab = &RvlcDCT3Dtab0[85];
		break;

	case 0x1bfc:
		tab = &RvlcDCT3Dtab0[70];
		break;

	case 0x1bfd:
		tab = &RvlcDCT3Dtab0[65];
		break;

	case 0x1dfc:
		tab = &RvlcDCT3Dtab0[71];
		break;

	case 0x1dfd:
		tab = &RvlcDCT3Dtab0[57];
		break;

	case 0x1efc:
		tab = &RvlcDCT3Dtab0[58];
		break;

	case 0x1efd:
		tab = &RvlcDCT3Dtab0[49];
		break;

	case 0x1f7c:
		tab = &RvlcDCT3Dtab0[50];
		break;

	case 0x1f7d:
		tab = &RvlcDCT3Dtab0[38];
		break;

	case 0x1fbc:
		tab = &RvlcDCT3Dtab0[39];
		break;

	case 0x1fbd:
		tab = &RvlcDCT3Dtab0[23];
		break;

	case 0x1fdc:
		tab = &RvlcDCT3Dtab0[24];
		break;

	case 0x1fdd:
		tab = &RvlcDCT3Dtab0[25];
		break;

	case 0x1fec:
		tab = &RvlcDCT3Dtab0[107];
		break;

	case 0x1fed:
		tab = &RvlcDCT3Dtab0[111];
		break;

	case 0x1ff4:
		tab = &RvlcDCT3Dtab0[131];
		break;

	case 0x1ff5:
		tab = &RvlcDCT3Dtab0[133];
		break;

	case 0x1ff8:
		tab = &RvlcDCT3Dtab0[135];
		break;

	case 0x1ff9:
		tab = &RvlcDCT3Dtab0[162];
		break;

	case 0x1ffc:
		tab = &RvlcDCT3Dtab0[26];
		break;

	case 0x1ffd:
		tab = &RvlcDCT3Dtab0[59];
		break;

	case 0x2002:
		tab = &RvlcDCT3Dtab0[163];
		break;

	case 0x2003:
		tab = &RvlcDCT3Dtab0[164];
		break;

	case 0x2ffc:
		tab = &RvlcDCT3Dtab0[76];
		break;

	case 0x2ffd:
		tab = &RvlcDCT3Dtab0[81];
		break;

	case 0x37fc:
		tab = &RvlcDCT3Dtab0[89];
		break;

	case 0x37fd:
		tab = &RvlcDCT3Dtab0[95];
		break;

	case 0x3bfc:
		tab = &RvlcDCT3Dtab0[102];
		break;

	case 0x3bfd:
		tab = &RvlcDCT3Dtab0[112];
		break;

	case 0x3dfc:
		tab = &RvlcDCT3Dtab0[115];
		break;

	case 0x3dfd:
		tab = &RvlcDCT3Dtab0[137];
		break;

	case 0x3efc:
		tab = &RvlcDCT3Dtab0[165];
		break;

	case 0x3efd:
		tab = &RvlcDCT3Dtab0[166];
		break;

	case 0x3f7c:
		tab = &RvlcDCT3Dtab0[167];
		break;

	case 0x3f7d:
		tab = &RvlcDCT3Dtab0[168];
		break;

	default:  
#ifdef _DEBUG
		printf ("Invalid Huffman code in RvlcDecTCOEF().\n");
#endif
		return tcoef;
		break;
	}

    k_mp4_FlushBits (ppBitStream,pBitOffset,tab[1]);

	if (tab[0] != 7167)
	{
		tcoef.run = (tab[0] >> 8) & 255;
		tcoef.level = tab[0] & 255;
		tcoef.last = (tab[0] >> 16) & 1;
		/*tcoef.sign = k_mp4_GetBits(ppBitStream, pBitOffset, 1);        */
		tcoef.sign = k_mp4_GetBit1(ppBitStream, pBitOffset);        
	}
	else 	/* if (tab[0] == 7167) ESCAPE */
	{
		k_mp4_FlushBits (ppBitStream,pBitOffset,1);

		/*tcoef.last = k_mp4_GetBits(ppBitStream, pBitOffset, 1);*/
		tcoef.last = k_mp4_GetBit1(ppBitStream, pBitOffset);

		/*tcoef.run = k_mp4_GetBits(ppBitStream, pBitOffset, 6);*/
		tcoef.run = k_mp4_GetBit6(ppBitStream, pBitOffset);

		k_mp4_FlushBits(ppBitStream, pBitOffset, 1);
		
		/*tcoef.level = k_mp4_GetBits(ppBitStream, pBitOffset, 11);*/
		tcoef.level = k_mp4_GetBit11(ppBitStream, pBitOffset);
		
		k_mp4_FlushBits(ppBitStream, pBitOffset, 1);      

		/*tcoef.sign = k_mp4_GetBits(ppBitStream, pBitOffset, 5);*/
		tcoef.sign = k_mp4_GetBit5(ppBitStream, pBitOffset);
	}

	return tcoef;
}

Tcoef
RvlcDecodeInterTCOEF(Ipp8u **ppBitStream, int *pBitOffset)
{
	Ipp32u		code, mask; 
	Ipp32s		*tab;
	Tcoef		tcoef =	{0, 0, 0};

    int count, len;
 
    mask = 0x4000;      /* mask  100000000000000   */
    /*code = k_mp4_ShowBits (ppBitStream, pBitOffset,15);*/
    code = k_mp4_ShowBits15 (ppBitStream, pBitOffset);
    len = 1;
     
	if (code & mask) {
		count = 1;
		while (count > 0) {
			mask = mask >> 1;
			if (code & mask) 
				count--;
			len++;
		}
	}
	else {
		count = 2;
		while (count > 0) {
			mask = mask >> 1;
			if (!(code & mask))
				count--;
			len++;
		}
	}
    
    code = code & 0x7fff;
    code = code >> (15 - (len + 1));
    
	switch(code) {

	case 0x0:
		tab = &RvlcDCT3Dtab1[169];
		break;

	case 0x1: 
		tab = &RvlcDCT3Dtab1[1];
		break;

	case 0x4: 
		tab = &RvlcDCT3Dtab1[2];
		break;

	case 0x5:
		tab = &RvlcDCT3Dtab1[36];
		break;

	case 0x6:
		tab = &RvlcDCT3Dtab1[0];
		break;

	case 0x7:
		tab = &RvlcDCT3Dtab1[19];
		break;

	case 0x8:
		tab = &RvlcDCT3Dtab1[43];
		break;

	case 0x9:
		tab = &RvlcDCT3Dtab1[48];
		break;

	case 0xa:
		tab = &RvlcDCT3Dtab1[29];
		break;

	case 0xb:
		tab = &RvlcDCT3Dtab1[103];
		break;

	case 0xc:
		tab = &RvlcDCT3Dtab1[20];
		break;

	case 0xd:
		tab = &RvlcDCT3Dtab1[52];
		break;

	case 0x12:
		tab = &RvlcDCT3Dtab1[108];
		break;

	case 0x13:
		tab = &RvlcDCT3Dtab1[113];
		break;

	case 0x14:
		tab = &RvlcDCT3Dtab1[56];
		break;

	case 0x15:
		tab = &RvlcDCT3Dtab1[60];
		break;

	case 0x18:
		tab = &RvlcDCT3Dtab1[116];
		break;

	case 0x19:
		tab = &RvlcDCT3Dtab1[118];
		break;

	case 0x1c:
		tab = &RvlcDCT3Dtab1[3];
		break;

	case 0x1d:
		tab = &RvlcDCT3Dtab1[30];
		break;

	case 0x22:
		tab = &RvlcDCT3Dtab1[120];
		break;

	case 0x23:
		tab = &RvlcDCT3Dtab1[122];
		break;

	case 0x2c:
		tab = &RvlcDCT3Dtab1[63];
		break;

	case 0x2d:
		tab = &RvlcDCT3Dtab1[66];
		break;

	case 0x34:
		tab = &RvlcDCT3Dtab1[68];
		break;

	case 0x35:
		tab = &RvlcDCT3Dtab1[124];
		break;

	case 0x38:
		tab = &RvlcDCT3Dtab1[126];
		break;

	case 0x39:
		tab = &RvlcDCT3Dtab1[128];
		break;

	case 0x3c:
		tab = &RvlcDCT3Dtab1[4];
		break;

	case 0x3d:
		tab = &RvlcDCT3Dtab1[5];
		break;

	case 0x42:
		tab = &RvlcDCT3Dtab1[130];
		break;

	case 0x43:
		tab = &RvlcDCT3Dtab1[132];
		break;

	case 0x5c:
		tab = &RvlcDCT3Dtab1[21];
		break;

	case 0x5d:
		tab = &RvlcDCT3Dtab1[37];
		break;

	case 0x6c:
		tab = &RvlcDCT3Dtab1[44];
		break;

	case 0x6d:
		tab = &RvlcDCT3Dtab1[70];
		break;

	case 0x74:
		tab = &RvlcDCT3Dtab1[72];
		break;

	case 0x75:
		tab = &RvlcDCT3Dtab1[74];
		break;

	case 0x78:
		tab = &RvlcDCT3Dtab1[104];
		break;

	case 0x79:
		tab = &RvlcDCT3Dtab1[134];
		break;

	case 0x7c:
		tab = &RvlcDCT3Dtab1[6];
		break;

	case 0x7d:
		tab = &RvlcDCT3Dtab1[22];
		break;

	case 0x82:
		tab = &RvlcDCT3Dtab1[136];
		break;

	case 0x83:
		tab = &RvlcDCT3Dtab1[138];
		break;

	case 0xbc:
		tab = &RvlcDCT3Dtab1[31];
		break;

	case 0xbd:
		tab = &RvlcDCT3Dtab1[49];
		break;

	case 0xdc:
		tab = &RvlcDCT3Dtab1[76];
		break;

	case 0xdd:
		tab = &RvlcDCT3Dtab1[78];
		break;

	case 0xec:
		tab = &RvlcDCT3Dtab1[80];
		break;

	case 0xed:
		tab = &RvlcDCT3Dtab1[109];
		break;

	case 0xf4:
		tab = &RvlcDCT3Dtab1[139];
		break;

	case 0xf5:
		tab = &RvlcDCT3Dtab1[140];
		break;

	case 0xf8:
		tab = &RvlcDCT3Dtab1[141];
		break;

	case 0xf9:
		tab = &RvlcDCT3Dtab1[142];
		break;

	case 0xfc:
		tab = &RvlcDCT3Dtab1[7];
		break;

	case 0xfd:
		tab = &RvlcDCT3Dtab1[8];
		break;

	case 0x102:
		tab = &RvlcDCT3Dtab1[143];
		break;

	case 0x103:
		tab = &RvlcDCT3Dtab1[144];
		break;

	case 0x17c:
		tab = &RvlcDCT3Dtab1[23];
		break;

	case 0x17d:
		tab = &RvlcDCT3Dtab1[38];
		break;

	case 0x1bc:
		tab = &RvlcDCT3Dtab1[53];
		break;

	case 0x1bd:
		tab = &RvlcDCT3Dtab1[57];
		break;

	case 0x1dc:
		tab = &RvlcDCT3Dtab1[61];
		break;

	case 0x1dd:
		tab = &RvlcDCT3Dtab1[64];
		break;

	case 0x1ec:
		tab = &RvlcDCT3Dtab1[82];
		break;

	case 0x1ed:
		tab = &RvlcDCT3Dtab1[83];
		break;

	case 0x1f4:
		tab = &RvlcDCT3Dtab1[84];
		break;

	case 0x1f5:
		tab = &RvlcDCT3Dtab1[145];
		break;

	case 0x1f8:
		tab = &RvlcDCT3Dtab1[146];
		break;

	case 0x1f9:
		tab = &RvlcDCT3Dtab1[147];
		break;

	case 0x1fc:
		tab = &RvlcDCT3Dtab1[9];
		break;

	case 0x1fd:
		tab = &RvlcDCT3Dtab1[10];
		break;

	case 0x202:
		tab = &RvlcDCT3Dtab1[148];
		break;

	case 0x203:
		tab = &RvlcDCT3Dtab1[149];
		break;

	case 0x2fc:
		tab = &RvlcDCT3Dtab1[24];
		break;

	case 0x2fd:
		tab = &RvlcDCT3Dtab1[32];
		break;

	case 0x37c:
		tab = &RvlcDCT3Dtab1[45];
		break;

	case 0x37d:
		tab = &RvlcDCT3Dtab1[50];
		break;

	case 0x3bc:
		tab = &RvlcDCT3Dtab1[67];
		break;

	case 0x3bd:
		tab = &RvlcDCT3Dtab1[85];
		break;

	case 0x3dc:
		tab = &RvlcDCT3Dtab1[86];
		break;

	case 0x3dd:
		tab = &RvlcDCT3Dtab1[87];
		break;

	case 0x3ec:
		tab = &RvlcDCT3Dtab1[88];
		break;

	case 0x3ed:
		tab = &RvlcDCT3Dtab1[89];
		break;

	case 0x3f4:
		tab = &RvlcDCT3Dtab1[90];
		break;

	case 0x3f5:
		tab = &RvlcDCT3Dtab1[105];
		break;

	case 0x3f8:
		tab = &RvlcDCT3Dtab1[114];
		break;

	case 0x3f9:
		tab = &RvlcDCT3Dtab1[150];
		break;

	case 0x3fc:
		tab = &RvlcDCT3Dtab1[11];
		break;

	case 0x3fd:
		tab = &RvlcDCT3Dtab1[25];
		break;

	case 0x402:
		tab = &RvlcDCT3Dtab1[151];
		break;

	case 0x403:
		tab = &RvlcDCT3Dtab1[152];
		break;

	case 0x5fc:
		tab = &RvlcDCT3Dtab1[33];
		break;

	case 0x5fd:
		tab = &RvlcDCT3Dtab1[39];
		break;

	case 0x6fc:
		tab = &RvlcDCT3Dtab1[54];
		break;

	case 0x6fd:
		tab = &RvlcDCT3Dtab1[58];
		break;

	case 0x77c:
		tab = &RvlcDCT3Dtab1[69];
		break;

	case 0x77d:
		tab = &RvlcDCT3Dtab1[91];
		break;

	case 0x7bc:
		tab = &RvlcDCT3Dtab1[92];
		break;

	case 0x7bd:
		tab = &RvlcDCT3Dtab1[93];
		break;

	case 0x7dc:
		tab = &RvlcDCT3Dtab1[110];
		break;

	case 0x7dd:
		tab = &RvlcDCT3Dtab1[117];
		break;

	case 0x7ec:
		tab = &RvlcDCT3Dtab1[119];
		break;

	case 0x7ed:
		tab = &RvlcDCT3Dtab1[153];
		break;

	case 0x7f4:
		tab = &RvlcDCT3Dtab1[154];
		break;

	case 0x7f5:
		tab = &RvlcDCT3Dtab1[155];
		break;

	case 0x7f8:
		tab = &RvlcDCT3Dtab1[156];
		break;

	case 0x7f9:
		tab = &RvlcDCT3Dtab1[157];
		break;

	case 0x7fc:
		tab = &RvlcDCT3Dtab1[12];
		break;

	case 0x7fd:
		tab = &RvlcDCT3Dtab1[13];
		break;

	case 0x802:
		tab = &RvlcDCT3Dtab1[158];
		break;

	case 0x803:
		tab = &RvlcDCT3Dtab1[159];
		break;

	case 0xbfc:
		tab = &RvlcDCT3Dtab1[14];
		break;

	case 0xbfd:
		tab = &RvlcDCT3Dtab1[15];
		break;

	case 0xdfc:
		tab = &RvlcDCT3Dtab1[26];
		break;

	case 0xdfd:
		tab = &RvlcDCT3Dtab1[40];
		break;

	case 0xefc:
		tab = &RvlcDCT3Dtab1[46];
		break;

	case 0xefd:
		tab = &RvlcDCT3Dtab1[51];
		break;

	case 0xf7c:
		tab = &RvlcDCT3Dtab1[62];
		break;

	case 0xf7d:
		tab = &RvlcDCT3Dtab1[71];
		break;

	case 0xfbc:
		tab = &RvlcDCT3Dtab1[94];
		break;

	case 0xfbd:
		tab = &RvlcDCT3Dtab1[95];
		break;

	case 0xfdc:
		tab = &RvlcDCT3Dtab1[96];
		break;

	case 0xfdd:
		tab = &RvlcDCT3Dtab1[97];
		break;

	case 0xfec:
		tab = &RvlcDCT3Dtab1[106];
		break;

	case 0xfed:
		tab = &RvlcDCT3Dtab1[121];
		break;

	case 0xff4:
		tab = &RvlcDCT3Dtab1[123];
		break;

	case 0xff5:
		tab = &RvlcDCT3Dtab1[125];
		break;

	case 0xff8:
		tab = &RvlcDCT3Dtab1[127];
		break;

	case 0xff9:
		tab = &RvlcDCT3Dtab1[129];
		break;

	case 0xffc:
		tab = &RvlcDCT3Dtab1[16];
		break;

	case 0xffd:
		tab = &RvlcDCT3Dtab1[17];
		break;

	case 0x1002:
		tab = &RvlcDCT3Dtab1[160];
		break;

	case 0x1003:
		tab = &RvlcDCT3Dtab1[161];
		break;

	case 0x17fc:
		tab = &RvlcDCT3Dtab1[27];
		break;

	case 0x17fd:
		tab = &RvlcDCT3Dtab1[28];
		break;

	case 0x1bfc:
		tab = &RvlcDCT3Dtab1[34];
		break;

	case 0x1bfd:
		tab = &RvlcDCT3Dtab1[35];
		break;

	case 0x1dfc:
		tab = &RvlcDCT3Dtab1[41];
		break;

	case 0x1dfd:
		tab = &RvlcDCT3Dtab1[55];
		break;

	case 0x1efc:
		tab = &RvlcDCT3Dtab1[65];
		break;

	case 0x1efd:
		tab = &RvlcDCT3Dtab1[73];
		break;

	case 0x1f7c:
		tab = &RvlcDCT3Dtab1[75];
		break;

	case 0x1f7d:
		tab = &RvlcDCT3Dtab1[77];
		break;

	case 0x1fbc:
		tab = &RvlcDCT3Dtab1[79];
		break;

	case 0x1fbd:
		tab = &RvlcDCT3Dtab1[98];
		break;

	case 0x1fdc:
		tab = &RvlcDCT3Dtab1[99];
		break;

	case 0x1fdd:
		tab = &RvlcDCT3Dtab1[100];
		break;

	case 0x1fec:
		tab = &RvlcDCT3Dtab1[107];
		break;

	case 0x1fed:
		tab = &RvlcDCT3Dtab1[111];
		break;

	case 0x1ff4:
		tab = &RvlcDCT3Dtab1[131];
		break;

	case 0x1ff5:
		tab = &RvlcDCT3Dtab1[133];
		break;

	case 0x1ff8:
		tab = &RvlcDCT3Dtab1[135];
		break;

	case 0x1ff9:
		tab = &RvlcDCT3Dtab1[162];
		break;

	case 0x1ffc:
		tab = &RvlcDCT3Dtab1[18];
		break;

	case 0x1ffd:
		tab = &RvlcDCT3Dtab1[42];
		break;

	case 0x2002:
		tab = &RvlcDCT3Dtab1[163];
		break;

	case 0x2003:
		tab = &RvlcDCT3Dtab1[164];
		break;

	case 0x2ffc:
		tab = &RvlcDCT3Dtab1[47];
		break;

	case 0x2ffd:
		tab = &RvlcDCT3Dtab1[59];
		break;

	case 0x37fc:
		tab = &RvlcDCT3Dtab1[81];
		break;

	case 0x37fd:
		tab = &RvlcDCT3Dtab1[101];
		break;

	case 0x3bfc:
		tab = &RvlcDCT3Dtab1[102];
		break;

	case 0x3bfd:
		tab = &RvlcDCT3Dtab1[112];
		break;

	case 0x3dfc:
		tab = &RvlcDCT3Dtab1[115];
		break;

	case 0x3dfd:
		tab = &RvlcDCT3Dtab1[137];
		break;

	case 0x3efc:
		tab = &RvlcDCT3Dtab1[165];
		break;

	case 0x3efd:
		tab = &RvlcDCT3Dtab1[166];
		break;

	case 0x3f7c:
		tab = &RvlcDCT3Dtab1[167];
		break;

	case 0x3f7d:
		tab = &RvlcDCT3Dtab1[168];
		break;

	default:  
#ifdef _DEBUG
		printf ("Invalid Huffman code in RvlcDecTCOEF().\n");
#endif
		return tcoef;
		break;
	}

    k_mp4_FlushBits (ppBitStream,pBitOffset,tab[1]);
	
	if (tab[0] != 7167)
	{
		tcoef.run = (tab[0] >> 8) & 255;
		tcoef.level = tab[0] & 255;
		tcoef.last = (tab[0] >> 16) & 1;
		/*tcoef.sign = k_mp4_GetBits(ppBitStream, pBitOffset, 1);        */
		tcoef.sign = k_mp4_GetBit1(ppBitStream, pBitOffset);        
	}
	else 	/* if (tab[0] == 7167) ESCAPE */
	{
		k_mp4_FlushBits (ppBitStream,pBitOffset,1);
		/*tcoef.last = k_mp4_GetBits(ppBitStream, pBitOffset, 1);*/
		tcoef.last = k_mp4_GetBit1(ppBitStream, pBitOffset);
		/*tcoef.run = k_mp4_GetBits(ppBitStream, pBitOffset, 6);*/
		tcoef.run = k_mp4_GetBit6(ppBitStream, pBitOffset);

		k_mp4_FlushBits(ppBitStream, pBitOffset, 1);

		/*tcoef.level = k_mp4_GetBits(ppBitStream, pBitOffset, 11)*/;
		tcoef.level = k_mp4_GetBit11(ppBitStream, pBitOffset);

		k_mp4_FlushBits(ppBitStream, pBitOffset, 1);      

		/*tcoef.sign = k_mp4_GetBits(ppBitStream, pBitOffset, 5);*/
		tcoef.sign = k_mp4_GetBit5(ppBitStream, pBitOffset);
	}

	return tcoef;

}



/*-----------------------------------------------------------------------------
*		quarter pixel interpolation about
*----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
Calculation of the quarter sample values
figure 7-32 iso-14496-2:2004(e) page: 342
(A)+   (e)x   (b)o   (f)x  (A)+

(g)x   (h)x   (i)x   (j)x

(c)o   (k)x   (d)o   (l)x

(m)x   (n)x   (o)x   (p)x

(A)+   (e)x   (b)o   (f)x  (A)+    

+: Integer sample position; o: Half sample position; x: Quarter sample position
-----------------------------------------------------------------------------*/

static int QP_CLIP[]=
{
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   1,   2,   3,   4,   5,   6,   7,
  8,   9,  10,  11,  12,  13,  14,  15,  16,  17,  18,  19,
 20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
 32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,
 44,  45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,
 56,  57,  58,  59,  60,  61,  62,  63,  64,  65,  66,  67,
 68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
 80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,
 92,  93,  94,  95,  96,  97,  98,  99, 100, 101, 102, 103,
104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115,
116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139,
140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151,
152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163,
164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187,
188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199,
200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211,
212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235,
236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247,
248, 249, 250, 251, 252, 253, 254, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
};
static int * pQPClip = QP_CLIP+112;


void calc_QP_only_h(const unsigned char *pSrc,int srcStep,int srcWidth,int srcHeight,
			unsigned char *pDst, int dstStep,int dstWidth, int dstHeight, int xFrac, int rounding)
{
 	int i,j;
	Ipp8u pSrcBlock[384];	/* 24 * 16 */
	Ipp8u *pSB;
	Ipp8u *pD, *pS;
	const int mirWidth = 24;

	register int tmp;
	int round, round1 = 1 - rounding;
	
#ifdef WIN32
	unsigned int* p1 ;
	unsigned int* p2 ;
#endif

	pSB = pSrcBlock + 3;
	pS = (Ipp8u *)pSrc;
  	for (j = 0; j < dstHeight; j++)
	{
#ifdef WIN32
		p1 = (unsigned int*)pSB;
		p2 = (unsigned int*)pS;
		for (i = 0; i < dstWidth; i+=4)
		{
			*(p1++) = *(p2++);
		}
		/* pSB[dstWidth] = pS[dstWidth];  move to mirror  loop */
#else
		/*for (i = 0; i < srcWidth; i++)
		{
			pSB[i] = pS[i];
		}*/
		memcpy(pSB,pS,srcWidth);
#endif
		/* do not use this, time consuming 
		pSB[-1] = pS[0];
		pSB[-2] = pS[1];
		pSB[-3] = pS[2];
		pSB[dstWidth+3] = pS[srcWidth-3];
		pSB[dstWidth+2] = pS[srcWidth-2];
		pSB[dstWidth+1] = pS[srcWidth-1];
		*/
		pS += srcStep;
		pSB += mirWidth;
	}

	pSB = pSrcBlock + 3;
	pS = (Ipp8u *)pSrc;
	for (j = 0; j < dstHeight; j++)
	{
		pSB[-1] = pS[0];
		pSB[-2] = pS[1];
		pSB[-3] = pS[2];
		pSB[dstWidth+3] = pS[srcWidth-3];
		pSB[dstWidth+2] = pS[srcWidth-2];
#ifdef WIN32
		pSB[dstWidth+1] = pSB[dstWidth] = pS[dstWidth];
#else 
		pSB[dstWidth+1] = pS[dstWidth];
#endif 
		pS += srcStep;
		pSB += mirWidth;
	}

	round = 128 - rounding;

	pSB = pSrcBlock + 3;
	pD = pDst;
	switch(xFrac)
	{
	case 1:
		for (j = 0; j < dstHeight; j++)
		{
			for (i = 0; i < dstWidth; i++)
			{
				tmp = FILTER(pSB[i]+pSB[i+1],pSB[i-1]+pSB[i+2],pSB[i-2]+pSB[i+3],pSB[i-3]+pSB[i+4],round);
				pD[i] = (pSB[i] + pQPClip[tmp] + round1)>>1;
				/*pD[i] = (pSB[i] + CLIP(tmp) + round1)>>1;*/
			}
			pD += dstStep;
			pSB += mirWidth;
		}
		break;

	case 2:
		for (j = 0; j < dstHeight; j++)
		{
			for (i = 0; i < dstWidth; i++)
			{
				tmp = FILTER(pSB[i]+pSB[i+1],pSB[i-1]+pSB[i+2],pSB[i-2]+pSB[i+3],pSB[i-3]+pSB[i+4],round);
				pD[i] = pQPClip[tmp];
				/*pD[i] =  CLIP(tmp);*/
			}
			pD += dstStep;
			pSB += mirWidth;
		}
		break;

	case 3:
		for (j = 0; j < dstHeight; j++)
		{
			for (i = 0; i < dstWidth; i++)
			{
				tmp = FILTER(pSB[i]+pSB[i+1],pSB[i-1]+pSB[i+2],pSB[i-2]+pSB[i+3],pSB[i-3]+pSB[i+4],round);
				pD[i] = (pSB[i+1] + pQPClip[tmp] + round1)>>1;
				/*pD[i] = (pSB[i+1] + CLIP(tmp) + round1)>>1;*/
			}
			pD += dstStep;
			pSB += mirWidth;
		}
		break;
	default:
		break;
	}

}


void calc_QP_only_v(const unsigned char *pSrc,int srcStep,int srcWidth,int srcHeight,
			unsigned char *pDst, int dstStep,int dstWidth, int dstHeight, int yFrac, int rounding)
{
	int i,j;
	Ipp8u pTmpBlock[368];	/*16 * 23*/
	Ipp8u *pTmp,*pD, *pS;
	int mirHeight;
	const int memWidth = 16;
	const int dstWidth2 = 32;
	const int dstWidth3 = 48;
	const int dstWidth4 = 64;

	register int tmp;
	int round,round1 = 1- rounding;
	
#ifdef WIN32
	unsigned int* p1 ;
	unsigned int* p2 ;
#endif
	pD = pDst;
	pS = (Ipp8u *)pSrc;

	mirHeight = srcHeight + 6;

	pTmp = pTmpBlock + dstWidth3;

  	for (j = 0; j < srcHeight; j++)
	{
#ifdef WIN32
		p1 = (unsigned int*)pTmp;
		p2 = (unsigned int*)pS;
		for (i = 0; i < dstWidth; i+=4)
		{
			*(p1++) = *(p2++);
		}
#else
		/*
		for (i = 0; i < dstWidth; i++)
		{
			pTmp[i] = pS[i];
		}*/
		memcpy(pTmp, pS,dstWidth);
#endif
		pTmp += memWidth;
		pS += srcStep;
	}
	round = 128 - rounding;

	pTmp = pTmpBlock + dstWidth3;

	/*memcpy(pTmp-memWidth, pTmp, dstWidth);
	memcpy(pTmp-dstWidth2, pTmp+memWidth, dstWidth);
	memcpy(pTmp-dstWidth3, pTmp+dstWidth2, dstWidth);
	*/
	for (i = 0; i < dstWidth; i++)
	{
		pTmp[i-memWidth] = pTmp[i];
		pTmp[i-dstWidth2] = pTmp[i+memWidth];
		pTmp[i-dstWidth3] = pTmp[i+dstWidth2];
	}

	pTmp = pTmpBlock + (memWidth * (mirHeight-3));
	
	/*memcpy(pTmp, pTmp-memWidth, dstWidth);
	memcpy(pTmp+memWidth, pTmp-dstWidth2, dstWidth);
	memcpy(pTmp+dstWidth2,pTmp-dstWidth3, dstWidth);
	*/
	for (i = 0; i < dstWidth; i++)
	{
		pTmp[i] = pTmp[i-memWidth];
		pTmp[i+memWidth] = pTmp[i-dstWidth2];
		pTmp[i+dstWidth2] = pTmp[i-dstWidth3];
	}


	pTmp = pTmpBlock + dstWidth3;

	switch(yFrac)
	{
	case 1:
		for (j = 0; j < dstHeight; j++)
		{
			for (i = 0; i < dstWidth; i++)
			{
				tmp = FILTER(pTmp[i]+pTmp[i+memWidth],pTmp[i-memWidth]+pTmp[i+dstWidth2],pTmp[i-dstWidth2]+pTmp[i+dstWidth3],pTmp[i-dstWidth3]+pTmp[i+dstWidth4],round);
				pD[i] = (pQPClip[tmp] + pTmp[i] + round1) >> 1;
				/*pD[i] = (CLIP(tmp) + pTmp[i] + round1) >> 1;*/
			}
			pD += dstStep;
			pTmp += memWidth;
		}
		break;
	case 2:
		for (j = 0; j < dstHeight; j++)
		{
			for (i = 0; i < dstWidth; i++)
			{
				tmp = FILTER(pTmp[i]+pTmp[i+memWidth],pTmp[i-memWidth]+pTmp[i+dstWidth2],pTmp[i-dstWidth2]+pTmp[i+dstWidth3],pTmp[i-dstWidth3]+pTmp[i+dstWidth4],round);
				pD[i] =  pQPClip[tmp];
				/*pD[i] =  CLIP(tmp);*/
			}
			pD += dstStep;
			pTmp += memWidth;
		}
		break;
	case 3:
		for (j = 0; j < dstHeight; j++)
		{
			for (i = 0; i < dstWidth; i++)
			{
				tmp = FILTER(pTmp[i]+pTmp[i+memWidth],pTmp[i-memWidth]+pTmp[i+dstWidth2],pTmp[i-dstWidth2]+pTmp[i+dstWidth3],pTmp[i-dstWidth3]+pTmp[i+dstWidth4],round);
				pD[i] = (pQPClip[tmp] + pTmp[i+memWidth] + round1) >> 1;
				/*pD[i] = (CLIP(tmp) + pTmp[i+memWidth] + round1) >> 1;*/
			}
			pD += dstStep;
			pTmp += memWidth;
		}
		break;
	default:
		break;
	}
}


void calc_QP_bias(const Ipp8u *pSrc, int srcStep, int srcWidth, int srcHeight,
							Ipp8u *pDst,int dstStep,int dstWidth, int dstHeight,
							int xFrac,int yFrac, int rounding)
{
	register int tmp;
 	int i,j,round,round1 = 1 - rounding;
	Ipp8u *pTmp, *pSB, *pD, *pS;
	Ipp8u pSrcBlock[408],pTmpBlock[368];	/* 24 * 17, 16 * 23 */
	const int mirWidth = 24;
	const int memWidth = 16;
	const int dstWidth2 = 32;
	const int dstWidth3 = 48;
	const int dstWidth4 = 64;
	int mirHeight = srcHeight + 6;

#ifdef WIN32
		unsigned int* p1 ;
		unsigned int* p2 ;
#endif

	pSB = pSrcBlock + 3;
	pS = (Ipp8u *)pSrc;

  	for (j = 0; j < srcHeight; j++)
	{
#ifdef WIN32
		p1 = (unsigned int*)pSB;
		p2 = (unsigned int*)pS;
		for (i = 0; i < dstWidth; i+=4)
		{
			*(p1++) = *(p2++);
		}
		/* pSB[dstWidth] = pS[dstWidth];  move to mirror loop */
#else
		/*for (i = 0; i < srcWidth; i++)
		  {
			pSB[i] = pS[i];
		  }
		  */
		memcpy(pSB,pS,srcWidth);
#endif
		/* do not use this, time consuming 
		pSB[-1] = pS[0];
		pSB[-2] = pS[1];
		pSB[-3] = pS[2];
		pSB[dstWidth+3] = pS[srcWidth-3];
		pSB[dstWidth+2] = pS[srcWidth-2];
		pSB[dstWidth+1] = pS[srcWidth-1];
		*/
		pS += srcStep;
		pSB += mirWidth;
	}

	pSB = pSrcBlock + 3;
	pS = (Ipp8u *)pSrc;

	for (j = 0; j < srcHeight; j++)
	{
		pSB[-1] = pS[0];
		pSB[-2] = pS[1];
		pSB[-3] = pS[2];
		pSB[dstWidth+3] = pS[srcWidth-3];
		pSB[dstWidth+2] = pS[srcWidth-2];
#ifdef WIN32
		pSB[dstWidth+1] = pSB[dstWidth] = pS[dstWidth];
#else 
		pSB[dstWidth+1] = pS[dstWidth];
#endif 
		pS += srcStep;
		pSB += mirWidth;

	}

	round = 128 - rounding;

	pTmp = pTmpBlock + dstWidth3;
	pSB = pSrcBlock + 3;
	switch(xFrac)
	{
	case 1:
		for (j = 0; j < srcHeight; j++)
		{
			for (i = 0; i < dstWidth; i++)
			{
				tmp = FILTER(pSB[i]+pSB[i+1],pSB[i-1]+pSB[i+2],pSB[i-2]+pSB[i+3],pSB[i-3]+pSB[i+4],round);
				pTmp[i] = (pSB[i] + pQPClip[tmp] + round1) >>1;
				/*pTmp[i] = (pSB[i] + CLIP(tmp) + round1) >>1;*/
			}
			pTmp += memWidth;
			pSB += mirWidth;

		}
		break;

	case 2:
		for (j = 0; j < srcHeight; j++)
		{
			for (i = 0; i < dstWidth; i++)
			{
				tmp = FILTER(pSB[i]+pSB[i+1],pSB[i-1]+pSB[i+2],pSB[i-2]+pSB[i+3],pSB[i-3]+pSB[i+4],round);
				pTmp[i] =  pQPClip[tmp];
				/*pTmp[i] =  CLIP(tmp);*/
			}
			pTmp += memWidth;
			pSB += mirWidth;
		}
		break;

	case 3:
		for (j = 0; j < srcHeight; j++)
		{
			for (i = 0; i < dstWidth; i++)
			{
				tmp = FILTER(pSB[i]+pSB[i+1],pSB[i-1]+pSB[i+2],pSB[i-2]+pSB[i+3],pSB[i-3]+pSB[i+4],round);
				pTmp[i] = (pSB[i+1] + pQPClip[tmp] + round1)>>1;
				/*pTmp[i] = (pSB[i+1] + CLIP(tmp) + round1)>>1;*/
			}
			pTmp += memWidth;
			pSB += mirWidth;
		}
		break;

	default:
		break;
	}


	pTmp = pTmpBlock + dstWidth3;
	/*memcpy(pTmp-memWidth, pTmp, dstWidth);
	memcpy(pTmp-dstWidth2, pTmp+memWidth, dstWidth);
	memcpy(pTmp-dstWidth3, pTmp+dstWidth2, dstWidth);
	*/
	for (i = 0; i < dstWidth; i++)
	{
		pTmp[i-memWidth] = pTmp[i];
		pTmp[i-dstWidth2] = pTmp[i+memWidth];
		pTmp[i-dstWidth3] = pTmp[i+dstWidth2];
	}

	pTmp = pTmpBlock + (memWidth * (mirHeight-3));
	/*memcpy(pTmp, pTmp-memWidth, dstWidth);
	memcpy(pTmp+memWidth, pTmp-dstWidth2, dstWidth);
	memcpy(pTmp+dstWidth2,pTmp-dstWidth3, dstWidth);
	*/
	for (i = 0; i < dstWidth; i++)
	{
		pTmp[i] = pTmp[i-memWidth];
		pTmp[i+memWidth] = pTmp[i-dstWidth2];
		pTmp[i+dstWidth2] = pTmp[i-dstWidth3];
	}

	pTmp = pTmpBlock + dstWidth3;
	pD = pDst;
	switch(yFrac)
	{
	case 1:
		for (j = 0; j < dstHeight; j++)
		{
			for (i = 0; i < dstWidth; i++)
			{
				tmp = FILTER(pTmp[i]+pTmp[i+memWidth],pTmp[i-memWidth]+pTmp[i+dstWidth2],pTmp[i-dstWidth2]+pTmp[i+dstWidth3],pTmp[i-dstWidth3]+pTmp[i+dstWidth4],round);
				pD[i] = (pQPClip[tmp] + pTmp[i] + round1) >> 1;
				/*pD[i] = (CLIP(tmp) + pTmp[i] + round1) >> 1;*/
			}
			pD += dstStep;
			pTmp += memWidth;
		}
		break;

	case 2:
		for (j = 0; j < dstHeight; j++)
		{
			for (i = 0; i < dstWidth; i++)
			{
				tmp = FILTER(pTmp[i]+pTmp[i+memWidth],pTmp[i-memWidth]+pTmp[i+dstWidth2],pTmp[i-dstWidth2]+pTmp[i+dstWidth3],pTmp[i-dstWidth3]+pTmp[i+dstWidth4],round);
				pD[i] =  pQPClip[tmp];
				/*pD[i] =  CLIP(tmp);*/
			}
			pD += dstStep;
			pTmp += memWidth;
		}
		break;

	case 3:
		for (j = 0; j < dstHeight; j++)
		{
			for (i = 0; i < dstWidth; i++)
			{
				tmp = FILTER(pTmp[i]+pTmp[i+memWidth],pTmp[i-memWidth]+pTmp[i+dstWidth2],pTmp[i-dstWidth2]+pTmp[i+dstWidth3],pTmp[i-dstWidth3]+pTmp[i+dstWidth4],round);
				pD[i] = (pQPClip[tmp] + pTmp[i+memWidth] + round1) >> 1;
				/*pD[i] = (CLIP(tmp) + pTmp[i+memWidth] + round1) >> 1;*/
			}
			pD += dstStep;
			pTmp += memWidth;
		}
		break;

	default:
		break;
	}

}


/* 4x8 block 1/2 interpolation, used in OBMC HP */
void Copy4x8HP_8u_C1R(							
    const Ipp8u* pSrc,
    int          srcStep,
    Ipp8u*       pDst,
    int          dstStep,
    int          acc,
    int          rounding)
{
	int i;
	register int round = 1 - rounding;
	Ipp8u *pS, *pD;

	pS = (Ipp8u*)pSrc;
	pD = pDst;

	if (acc == 0)		/*the a points, every point need not to think the border of block*/
	{
#ifdef WIN32
		unsigned int* p1 ;
		unsigned int* p2 ;
#endif
		for (i = 8; i > 0; i--)
		{
#ifdef WIN32
			p1 = (unsigned int*)pD;
			p2 = (unsigned int*)pS;
			*p1 = *p2;
#else
			pD[0] = pS[0];
			pD[1] = pS[1];
			pD[2] = pS[2];
			pD[3] = pS[3];
#endif
			pS += srcStep;
			pD += dstStep;
		}
	}
	else if (acc == 1)		/* the b points*/
	{
		for (i = 8; i > 0; i--)
		{
			pD[0] = (pS[0] + pS[1] + round)>>1;
			pD[1] = (pS[1] + pS[2] + round)>>1;
			pD[2] = (pS[2] + pS[3] + round)>>1;
			pD[3] = (pS[3] + pS[4] + round)>>1;

			pS += srcStep;
			pD += dstStep;
		}
	}
	else if (acc == 2)		/* the c points*/
	{
		Ipp8u *pSN = (Ipp8u*)pSrc + srcStep;

		for (i = 8; i > 0; i--)
		{
			pD[0] = (pS[0] + pSN[0] + round)>>1;
			pD[1] = (pS[1] + pSN[1] + round)>>1;
			pD[2] = (pS[2] + pSN[2] + round)>>1;
			pD[3] = (pS[3] + pSN[3] + round)>>1;

			pS += srcStep;
			pSN += srcStep;
			pD += dstStep;
		}

	}
	else if (acc == 3)		/* the d points */
	{
		Ipp8u *pSN = (Ipp8u*)pSrc + srcStep;
		round = 2 - rounding;
		for (i = 8; i > 0; i--)
		{
			pD[0] = (pS[0] + pS[1] + pSN[0] + pSN[1] + round)>>2;
			pD[1] = (pS[1] + pS[2] + pSN[1] + pSN[2] + round)>>2;
			pD[2] = (pS[2] + pS[3] + pSN[2] + pSN[3] + round)>>2;
			pD[3] = (pS[3] + pS[4] + pSN[3] + pSN[4] + round)>>2;

			pS += srcStep;
			pSN += srcStep;
			pD += dstStep;
		}	
	}
	
}
	
