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
#include <stdio.h>
#include <math.h>
#include <memory.h>

#ifdef __KINOMA_IPP__

#include "kinoma_avc_defines.h"

#include "ippvc.h"




_DECLSPEC const unsigned short InterPolationTbl20[] = {
    0,   20,   40,   60,   80,  100,  120,  140,  160,  180,
  200,  220,  240,  260,  280,  300,  320,  340,  360,  380,
  400,  420,  440,  460,  480,  500,  520,  540,  560,  580,
  600,  620,  640,  660,  680,  700,  720,  740,  760,  780,
  800,  820,  840,  860,  880,  900,  920,  940,  960,  980,
 1000, 1020, 1040, 1060, 1080, 1100, 1120, 1140, 1160, 1180,
 1200, 1220, 1240, 1260, 1280, 1300, 1320, 1340, 1360, 1380,
 1400, 1420, 1440, 1460, 1480, 1500, 1520, 1540, 1560, 1580,
 1600, 1620, 1640, 1660, 1680, 1700, 1720, 1740, 1760, 1780,
 1800, 1820, 1840, 1860, 1880, 1900, 1920, 1940, 1960, 1980,
 2000, 2020, 2040, 2060, 2080, 2100, 2120, 2140, 2160, 2180,
 2200, 2220, 2240, 2260, 2280, 2300, 2320, 2340, 2360, 2380,
 2400, 2420, 2440, 2460, 2480, 2500, 2520, 2540, 2560, 2580,
 2600, 2620, 2640, 2660, 2680, 2700, 2720, 2740, 2760, 2780,
 2800, 2820, 2840, 2860, 2880, 2900, 2920, 2940, 2960, 2980,
 3000, 3020, 3040, 3060, 3080, 3100, 3120, 3140, 3160, 3180,
 3200, 3220, 3240, 3260, 3280, 3300, 3320, 3340, 3360, 3380,
 3400, 3420, 3440, 3460, 3480, 3500, 3520, 3540, 3560, 3580,
 3600, 3620, 3640, 3660, 3680, 3700, 3720, 3740, 3760, 3780,
 3800, 3820, 3840, 3860, 3880, 3900, 3920, 3940, 3960, 3980,
 4000, 4020, 4040, 4060, 4080, 4100, 4120, 4140, 4160, 4180,
 4200, 4220, 4240, 4260, 4280, 4300, 4320, 4340, 4360, 4380,
 4400, 4420, 4440, 4460, 4480, 4500, 4520, 4540, 4560, 4580,
 4600, 4620, 4640, 4660, 4680, 4700, 4720, 4740, 4760, 4780,
 4800, 4820, 4840, 4860, 4880, 4900, 4920, 4940, 4960, 4980,
 5000, 5020, 5040, 5060, 5080, 5100, 5120, 5140, 5160, 5180,
 5200, 5220, 5240, 5260, 5280, 5300, 5320, 5340, 5360, 5380,
 5400, 5420, 5440, 5460, 5480, 5500, 5520, 5540, 5560, 5580,
 5600, 5620, 5640, 5660, 5680, 5700, 5720, 5740, 5760, 5780,
 5800, 5820, 5840, 5860, 5880, 5900, 5920, 5940, 5960, 5980,
 6000, 6020, 6040, 6060, 6080, 6100, 6120, 6140, 6160, 6180,
 6200, 6220, 6240, 6260, 6280, 6300, 6320, 6340, 6360, 6380,
 6400, 6420, 6440, 6460, 6480, 6500, 6520, 6540, 6560, 6580,
 6600, 6620, 6640, 6660, 6680, 6700, 6720, 6740, 6760, 6780,
 6800, 6820, 6840, 6860, 6880, 6900, 6920, 6940, 6960, 6980,
 7000, 7020, 7040, 7060, 7080, 7100, 7120, 7140, 7160, 7180,
 7200, 7220, 7240, 7260, 7280, 7300, 7320, 7340, 7360, 7380,
 7400, 7420, 7440, 7460, 7480, 7500, 7520, 7540, 7560, 7580,
 7600, 7620, 7640, 7660, 7680, 7700, 7720, 7740, 7760, 7780,
 7800, 7820, 7840, 7860, 7880, 7900, 7920, 7940, 7960, 7980,
 8000, 8020, 8040, 8060, 8080, 8100, 8120, 8140, 8160, 8180,
 8200, 8220, 8240, 8260, 8280, 8300, 8320, 8340, 8360, 8380,
 8400, 8420, 8440, 8460, 8480, 8500, 8520, 8540, 8560, 8580,
 8600, 8620, 8640, 8660, 8680, 8700, 8720, 8740, 8760, 8780,
 8800, 8820, 8840, 8860, 8880, 8900, 8920, 8940, 8960, 8980,
 9000, 9020, 9040, 9060, 9080, 9100, 9120, 9140, 9160, 9180,
 9200, 9220, 9240, 9260, 9280, 9300, 9320, 9340, 9360, 9380,
 9400, 9420, 9440, 9460, 9480, 9500, 9520, 9540, 9560, 9580,
 9600, 9620, 9640, 9660, 9680, 9700, 9720, 9740, 9760, 9780,
 9800, 9820, 9840, 9860, 9880, 9900, 9920, 9940, 9960, 9980,
10000,10020,10040,10060,10080,10100,10120,10140,10160,10180,
10200,10220
};

_DECLSPEC const unsigned short InterPolationTbl5[] = {
    0,    5,   10,   15,   20,   25,   30,   35,   40,   45,
   50,   55,   60,   65,   70,   75,   80,   85,   90,   95,
  100,  105,  110,  115,  120,  125,  130,  135,  140,  145,
  150,  155,  160,  165,  170,  175,  180,  185,  190,  195,
  200,  205,  210,  215,  220,  225,  230,  235,  240,  245,
  250,  255,  260,  265,  270,  275,  280,  285,  290,  295,
  300,  305,  310,  315,  320,  325,  330,  335,  340,  345,
  350,  355,  360,  365,  370,  375,  380,  385,  390,  395,
  400,  405,  410,  415,  420,  425,  430,  435,  440,  445,
  450,  455,  460,  465,  470,  475,  480,  485,  490,  495,
  500,  505,  510,  515,  520,  525,  530,  535,  540,  545,
  550,  555,  560,  565,  570,  575,  580,  585,  590,  595,
  600,  605,  610,  615,  620,  625,  630,  635,  640,  645,
  650,  655,  660,  665,  670,  675,  680,  685,  690,  695,
  700,  705,  710,  715,  720,  725,  730,  735,  740,  745,
  750,  755,  760,  765,  770,  775,  780,  785,  790,  795,
  800,  805,  810,  815,  820,  825,  830,  835,  840,  845,
  850,  855,  860,  865,  870,  875,  880,  885,  890,  895,
  900,  905,  910,  915,  920,  925,  930,  935,  940,  945,
  950,  955,  960,  965,  970,  975,  980,  985,  990,  995,
 1000, 1005, 1010, 1015, 1020, 1025, 1030, 1035, 1040, 1045,
 1050, 1055, 1060, 1065, 1070, 1075, 1080, 1085, 1090, 1095,
 1100, 1105, 1110, 1115, 1120, 1125, 1130, 1135, 1140, 1145,
 1150, 1155, 1160, 1165, 1170, 1175, 1180, 1185, 1190, 1195,
 1200, 1205, 1210, 1215, 1220, 1225, 1230, 1235, 1240, 1245,
 1250, 1255, 1260, 1265, 1270, 1275, 1280, 1285, 1290, 1295,
 1300, 1305, 1310, 1315, 1320, 1325, 1330, 1335, 1340, 1345,
 1350, 1355, 1360, 1365, 1370, 1375, 1380, 1385, 1390, 1395,
 1400, 1405, 1410, 1415, 1420, 1425, 1430, 1435, 1440, 1445,
 1450, 1455, 1460, 1465, 1470, 1475, 1480, 1485, 1490, 1495,
 1500, 1505, 1510, 1515, 1520, 1525, 1530, 1535, 1540, 1545,
 1550, 1555, 1560, 1565, 1570, 1575, 1580, 1585, 1590, 1595,
 1600, 1605, 1610, 1615, 1620, 1625, 1630, 1635, 1640, 1645,
 1650, 1655, 1660, 1665, 1670, 1675, 1680, 1685, 1690, 1695,
 1700, 1705, 1710, 1715, 1720, 1725, 1730, 1735, 1740, 1745,
 1750, 1755, 1760, 1765, 1770, 1775, 1780, 1785, 1790, 1795,
 1800, 1805, 1810, 1815, 1820, 1825, 1830, 1835, 1840, 1845,
 1850, 1855, 1860, 1865, 1870, 1875, 1880, 1885, 1890, 1895,
 1900, 1905, 1910, 1915, 1920, 1925, 1930, 1935, 1940, 1945,
 1950, 1955, 1960, 1965, 1970, 1975, 1980, 1985, 1990, 1995,
 2000, 2005, 2010, 2015, 2020, 2025, 2030, 2035, 2040, 2045,
 2050, 2055, 2060, 2065, 2070, 2075, 2080, 2085, 2090, 2095,
 2100, 2105, 2110, 2115, 2120, 2125, 2130, 2135, 2140, 2145,
 2150, 2155, 2160, 2165, 2170, 2175, 2180, 2185, 2190, 2195,
 2200, 2205, 2210, 2215, 2220, 2225, 2230, 2235, 2240, 2245,
 2250, 2255, 2260, 2265, 2270, 2275, 2280, 2285, 2290, 2295,
 2300, 2305, 2310, 2315, 2320, 2325, 2330, 2335, 2340, 2345,
 2350, 2355, 2360, 2365, 2370, 2375, 2380, 2385, 2390, 2395,
 2400, 2405, 2410, 2415, 2420, 2425, 2430, 2435, 2440, 2445,
 2450, 2455, 2460, 2465, 2470, 2475, 2480, 2485, 2490, 2495,
 2500, 2505, 2510, 2515, 2520, 2525, 2530, 2535, 2540, 2545,
 2550, 2555
};

_DECLSPEC const unsigned char ClipTbl[] = {
	 /* -80 -- -1 */
     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,

	 /* 0 -- 255 */
     0,     1,     2,     3,     4,     5,     6,     7,     
	 8,     9,    10,    11,    12,    13,    14,    15,    
	 16,   17,    18,    19,    20,    21,    22,    23,    
	 24,   25,    26,    27,    28,    29,    30,    31,    
	 32,   33,    34,    35,    36,    37,    38,    39,    
	 40,   41,    42,    43,    44,    45,    46,    47,    
	 48,   49,    50,    51,    52,    53,    54,    55,    
	 56,   57,    58,    59,    60,    61,    62,    63,    
	 64,   65,    66,    67,    68,    69,    70,    71,    
	 72,   73,    74,    75,    76,    77,    78,    79,    
	 80,   81,    82,    83,    84,    85,    86,    87,    
	 88,   89,    90,    91,    92,    93,    94,    95,    
	 96,   97,    98,    99,   100,   101,   102,   103,   
	104,  105,   106,   107,   108,   109,   110,   111,  
	112,  113,   114,   115,   116,   117,   118,   119,  
	120,  121,   122,   123,   124,   125,   126,   127,  
	128,  129,   130,   131,   132,   133,   134,   135,  
	136,  137,   138,   139,   140,   141,   142,   143, 
	144,  145,   146,   147,   148,   149,   150,   151, 
	152,  153,   154,   155,   156,   157,   158,   159,  
	160,  161,   162,   163,   164,   165,   166,   167,   
	168,  169,   170,   171,   172,   173,   174,   175, 
	176,  177,   178,   179,   180,   181,   182,   183,   
	184,  185,   186,   187,   188,   189,   190,   191,
	192,  193,   194,   195,   196,   197,   198,   199, 
	200,  201,   202,   203,   204,   205,   206,   207,  
	208,  209,   210,   211,   212,   213,   214,   215, 
	216,  217,   218,   219,   220,   221,   222,   223, 
	224,  225,   226,   227,   228,   229,   230,   231, 
	232,  233,   234,   235,   236,   237,   238,   239, 
	240,  241,   242,   243,   244,   245,   246,   247,
	248,  249,   250,   251,   252,   253,   254,   255, 
	
	/* 256 -- 344 */
	255,  255,   255,   255,   255,   255,   255,   255,
	255,  255,   255,   255,   255,   255,   255,   255,
	255,  255,   255,   255,   255,   255,   255,   255,
	255,  255,   255,   255,   255,   255,   255,   255,
	255,  255,   255,   255,   255,   255,   255,   255,
	255,  255,   255,   255,   255,   255,   255,   255, 
	255,  255,   255,   255,   255,   255,   255,   255, 
	255,  255,   255,   255,   255,   255,   255,   255, 
	255,  255,   255,   255,   255,   255,   255,   255,
	255,  255,   255,   255,   255,   255,   255,   255,
	255,  255,   255,   255,   255,   255,   255,   255
};

#define IClip2(x)    ClipTbl[(x)+80]


IppStatus __STDCALL ippiInterpolateLuma_H264_8u_C1R(
	  const Ipp8u*   pSrc,
			Ipp32s   srcStep,
			Ipp8u*   pDst,
			Ipp32s   dstStep,
			Ipp32s   dx,
			Ipp32s   dy,
			IppiSize roiSize)			// Must be 16,8 or 4
{
	_DECLSPEC int	temp_res[25][25];

	int		width  = roiSize.width;
	int		height = roiSize.height;
	int		i, j, k;

	const unsigned char	*psrc;
	unsigned char   *pdst;

	const unsigned int	*pSrcint;
	unsigned int   *pDstint;

	int		temp_result;

	int		dxx = dx>> 1;
	int		dyy = dy>> 1;

	int		casedxy = (dy<<2) + dx;


	int		srcStep_2 = srcStep << 1;
	int		dstStep_2 = dstStep << 1;

	int		srcStep_3 = srcStep_2 + srcStep;
	int		srcStep_4 = srcStep_2 + srcStep_2;
	int		srcStep_5 = srcStep_2 + srcStep_3;


	static const int COEF[6] = {    1, -5, 20, 20, -5, 1  };

	unsigned char	tmpvalue;

	// Check parameters
	if((height%4) || (width%4))
		return ippStsSizeErr;
	if(pSrc==NULL || pDst==NULL)
		return ippStsNullPtrErr;
	if(dx<0 || dx>3 || dy<0 || dy>3)
		return ippStsBadArgErr;
	if(srcStep < 16)
		return ippStsStepErr;


	switch(casedxy)
	{

		case 0:		// dx =0 && dy =0	
			psrc = pSrc;
			pdst = pDst;

			for(j=0; j<height; j++)
			{
#ifdef WIN32			// This is only valid and effective in INTEL and WIndows System -- WWD in 2006-06-10
				pSrcint = (unsigned int *) psrc;
				pDstint = (unsigned int *) pdst;				
				for(i=0; i<width; i+=4)
					*(pDstint++) = *(pSrcint++);				
#else
				memcpy( pdst, psrc, width);
#endif

				psrc += srcStep; pdst += dstStep;
			}

			break;
		case 1:		// dx = 1 && dy = 0
		case 3:		// dx = 3 && dy = 0
				psrc = pSrc - 4;
				pdst = pDst;
				{
				#ifdef LARGE_CACHE
					for(j=0; j<height; j++)
					{
						for(i=0; i<width; i++)
						{
							temp_result  = 16 + psrc[i +2 ] - InterPolationTbl5[psrc[i +3]+ psrc[i +6] ] + InterPolationTbl20[psrc[i+4] + psrc[i +5]] + psrc[i +7];
							tmpvalue = IClip2(temp_result>> 5);
							pdst[i] = (tmpvalue + psrc[i+4+dxx] + 1) >> 1;
							
						}
						psrc += srcStep;
						pdst += dstStep;
					}
				#else
					for(j=0; j<height; j++)
					{
						for(i=0; i<width; i++)
						{					
							temp_result  = 16 + psrc[i +2 ] - 5*psrc[i +3] + (psrc[i+4] + psrc[i +5]) *20 - 5*psrc[i +6] + psrc[i +7];
							//temp_result  = 16 + psrc[i -2 ] - InterPolationTbl5[psrc[i -1]] + InterPolationTbl20[psrc[i ]] + InterPolationTbl20[psrc[i +1]] - InterPolationTbl5[psrc[i +2]] + psrc[i +3];
							tmpvalue = IClip2(temp_result>> 5);
							
							temp_res[j][i] = (tmpvalue + psrc[i+4+dxx] + 1)>>1;
						}
						psrc += srcStep;
					}

					// Write Result
					for(j=0; j<height; j++)
					{
						for(i=0; i<width; i++)
						{					
							pdst[i] = temp_res[j][i];
						}
						pdst += dstStep;
					}
				#endif
				}

			break;

		case 2:		// dx = 2 && dy = 0
			psrc = pSrc - 4;
			pdst = pDst;
			{
			#ifdef LARGE_CACHE
				for(j=0; j<height; j++)
				{
					for(i=0; i<width; i++)
					{			
						//temp_result  = 16 + psrc[i +2 ] - 5*psrc[i +3] + (psrc[i+4] + psrc[i +5]) *20 - 5*psrc[i +6] + psrc[i +7];
						temp_result  = 16 + psrc[i +2 ] - InterPolationTbl5[psrc[i +3]+ psrc[i +6] ] + InterPolationTbl20[psrc[i+4] + psrc[i +5]] + psrc[i +7];
						pdst[i] = IClip2((temp_result ) >> 5);
					}
					psrc += srcStep;
					pdst += dstStep;
				}
			#else
				for(j=0; j<height; j++)
				{
					for(i=0; i<width; i++)
					{			
						temp_res[j][i]  = 16 + psrc[i +2 ] - 5*psrc[i +3] + (psrc[i+4] + psrc[i +5]) *20 - 5*psrc[i +6] + psrc[i +7];							
					}
					psrc += srcStep;
				}
				for(j=0; j<height; j++)
				{
					for(i=0; i<width; i++)
					{			
						pdst[i] = IClip2((temp_res[j][i] ) >> 5);
					}
					pdst += dstStep;
				}
			#endif

			}

		break;

		case 4:			// dx = 0 && dy = 1
		case 12:		// dx = 0 && dy = 3
			psrc = pSrc-srcStep_2;
			pdst = pDst;					
			{
			#ifdef LARGE_CACHE
				for(j=0; j<height; j++)
				{
					for(i=0; i<width; i++)
					{					
						//temp_result  = 16 + psrc[i] - 5*psrc[i +srcStep] + (psrc[i+srcStep_2 ] + psrc[i +srcStep_3]) *20 - 5*psrc[i +srcStep_4] + psrc[i +srcStep_5];
						temp_result  = 16 + psrc[i] - InterPolationTbl5[psrc[i +srcStep]+ psrc[i +srcStep_4] ] + InterPolationTbl20[psrc[i+srcStep_2 ] + psrc[i +srcStep_3]] + psrc[i +srcStep_5];
							
						pdst[i] = IClip2((temp_result) >> 5);

						pdst[i] = (pdst[i] + psrc[i+dyy*srcStep + srcStep_2] + 1) >> 1;
					}
					psrc += srcStep;
					pdst += dstStep;
				}
			#else
				for(j=0; j<height; j++)
				{
					for(i=0; i<width; i++)
					{					
						temp_result  = 16 + psrc[i] - 5*psrc[i +srcStep] + (psrc[i+srcStep_2 ] + psrc[i +srcStep_3]) *20 - 5*psrc[i +srcStep_4] + psrc[i +srcStep_5];
							
						tmpvalue = IClip2((temp_result) >> 5);

						temp_res[j][i] = (tmpvalue + psrc[i+dyy*srcStep+srcStep_2] + 1) >> 1;
					}
					psrc += srcStep;
				}
				for(j=0; j<height; j++)
				{
					for(i=0; i<width; i++)
					{					
						pdst[i] = temp_res[j][i];
					}
					pdst += dstStep;
				}
			#endif
			}
			break;

		case 8:			// dx = 0 && dy = 2
			psrc = pSrc - srcStep_2;
			pdst = pDst;				
			{
			#ifdef LARGE_CACHE
				for(j=0; j<height; j++)
				{
					for(i=0; i<width; i++)
					{
						//temp_result  = 16 + psrc[i] - 5*psrc[i +srcStep] + (psrc[i+srcStep_2 ] + psrc[i +srcStep_3]) *20 - 5*psrc[i +srcStep_4] + psrc[i +srcStep_5];
						temp_result  = 16 + psrc[i] - InterPolationTbl5[psrc[i +srcStep]+ psrc[i +srcStep_4] ] + InterPolationTbl20[psrc[i+srcStep_2 ] + psrc[i +srcStep_3]] + psrc[i +srcStep_5];

						pdst[i] = IClip2((temp_result) >> 5);
					}
					psrc += srcStep;
					pdst += dstStep;
				}
			#else
				for(j=0; j<height; j++)
				{
					for(i=0; i<width; i++)
						temp_res[j][i]  = 16 + psrc[i] - 5*psrc[i +srcStep] + (psrc[i+srcStep_2 ] + psrc[i +srcStep_3]) *20 - 5*psrc[i +srcStep_4] + psrc[i +srcStep_5];

					psrc += srcStep;
				}
				for(j=0; j<height; j++)
				{
					for(i=0; i<width; i++)
						pdst[i] = IClip2((temp_res[j][i]) >> 5);
					pdst += dstStep;
				}
			#endif
			}
			break;
		case 6:		// dx = 2 && dy = 1
		case 14:	// dx = 2 && dy = 3
				// Prepair data in H filter: Compute 1/2 position for H 
			{
			// Prepair data in H filter: Compute 1/2 position for H 
			#ifdef LARGE_CACHE
				psrc = pSrc - srcStep_2 - 4;
				for(j=0; j<(height+6); j++)
				{
					for(i=0; i<width; i++)
						//temp_res[j][i]  = psrc[i +2 ] - 5*psrc[i +3] + 20*(psrc[i+4] + psrc[i +5])  - 5*psrc[i +6] + psrc[i +7];
						temp_res[j][i]  = psrc[i +2 ] - InterPolationTbl5[psrc[i +3] + psrc[i +6]]+ InterPolationTbl20[psrc[i+4] + psrc[i +5]]  + psrc[i +7];
					psrc += srcStep;
				}

				// do V filter: compute 1/2 position for V -- But just in position (2,2) ONLY, no vertical 1/2 position was computed based on integer vertical position
				pdst = pDst;
				for(j=0; j<height; j++)
				{
					for(i=0; i<width; i++)
					{
						temp_result = 512 + temp_res[j][i] -5*temp_res[j + 1][i] + 20*(temp_res[j + 2][i] + temp_res[j + 3][i]) - 5*temp_res[j + 4][i] + temp_res[j + 5][i];
							
						pdst[i] = IClip(0, 255, (temp_result ) >> 10);
						pdst[i] = (pdst[i] + IClip(0, 255, ((temp_res[j+dyy+2][i] + 16)>>5)) + 1) >> 1;
					}

					pdst += dstStep;
				}
			#else
				psrc = pSrc - srcStep_2 - 4;
				for(j=0; j<(height+6); j++)
				{
					for(i=0; i<width; i++)
						temp_res[j][i]  = psrc[i +2 ] - 5*psrc[i +3] + 20*(psrc[i+4] + psrc[i +5])  - 5*psrc[i +6] + psrc[i +7];
					psrc += srcStep;
				}

				// do V filter: compute 1/2 position for V -- But just in position (2,2) ONLY, no vertical 1/2 position was computed based on integer vertical position
				pdst = pDst;
				for(j=0; j<height; j++)
				{
					for(i=0; i<width; i++)
					{
						temp_result = 512 + temp_res[j][i] -5*temp_res[j + 1][i] + 20*(temp_res[j + 2][i] + temp_res[j + 3][i]) - 5*temp_res[j + 4][i] + temp_res[j + 5][i];
						
						pdst[i] = IClip(0, 255, (temp_result ) >> 10);
						pdst[i] = (pdst[i] + IClip(0, 255, ((temp_res[j+dyy+2][i] + 16)>>5)) + 1) >> 1;
					}

					pdst += dstStep;
				}
			#endif

			}

			break;

		case 10:	// dx = 2 && dy = 2
				// Prepair data in H filter: Compute 1/2 position for H 
			{
				psrc = pSrc -2 - srcStep_2;
			// Prepair data in V filter
			#ifdef LARGE_CACHE
				for(j=0; j<height; j++)
				{
					for(i=0; i<width+6; i++)
					{
						temp_result = 0;
						for(k=0; k<6; k++)
							temp_result += psrc[i +k*srcStep] * COEF[k];						
							
						temp_res[j][i] = temp_result;
					}
					psrc += srcStep;
				}

				// do H filter
				pdst = pDst;
				for(j=0; j<height; j++)
				{
					for(i=0; i<width; i++)
					{
						temp_result = 512;

						for(k=0; k<6; k++)
							temp_result += temp_res[j][i + k] * COEF[k];
						
							
						pdst[i] = IClip(0, 255, (temp_result) >> 10);
					}
					pdst += dstStep;
				}
		#else
				for(j=0; j<height; j++)
				{
					for(i=0; i<width+6; i++)
					{
						temp_result =0;
						for(k=0; k<6; k++)
							temp_result += psrc[i +k*srcStep] * COEF[k];						
							
						temp_res[j][i] = temp_result;
					}
					psrc += srcStep;
				}

				// do H filter
				pdst = pDst;
				for(j=0; j<height; j++)
				{
					for(i=0; i<width; i++)
					{
						temp_result = 512;

						for(k=0; k<6; k++)
							temp_result += temp_res[j][i + k] * COEF[k];
						
							
						pdst[i] = IClip(0, 255, (temp_result) >> 10);
					}
					pdst += dstStep;
				}
		#endif
			}

			break;

		case 9:		// dx = 1 && dy = 2
		case 11:	// dx = 3 && dy = 2
				// Prepair data in V filter
			{
				// Prepair data in V filter
				psrc = pSrc - srcStep_2 -2;
				for(j=0; j<height; j++)
				{
					// V filter
					for(i=0; i<width+6; i++)
					{
						temp_result =0;

						for(k=0; k<6; k++)
							temp_result += psrc[i + k*srcStep] * COEF[k];						
							
						temp_res[j][i] = temp_result;
					}
					psrc += srcStep;
				}

				// do H filter
				pdst = pDst ;
				for(j=0; j<height; j++)
				{
					for(i=0; i<width; i++)
					{
						temp_result = 512;
						for(k=0; k<6; k++)
							temp_result += temp_res[j][i + k] * COEF[k];
						
							
						pdst[i] = IClip(0, 255, (temp_result) >> 10);
						pdst[i] = (pdst[i] + IClip(0, 255, ((temp_res[j][i+dxx+2]+16)>>5)) + 1) >> 1;

					}
					pdst += dstStep;
				}
			}

			break;

		case 5:
		case 7:
		case 13:
		case 15:
			{
				/* Algorithm 
				 *           G      b       H
				 *           d   e  f   g   
				 *           h   i  J   k   m
				 *           n   p  q   r
				 *           M      s       N
				 *
				 *
				*/
				// 以下的算法首先计算b/s，但是根据dyy则可能b不需要计算
				 /* Diagonal interpolation */
				// 这里需要进一步区分e,g  与 p, r （Spec）
				psrc = pSrc + (dyy)*srcStep -2;

				for (j = 0; j < height; j++) 
				{
					for (i = 0; i < width; i++) 
					{
						temp_result = 16;
						for (k = 0; k < 6; k++)
							temp_result += psrc[i + k] * COEF[k];
						
						temp_res[j][i] = IClip2((temp_result )>>5);
					}

					psrc += srcStep;
				}

				// 以下计算h/m，但是根据dxx则可能h不需要计算
				pdst = pDst;
				psrc = pSrc + dxx - srcStep_2;

				for (j = 0; j < height; j++) 
				{
					for (i = 0; i < width; i++) 
					{
						temp_result = 16;
						for (k = 0; k < 6; k++)
							temp_result += psrc[i + (k)*srcStep] * COEF[k];

						pdst[i] = (temp_res[j][i] + IClip2((temp_result )>>5) + 1) >> 1;

					}
					pdst += dstStep;
					psrc += srcStep;
				}

			}

			break;
	
	}


    return ippStsNoErr;
}


/*******************************************************************************
 * Function Name : k_ippiInterpolateChroma_H264_8u_C1R
 * Description   : Make interpolation for Chroma components
 * Author        : Wang Wendong
 * Version       : 0.1
 ******************************************************************************/
//1， 本函数进一步优化的策略在于roiSize中的数据 -- 将循环展开
//2， 将dx*dy事先计算好
//3， 检查精度问题
IppStatus  __STDCALL ippiInterpolateChroma_H264_8u_C1R(
		const   Ipp8u*   pSrc,
				Ipp32s   srcStep,
				Ipp8u*   pDst,
				Ipp32s   dstStep,
				Ipp32s   dx,
				Ipp32s   dy,
				IppiSize roiSize)
{
	int		width  = roiSize.width;
	int		height = roiSize.height;
	int		i, j;

	int		srcstep2 = srcStep << 1;
	int		dststep2 = dstStep << 1;

	unsigned int	tmpvalue0;
	int		dx8, dy8, dxy, dxy8;


	const unsigned char	*psrc= pSrc;
	unsigned char   *pdst= pDst;

	// Please note : roiSize must be 2^n : 8/4/2
	if((dx==0)&&(dy==0))
	{
		for(j=0; j<height; j++)
		{
			memcpy(pdst, psrc, width);
			psrc += srcStep;
			pdst += dstStep;
		}

	}
	else
	{
		if(dy == 0)
		{
			dx8 = 8 - dx;		// dx8 = [1, 7], dx = [1,7]
			for(j=0; j<height; j++)
			{
				for(i=0; i<width; i+=2)
				{
					tmpvalue0 = psrc[i+1]*dx;
					pdst[i]   = (psrc[i]*dx8 + tmpvalue0 + 4) >> 3;
					
					pdst[i+1] = ((psrc[i+1]<<3) - tmpvalue0 + psrc[i+2]*dx + 4) >> 3;
				}
				psrc += srcStep;
				pdst += dstStep;
			}
		}
		else
			if(dx == 0)
			{
				dy8 = 8 - dy;	// dy8=[1,7],  dy=[1,7]
				for(j=0; j<height; j++)
				{
					for(i=0; i<width; i+=2)
					{
						pdst[i]   = (psrc[i  ]*dy8 + psrc[i + srcStep  ]*dy + 4) >> 3;
						pdst[i+1] = (psrc[i+1]*dy8 + psrc[i + srcStep+1]*dy + 4) >> 3;

					}
					psrc += srcStep;
					pdst += dstStep;
				}
			}
			else
			{
				dx8 = dy*(8 - dx);
				dxy = dx*dy;
				dy8 = dx*(8 - dy);
				dxy8= (8-dx)*(8-dy);
#if 1
				for(j=0; j<height; j++)
				{
					for(i=0; i<width; i+=2)
					{
						pdst[i]   = (psrc[i  ]*dxy8 + psrc[i+1]*dy8 + psrc[i + srcStep  ]*dx8 + psrc[i+1+srcStep]*dxy + 32) >> 6;
						pdst[i+1] = (psrc[i+1]*dxy8 + psrc[i+2]*dy8 + psrc[i + srcStep+1]*dx8 + psrc[i+2+srcStep]*dxy + 32) >> 6;

					}
					psrc += srcStep;
					pdst += dstStep;
				}
#else
				for(j=0; j<height; j++)
				{
					for(i=0; i<width; i+=2)
					{
						tmp_res[j][i]   = (psrc[i  ]*dxy8 + psrc[i+1]*dy8 + psrc[i + srcStep  ]*dx8 + psrc[i+1+srcStep]*dxy + 32) >> 6;
						tmp_res[j][i+1] = (psrc[i+1]*dxy8 + psrc[i+2]*dy8 + psrc[i + srcStep+1]*dx8 + psrc[i+2+srcStep]*dxy + 32) >> 6;

					}
					for(i=0; i<width; i+=2)
					{
						pdst[i]   = tmp_res[j][i]  ;
						pdst[i+1] = tmp_res[j][i+1];

					}
					pdst += dstStep;

					psrc += srcStep;
				}
#endif
			}

	}

    return ippStsNoErr;

}

/*******************************************************************************
 * Function Name : k_ippiInterpolateBlock_H264_8u_P2P1R
 * Description   : Average data in two streams (A + B +1)/2
 * Author        : Wang Wendong
 * Version       : 0.1
 ******************************************************************************/
IppStatus __STDCALL ippiInterpolateBlock_H264_8u_P2P1R(
			Ipp8u *pSrc1,
			Ipp8u *pSrc2,
			Ipp8u *pDst,
			Ipp32u uWidth,
			Ipp32u uHeight,
			Ipp32u pitch)
{
	unsigned int	j;
	register int	position;
	//register int	pitchdiff = pitch - uWidth;
	//unsigned char *pdst = pDst;

	position = 0;

	// uWidth and uHeight MUST be 2^n (n>=1)
	if(uWidth == 2)
	{
		for(j=0; j<uHeight; j++)
		{

			pDst[position  ] = (pSrc1[position  ] + pSrc2[position  ] + 1) >> 1;
			pDst[position+1] = (pSrc1[position+1] + pSrc2[position+1] + 1) >> 1;

			position += pitch;

		}
	}
	else
		if(uWidth == 4)
		{
			for(j=0; j<uHeight; j++)
			{

				pDst[position  ] = (pSrc1[position  ] + pSrc2[position  ] + 1) >> 1;
				pDst[position+1] = (pSrc1[position+1] + pSrc2[position+1] + 1) >> 1;
				pDst[position+2] = (pSrc1[position+2] + pSrc2[position+2] + 1) >> 1;
				pDst[position+3] = (pSrc1[position+3] + pSrc2[position+3] + 1) >> 1;

				position += pitch;

			}

		}
		else
			if(uWidth == 8)
			{
				for(j=0; j<uHeight; j++)
				{
					pDst[position  ] = (pSrc1[position  ] + pSrc2[position  ] + 1) >> 1;
					pDst[position+1] = (pSrc1[position+1] + pSrc2[position+1] + 1) >> 1;
					pDst[position+2] = (pSrc1[position+2] + pSrc2[position+2] + 1) >> 1;
					pDst[position+3] = (pSrc1[position+3] + pSrc2[position+3] + 1) >> 1;
					pDst[position+4] = (pSrc1[position+4] + pSrc2[position+4] + 1) >> 1;
					pDst[position+5] = (pSrc1[position+5] + pSrc2[position+5] + 1) >> 1;
					pDst[position+6] = (pSrc1[position+6] + pSrc2[position+6] + 1) >> 1;
					pDst[position+7] = (pSrc1[position+7] + pSrc2[position+7] + 1) >> 1;

					position += pitch;
				}

			}
			else
			{
				// uWidth == 16)
				for(j=0; j<uHeight; j++)
				{

					pDst[position  ] = (pSrc1[position  ] + pSrc2[position  ] + 1) >> 1;
					pDst[position+1] = (pSrc1[position+1] + pSrc2[position+1] + 1) >> 1;
					pDst[position+2] = (pSrc1[position+2] + pSrc2[position+2] + 1) >> 1;
					pDst[position+3] = (pSrc1[position+3] + pSrc2[position+3] + 1) >> 1;
					pDst[position+4] = (pSrc1[position+4] + pSrc2[position+4] + 1) >> 1;
					pDst[position+5] = (pSrc1[position+5] + pSrc2[position+5] + 1) >> 1;
					pDst[position+6] = (pSrc1[position+6] + pSrc2[position+6] + 1) >> 1;
					pDst[position+7] = (pSrc1[position+7] + pSrc2[position+7] + 1) >> 1;

					pDst[position+ 8] = (pSrc1[position+ 8] + pSrc2[position+ 8] + 1) >> 1;
					pDst[position+ 9] = (pSrc1[position+ 9] + pSrc2[position+ 9] + 1) >> 1;
					pDst[position+10] = (pSrc1[position+10] + pSrc2[position+10] + 1) >> 1;
					pDst[position+11] = (pSrc1[position+11] + pSrc2[position+11] + 1) >> 1;
					pDst[position+12] = (pSrc1[position+12] + pSrc2[position+12] + 1) >> 1;
					pDst[position+13] = (pSrc1[position+13] + pSrc2[position+13] + 1) >> 1;
					pDst[position+14] = (pSrc1[position+14] + pSrc2[position+14] + 1) >> 1;
					pDst[position+15] = (pSrc1[position+15] + pSrc2[position+15] + 1) >> 1;

					position += pitch;

				}

			}

#if 0   // Correct version for all situation
		for(j=0; j<uHeight; j++)
		{

			for(i=0; i<uWidth; i+=2)
			{

				pdst[position  ] = (pSrc1[position  ] + pSrc2[position  ] + 1) >> 1;
				pdst[position+1] = (pSrc1[position+1] + pSrc2[position+1] + 1) >> 1;

				position += 2;

			}

			position += pitchdiff;

		}
#endif

    return ippStsNoErr;
}

#endif
