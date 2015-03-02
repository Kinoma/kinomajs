/*
     Copyright (C) 2010-2015 Marvell International Ltd.
     Copyright (C) 2002-2010 Kinoma, Inc.

     Licensed under the Apache License, Version 2.0 (the "License");
     you may not use this file except in compliance with the License.
     You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

     Unless required by applicable law or agreed to in writing, software
     distributed under the License is distributed on an "AS IS" BASIS,
     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
     See the License for the specific language governing permissions and
     limitations under the License.
*/
#include "FskYUV422toYUV420.h"



#ifndef PRAGMA_MARK_SUPPORTED
	#if defined(macintosh)
		#define PRAGMA_MARK_SUPPORTED 1
	#else /*  !PRAGMA_MARK_SUPPORTED */
		#define PRAGMA_MARK_SUPPORTED 0
	#endif /*  !PRAGMA_MARK_SUPPORTED */
#endif /*  PRAGMA_MARK_SUPPORTED */



/* These are related by powers of two, and can be converted simply by shifting */
#define C256toC128(c)		((UInt8)((c) >> 1))
#define C224toC112(c)		((UInt8)((c) >> 1))
#define C1024toC128(c)		(UInt8)((((c) + (1 << (3-1))) >> 3))
#define CS224toC112(c)		(((UInt8)((c) >> 1)) + 128)
#define CS896toC112(c)		((UInt8)((((c) + (1 << (3-1))) >> 3)) + 128)

/* These require multiplication, so are good candidates for table lookup */
#define USE_TABLES
#ifndef USE_TABLES
	#define	Y255toY219(y, tmp)	((UInt8)(tmp = 219 * (UInt32)(y), tmp += tmp >> 8, tmp += (1 << (8-1)), tmp >>= 8, tmp += 16))
	#define C128toC112(c)		((UInt8)(((112 * (UInt32)(c) + (1 <<  (7-1))) >>  7) + 16))		/* round((c - 128) * 112./128. + 128) */
	#define C256toC112(c)		((UInt8)(((112 * (UInt32)(c) + (1 <<  (8-1))) >>  8) + 16))
	#define CS254toC112(c)		((UInt8)(((c) * 0x70E2 + (1 << (15-1))) >> 15))
	#define C1024toC112(c)		((UInt8)(((112 * (UInt32)(c) + (1 << (10-1))) >> 10) + 16))
#else /* USE_TABLES */
	#define	Y255toY219(y, tmp)	TabFullToVideoLuma[y]
	#define C128toC112(c)		TabFullToVideoChroma[c]
	#define C256toC112(c)		TabFullToVideoChroma[(c) >> 1]
	#define CS254toC112(c)		TabWideToVideoChroma[((c) + 256) >> 1]
	#define C1024toC112(c)		TabFullToVideoChroma[((c) + (1 << (3 - 1))) >> 3]


	/********************************************************************************
	 * Y and C tables, mapping from full range to standard range
	 ********************************************************************************/

static const UInt8 TabFullToVideoLuma[256] = {
	16,		/* 0 */
	17,		/* 1 */
	18,		/* 2 */
	19,		/* 3 */
	19,		/* 4 */
	20,		/* 5 */
	21,		/* 6 */
	22,		/* 7 */
	23,		/* 8 */
	24,		/* 9 */
	25,		/* 10 */
	25,		/* 11 */
	26,		/* 12 */
	27,		/* 13 */
	28,		/* 14 */
	29,		/* 15 */
	30,		/* 16 */
	31,		/* 17 */
	31,		/* 18 */
	32,		/* 19 */
	33,		/* 20 */
	34,		/* 21 */
	35,		/* 22 */
	36,		/* 23 */
	37,		/* 24 */
	37,		/* 25 */
	38,		/* 26 */
	39,		/* 27 */
	40,		/* 28 */
	41,		/* 29 */
	42,		/* 30 */
	43,		/* 31 */
	43,		/* 32 */
	44,		/* 33 */
	45,		/* 34 */
	46,		/* 35 */
	47,		/* 36 */
	48,		/* 37 */
	49,		/* 38 */
	49,		/* 39 */
	50,		/* 40 */
	51,		/* 41 */
	52,		/* 42 */
	53,		/* 43 */
	54,		/* 44 */
	55,		/* 45 */
	56,		/* 46 */
	56,		/* 47 */
	57,		/* 48 */
	58,		/* 49 */
	59,		/* 50 */
	60,		/* 51 */
	61,		/* 52 */
	62,		/* 53 */
	62,		/* 54 */
	63,		/* 55 */
	64,		/* 56 */
	65,		/* 57 */
	66,		/* 58 */
	67,		/* 59 */
	68,		/* 60 */
	68,		/* 61 */
	69,		/* 62 */
	70,		/* 63 */
	71,		/* 64 */
	72,		/* 65 */
	73,		/* 66 */
	74,		/* 67 */
	74,		/* 68 */
	75,		/* 69 */
	76,		/* 70 */
	77,		/* 71 */
	78,		/* 72 */
	79,		/* 73 */
	80,		/* 74 */
	80,		/* 75 */
	81,		/* 76 */
	82,		/* 77 */
	83,		/* 78 */
	84,		/* 79 */
	85,		/* 80 */
	86,		/* 81 */
	86,		/* 82 */
	87,		/* 83 */
	88,		/* 84 */
	89,		/* 85 */
	90,		/* 86 */
	91,		/* 87 */
	92,		/* 88 */
	92,		/* 89 */
	93,		/* 90 */
	94,		/* 91 */
	95,		/* 92 */
	96,		/* 93 */
	97,		/* 94 */
	98,		/* 95 */
	98,		/* 96 */
	99,		/* 97 */
	100,	/* 98 */
	101,	/* 99 */
	102,	/* 100 */
	103,	/* 101 */
	104,	/* 102 */
	104,	/* 103 */
	105,	/* 104 */
	106,	/* 105 */
	107,	/* 106 */
	108,	/* 107 */
	109,	/* 108 */
	110,	/* 109 */
	110,	/* 110 */
	111,	/* 111 */
	112,	/* 112 */
	113,	/* 113 */
	114,	/* 114 */
	115,	/* 115 */
	116,	/* 116 */
	116,	/* 117 */
	117,	/* 118 */
	118,	/* 119 */
	119,	/* 120 */
	120,	/* 121 */
	121,	/* 122 */
	122,	/* 123 */
	122,	/* 124 */
	123,	/* 125 */
	124,	/* 126 */
	125,	/* 127 */
	126,	/* 128 */
	127,	/* 129 */
	128,	/* 130 */
	129,	/* 131 */
	129,	/* 132 */
	130,	/* 133 */
	131,	/* 134 */
	132,	/* 135 */
	133,	/* 136 */
	134,	/* 137 */
	135,	/* 138 */
	135,	/* 139 */
	136,	/* 140 */
	137,	/* 141 */
	138,	/* 142 */
	139,	/* 143 */
	140,	/* 144 */
	141,	/* 145 */
	141,	/* 146 */
	142,	/* 147 */
	143,	/* 148 */
	144,	/* 149 */
	145,	/* 150 */
	146,	/* 151 */
	147,	/* 152 */
	147,	/* 153 */
	148,	/* 154 */
	149,	/* 155 */
	150,	/* 156 */
	151,	/* 157 */
	152,	/* 158 */
	153,	/* 159 */
	153,	/* 160 */
	154,	/* 161 */
	155,	/* 162 */
	156,	/* 163 */
	157,	/* 164 */
	158,	/* 165 */
	159,	/* 166 */
	159,	/* 167 */
	160,	/* 168 */
	161,	/* 169 */
	162,	/* 170 */
	163,	/* 171 */
	164,	/* 172 */
	165,	/* 173 */
	165,	/* 174 */
	166,	/* 175 */
	167,	/* 176 */
	168,	/* 177 */
	169,	/* 178 */
	170,	/* 179 */
	171,	/* 180 */
	171,	/* 181 */
	172,	/* 182 */
	173,	/* 183 */
	174,	/* 184 */
	175,	/* 185 */
	176,	/* 186 */
	177,	/* 187 */
	177,	/* 188 */
	178,	/* 189 */
	179,	/* 190 */
	180,	/* 191 */
	181,	/* 192 */
	182,	/* 193 */
	183,	/* 194 */
	183,	/* 195 */
	184,	/* 196 */
	185,	/* 197 */
	186,	/* 198 */
	187,	/* 199 */
	188,	/* 200 */
	189,	/* 201 */
	189,	/* 202 */
	190,	/* 203 */
	191,	/* 204 */
	192,	/* 205 */
	193,	/* 206 */
	194,	/* 207 */
	195,	/* 208 */
	195,	/* 209 */
	196,	/* 210 */
	197,	/* 211 */
	198,	/* 212 */
	199,	/* 213 */
	200,	/* 214 */
	201,	/* 215 */
	202,	/* 216 */
	202,	/* 217 */
	203,	/* 218 */
	204,	/* 219 */
	205,	/* 220 */
	206,	/* 221 */
	207,	/* 222 */
	208,	/* 223 */
	208,	/* 224 */
	209,	/* 225 */
	210,	/* 226 */
	211,	/* 227 */
	212,	/* 228 */
	213,	/* 229 */
	214,	/* 230 */
	214,	/* 231 */
	215,	/* 232 */
	216,	/* 233 */
	217,	/* 234 */
	218,	/* 235 */
	219,	/* 236 */
	220,	/* 237 */
	220,	/* 238 */
	221,	/* 239 */
	222,	/* 240 */
	223,	/* 241 */
	224,	/* 242 */
	225,	/* 243 */
	226,	/* 244 */
	226,	/* 245 */
	227,	/* 246 */
	228,	/* 247 */
	229,	/* 248 */
	230,	/* 249 */
	231,	/* 250 */
	232,	/* 251 */
	232,	/* 252 */
	233,	/* 253 */
	234,	/* 254 */
	235,	/* 255 */
};
static const UInt8 TabFullToVideoChroma[256] = {
	16,		/*   0 or -128 */
	17,		/*   1 or -127 */
	18,		/*   2 or -126 */
	19,		/*   3 or -125 */
	20,		/*   4 or -124 */
	20,		/*   5 or -123 */
	21,		/*   6 or -122 */
	22,		/*   7 or -121 */
	23,		/*   8 or -120 */
	24,		/*   9 or -119 */
	25,		/*  10 or -118 */
	26,		/*  11 or -117 */
	27,		/*  12 or -116 */
	27,		/*  13 or -115 */
	28,		/*  14 or -114 */
	29,		/*  15 or -113 */
	30,		/*  16 or -112 */
	31,		/*  17 or -111 */
	32,		/*  18 or -110 */
	33,		/*  19 or -109 */
	34,		/*  20 or -108 */
	34,		/*  21 or -107 */
	35,		/*  22 or -106 */
	36,		/*  23 or -105 */
	37,		/*  24 or -104 */
	38,		/*  25 or -103 */
	39,		/*  26 or -102 */
	40,		/*  27 or -101 */
	41,		/*  28 or -100 */
	41,		/*  29 or  -99 */
	42,		/*  30 or  -98 */
	43,		/*  31 or  -97 */
	44,		/*  32 or  -96 */
	45,		/*  33 or  -95 */
	46,		/*  34 or  -94 */
	47,		/*  35 or  -93 */
	48,		/*  36 or  -92 */
	48,		/*  37 or  -91 */
	49,		/*  38 or  -90 */
	50,		/*  39 or  -89 */
	51,		/*  40 or  -88 */
	52,		/*  41 or  -87 */
	53,		/*  42 or  -86 */
	54,		/*  43 or  -85 */
	55,		/*  44 or  -84 */
	55,		/*  45 or  -83 */
	56,		/*  46 or  -82 */
	57,		/*  47 or  -81 */
	58,		/*  48 or  -80 */
	59,		/*  49 or  -79 */
	60,		/*  50 or  -78 */
	61,		/*  51 or  -77 */
	62,		/*  52 or  -76 */
	62,		/*  53 or  -75 */
	63,		/*  54 or  -74 */
	64,		/*  55 or  -73 */
	65,		/*  56 or  -72 */
	66,		/*  57 or  -71 */
	67,		/*  58 or  -70 */
	68,		/*  59 or  -69 */
	69,		/*  60 or  -68 */
	69,		/*  61 or  -67 */
	70,		/*  62 or  -66 */
	71,		/*  63 or  -65 */
	72,		/*  64 or  -64 */
	73,		/*  65 or  -63 */
	74,		/*  66 or  -62 */
	75,		/*  67 or  -61 */
	76,		/*  68 or  -60 */
	76,		/*  69 or  -59 */
	77,		/*  70 or  -58 */
	78,		/*  71 or  -57 */
	79,		/*  72 or  -56 */
	80,		/*  73 or  -55 */
	81,		/*  74 or  -54 */
	82,		/*  75 or  -53 */
	83,		/*  76 or  -52 */
	83,		/*  77 or  -51 */
	84,		/*  78 or  -50 */
	85,		/*  79 or  -49 */
	86,		/*  80 or  -48 */
	87,		/*  81 or  -47 */
	88,		/*  82 or  -46 */
	89,		/*  83 or  -45 */
	90,		/*  84 or  -44 */
	90,		/*  85 or  -43 */
	91,		/*  86 or  -42 */
	92,		/*  87 or  -41 */
	93,		/*  88 or  -40 */
	94,		/*  89 or  -39 */
	95,		/*  90 or  -38 */
	96,		/*  91 or  -37 */
	97,		/*  92 or  -36 */
	97,		/*  93 or  -35 */
	98,		/*  94 or  -34 */
	99,		/*  95 or  -33 */
	100,	/*  96 or  -32 */
	101,	/*  97 or  -31 */
	102,	/*  98 or  -30 */
	103,	/*  99 or  -29 */
	104,	/* 100 or  -28 */
	104,	/* 101 or  -27 */
	105,	/* 102 or  -26 */
	106,	/* 103 or  -25 */
	107,	/* 104 or  -24 */
	108,	/* 105 or  -23 */
	109,	/* 106 or  -22 */
	110,	/* 107 or  -21 */
	111,	/* 108 or  -20 */
	111,	/* 109 or  -19 */
	112,	/* 110 or  -18 */
	113,	/* 111 or  -17 */
	114,	/* 112 or  -16 */
	115,	/* 113 or  -15 */
	116,	/* 114 or  -14 */
	117,	/* 115 or  -13 */
	118,	/* 116 or  -12 */
	118,	/* 117 or  -11 */
	119,	/* 118 or  -10 */
	120,	/* 119 or   -9 */
	121,	/* 120 or   -8 */
	122,	/* 121 or   -7 */
	123,	/* 122 or   -6 */
	124,	/* 123 or   -5 */
	125,	/* 124 or   -4 */
	125,	/* 125 or   -3 */
	126,	/* 126 or   -2 */
	127,	/* 127 or   -1 */
	128,	/* 128 or    0 */
	129,	/* 129 or    1 */
	130,	/* 130 or    2 */
	131,	/* 131 or    3 */
	132,	/* 132 or    4 */
	132,	/* 133 or    5 */
	133,	/* 134 or    6 */
	134,	/* 135 or    7 */
	135,	/* 136 or    8 */
	136,	/* 137 or    9 */
	137,	/* 138 or   10 */
	138,	/* 139 or   11 */
	139,	/* 140 or   12 */
	139,	/* 141 or   13 */
	140,	/* 142 or   14 */
	141,	/* 143 or   15 */
	142,	/* 144 or   16 */
	143,	/* 145 or   17 */
	144,	/* 146 or   18 */
	145,	/* 147 or   19 */
	146,	/* 148 or   20 */
	146,	/* 149 or   21 */
	147,	/* 150 or   22 */
	148,	/* 151 or   23 */
	149,	/* 152 or   24 */
	150,	/* 153 or   25 */
	151,	/* 154 or   26 */
	152,	/* 155 or   27 */
	153,	/* 156 or   28 */
	153,	/* 157 or   29 */
	154,	/* 158 or   30 */
	155,	/* 159 or   31 */
	156,	/* 160 or   32 */
	157,	/* 161 or   33 */
	158,	/* 162 or   34 */
	159,	/* 163 or   35 */
	160,	/* 164 or   36 */
	160,	/* 165 or   37 */
	161,	/* 166 or   38 */
	162,	/* 167 or   39 */
	163,	/* 168 or   40 */
	164,	/* 169 or   41 */
	165,	/* 170 or   42 */
	166,	/* 171 or   43 */
	167,	/* 172 or   44 */
	167,	/* 173 or   45 */
	168,	/* 174 or   46 */
	169,	/* 175 or   47 */
	170,	/* 176 or   48 */
	171,	/* 177 or   49 */
	172,	/* 178 or   50 */
	173,	/* 179 or   51 */
	174,	/* 180 or   52 */
	174,	/* 181 or   53 */
	175,	/* 182 or   54 */
	176,	/* 183 or   55 */
	177,	/* 184 or   56 */
	178,	/* 185 or   57 */
	179,	/* 186 or   58 */
	180,	/* 187 or   59 */
	181,	/* 188 or   60 */
	181,	/* 189 or   61 */
	182,	/* 190 or   62 */
	183,	/* 191 or   63 */
	184,	/* 192 or   64 */
	185,	/* 193 or   65 */
	186,	/* 194 or   66 */
	187,	/* 195 or   67 */
	188,	/* 196 or   68 */
	188,	/* 197 or   69 */
	189,	/* 198 or   70 */
	190,	/* 199 or   71 */
	191,	/* 200 or   72 */
	192,	/* 201 or   73 */
	193,	/* 202 or   74 */
	194,	/* 203 or   75 */
	195,	/* 204 or   76 */
	195,	/* 205 or   77 */
	196,	/* 206 or   78 */
	197,	/* 207 or   79 */
	198,	/* 208 or   80 */
	199,	/* 209 or   81 */
	200,	/* 210 or   82 */
	201,	/* 211 or   83 */
	202,	/* 212 or   84 */
	202,	/* 213 or   85 */
	203,	/* 214 or   86 */
	204,	/* 215 or   87 */
	205,	/* 216 or   88 */
	206,	/* 217 or   89 */
	207,	/* 218 or   90 */
	208,	/* 219 or   91 */
	209,	/* 220 or   92 */
	209,	/* 221 or   93 */
	210,	/* 222 or   94 */
	211,	/* 223 or   95 */
	212,	/* 224 or   96 */
	213,	/* 225 or   97 */
	214,	/* 226 or   98 */
	215,	/* 227 or   99 */
	216,	/* 228 or  100 */
	216,	/* 229 or  101 */
	217,	/* 230 or  102 */
	218,	/* 231 or  103 */
	219,	/* 232 or  104 */
	220,	/* 233 or  105 */
	221,	/* 234 or  106 */
	222,	/* 235 or  107 */
	223,	/* 236 or  108 */
	223,	/* 237 or  109 */
	224,	/* 238 or  110 */
	225,	/* 239 or  111 */
	226,	/* 240 or  112 */
	227,	/* 241 or  113 */
	228,	/* 242 or  114 */
	229,	/* 243 or  115 */
	230,	/* 244 or  116 */
	230,	/* 245 or  117 */
	231,	/* 246 or  118 */
	232,	/* 247 or  119 */
	233,	/* 248 or  120 */
	234,	/* 249 or  121 */
	235,	/* 250 or  122 */
	236,	/* 251 or  123 */
	237,	/* 252 or  124 */
	237,	/* 253 or  125 */
	238,	/* 254 or  126 */
	239,	/* 255 or  127 */
};
static const UInt8 TabWideToVideoChroma[256] = {
	15,		/*   0 or -128 */
	16,		/*   1 or -127 */
	17,		/*   2 or -126 */
	18,		/*   3 or -125 */
	19,		/*   4 or -124 */
	19,		/*   5 or -123 */
	20,		/*   6 or -122 */
	21,		/*   7 or -121 */
	22,		/*   8 or -120 */
	23,		/*   9 or -119 */
	24,		/*  10 or -118 */
	25,		/*  11 or -117 */
	26,		/*  12 or -116 */
	26,		/*  13 or -115 */
	27,		/*  14 or -114 */
	28,		/*  15 or -113 */
	29,		/*  16 or -112 */
	30,		/*  17 or -111 */
	31,		/*  18 or -110 */
	32,		/*  19 or -109 */
	33,		/*  20 or -108 */
	34,		/*  21 or -107 */
	34,		/*  22 or -106 */
	35,		/*  23 or -105 */
	36,		/*  24 or -104 */
	37,		/*  25 or -103 */
	38,		/*  26 or -102 */
	39,		/*  27 or -101 */
	40,		/*  28 or -100 */
	41,		/*  29 or  -99 */
	41,		/*  30 or  -98 */
	42,		/*  31 or  -97 */
	43,		/*  32 or  -96 */
	44,		/*  33 or  -95 */
	45,		/*  34 or  -94 */
	46,		/*  35 or  -93 */
	47,		/*  36 or  -92 */
	48,		/*  37 or  -91 */
	49,		/*  38 or  -90 */
	49,		/*  39 or  -89 */
	50,		/*  40 or  -88 */
	51,		/*  41 or  -87 */
	52,		/*  42 or  -86 */
	53,		/*  43 or  -85 */
	54,		/*  44 or  -84 */
	55,		/*  45 or  -83 */
	56,		/*  46 or  -82 */
	56,		/*  47 or  -81 */
	57,		/*  48 or  -80 */
	58,		/*  49 or  -79 */
	59,		/*  50 or  -78 */
	60,		/*  51 or  -77 */
	61,		/*  52 or  -76 */
	62,		/*  53 or  -75 */
	63,		/*  54 or  -74 */
	64,		/*  55 or  -73 */
	64,		/*  56 or  -72 */
	65,		/*  57 or  -71 */
	66,		/*  58 or  -70 */
	67,		/*  59 or  -69 */
	68,		/*  60 or  -68 */
	69,		/*  61 or  -67 */
	70,		/*  62 or  -66 */
	71,		/*  63 or  -65 */
	72,		/*  64 or  -64 */
	72,		/*  65 or  -63 */
	73,		/*  66 or  -62 */
	74,		/*  67 or  -61 */
	75,		/*  68 or  -60 */
	76,		/*  69 or  -59 */
	77,		/*  70 or  -58 */
	78,		/*  71 or  -57 */
	79,		/*  72 or  -56 */
	79,		/*  73 or  -55 */
	80,		/*  74 or  -54 */
	81,		/*  75 or  -53 */
	82,		/*  76 or  -52 */
	83,		/*  77 or  -51 */
	84,		/*  78 or  -50 */
	85,		/*  79 or  -49 */
	86,		/*  80 or  -48 */
	87,		/*  81 or  -47 */
	87,		/*  82 or  -46 */
	88,		/*  83 or  -45 */
	89,		/*  84 or  -44 */
	90,		/*  85 or  -43 */
	91,		/*  86 or  -42 */
	92,		/*  87 or  -41 */
	93,		/*  88 or  -40 */
	94,		/*  89 or  -39 */
	94,		/*  90 or  -38 */
	95,		/*  91 or  -37 */
	96,		/*  92 or  -36 */
	97,		/*  93 or  -35 */
	98,		/*  94 or  -34 */
	99,		/*  95 or  -33 */
	100,	/*  96 or  -32 */
	101,	/*  97 or  -31 */
	102,	/*  98 or  -30 */
	102,	/*  99 or  -29 */
	103,	/* 100 or  -28 */
	104,	/* 101 or  -27 */
	105,	/* 102 or  -26 */
	106,	/* 103 or  -25 */
	107,	/* 104 or  -24 */
	108,	/* 105 or  -23 */
	109,	/* 106 or  -22 */
	109,	/* 107 or  -21 */
	110,	/* 108 or  -20 */
	111,	/* 109 or  -19 */
	112,	/* 110 or  -18 */
	113,	/* 111 or  -17 */
	114,	/* 112 or  -16 */
	115,	/* 113 or  -15 */
	116,	/* 114 or  -14 */
	117,	/* 115 or  -13 */
	117,	/* 116 or  -12 */
	118,	/* 117 or  -11 */
	119,	/* 118 or  -10 */
	120,	/* 119 or   -9 */
	121,	/* 120 or   -8 */
	122,	/* 121 or   -7 */
	123,	/* 122 or   -6 */
	124,	/* 123 or   -5 */
	124,	/* 124 or   -4 */
	125,	/* 125 or   -3 */
	126,	/* 126 or   -2 */
	127,	/* 127 or   -1 */
	128,	/* 128 or    0 */
	129,	/* 129 or    1 */
	130,	/* 130 or    2 */
	131,	/* 131 or    3 */
	132,	/* 132 or    4 */
	132,	/* 133 or    5 */
	133,	/* 134 or    6 */
	134,	/* 135 or    7 */
	135,	/* 136 or    8 */
	136,	/* 137 or    9 */
	137,	/* 138 or   10 */
	138,	/* 139 or   11 */
	139,	/* 140 or   12 */
	139,	/* 141 or   13 */
	140,	/* 142 or   14 */
	141,	/* 143 or   15 */
	142,	/* 144 or   16 */
	143,	/* 145 or   17 */
	144,	/* 146 or   18 */
	145,	/* 147 or   19 */
	146,	/* 148 or   20 */
	147,	/* 149 or   21 */
	147,	/* 150 or   22 */
	148,	/* 151 or   23 */
	149,	/* 152 or   24 */
	150,	/* 153 or   25 */
	151,	/* 154 or   26 */
	152,	/* 155 or   27 */
	153,	/* 156 or   28 */
	154,	/* 157 or   29 */
	154,	/* 158 or   30 */
	155,	/* 159 or   31 */
	156,	/* 160 or   32 */
	157,	/* 161 or   33 */
	158,	/* 162 or   34 */
	159,	/* 163 or   35 */
	160,	/* 164 or   36 */
	161,	/* 165 or   37 */
	162,	/* 166 or   38 */
	162,	/* 167 or   39 */
	163,	/* 168 or   40 */
	164,	/* 169 or   41 */
	165,	/* 170 or   42 */
	166,	/* 171 or   43 */
	167,	/* 172 or   44 */
	168,	/* 173 or   45 */
	169,	/* 174 or   46 */
	169,	/* 175 or   47 */
	170,	/* 176 or   48 */
	171,	/* 177 or   49 */
	172,	/* 178 or   50 */
	173,	/* 179 or   51 */
	174,	/* 180 or   52 */
	175,	/* 181 or   53 */
	176,	/* 182 or   54 */
	177,	/* 183 or   55 */
	177,	/* 184 or   56 */
	178,	/* 185 or   57 */
	179,	/* 186 or   58 */
	180,	/* 187 or   59 */
	181,	/* 188 or   60 */
	182,	/* 189 or   61 */
	183,	/* 190 or   62 */
	184,	/* 191 or   63 */
	185,	/* 192 or   64 */
	185,	/* 193 or   65 */
	186,	/* 194 or   66 */
	187,	/* 195 or   67 */
	188,	/* 196 or   68 */
	189,	/* 197 or   69 */
	190,	/* 198 or   70 */
	191,	/* 199 or   71 */
	192,	/* 200 or   72 */
	192,	/* 201 or   73 */
	193,	/* 202 or   74 */
	194,	/* 203 or   75 */
	195,	/* 204 or   76 */
	196,	/* 205 or   77 */
	197,	/* 206 or   78 */
	198,	/* 207 or   79 */
	199,	/* 208 or   80 */
	200,	/* 209 or   81 */
	200,	/* 210 or   82 */
	201,	/* 211 or   83 */
	202,	/* 212 or   84 */
	203,	/* 213 or   85 */
	204,	/* 214 or   86 */
	205,	/* 215 or   87 */
	206,	/* 216 or   88 */
	207,	/* 217 or   89 */
	207,	/* 218 or   90 */
	208,	/* 219 or   91 */
	209,	/* 220 or   92 */
	210,	/* 221 or   93 */
	211,	/* 222 or   94 */
	212,	/* 223 or   95 */
	213,	/* 224 or   96 */
	214,	/* 225 or   97 */
	215,	/* 226 or   98 */
	215,	/* 227 or   99 */
	216,	/* 228 or  100 */
	217,	/* 229 or  101 */
	218,	/* 230 or  102 */
	219,	/* 231 or  103 */
	220,	/* 232 or  104 */
	221,	/* 233 or  105 */
	222,	/* 234 or  106 */
	222,	/* 235 or  107 */
	223,	/* 236 or  108 */
	224,	/* 237 or  109 */
	225,	/* 238 or  110 */
	226,	/* 239 or  111 */
	227,	/* 240 or  112 */
	228,	/* 241 or  113 */
	229,	/* 242 or  114 */
	230,	/* 243 or  115 */
	230,	/* 244 or  116 */
	231,	/* 245 or  117 */
	232,	/* 246 or  118 */
	233,	/* 247 or  119 */
	234,	/* 248 or  120 */
	235,	/* 249 or  121 */
	236,	/* 250 or  122 */
	237,	/* 251 or  123 */
	237,	/* 252 or  124 */
	238,	/* 253 or  125 */
	239,	/* 254 or  126 */
	240,	/* 255 or  127 */
};


#endif /* USE_TABLES */



/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****						4:2:0 for MPEG-1, H.261, JFIF					*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


/********************************************************************************
 * FskUYVY422toYUV420								u y v y
 ********************************************************************************/

void
FskUYVY422toYUV420(
	UInt32			width,
	UInt32			height,
	
	const UInt32	*src,				/* Cb' Y' Cr' Y' src */
	SInt32			srcRB,
	
	UInt8			*dstY0,				/* Y'  dst */
	SInt32			dstYRB,
	UInt8			*dstU,				/* Cb' dst */
	UInt8			*dstV,				/* Cr' dst */
	SInt32			dstUVRB
)
{
	const UInt8		*src0	= (const UInt8*)src;
	const UInt8		*src1	= src0  + srcRB;
	UInt8			*dstY1	= dstY0 + dstYRB;
	UInt32			i, u0, u1, v0, v1;
	
	width  >>= 1;															/* Chroma blocks */
	height >>= 1;															/* Chroma blocks */
	if ((width == 0) ||(height == 0))
		return;
	
	srcRB   = (srcRB   << 1) - (SInt32)(width << 2);						/* srcBump   - 2 lines at a time minus a line of VYUY */
	dstYRB  = (dstYRB  << 1) - (SInt32)(width << 1);						/* dstYBump  - 2 lines at a time minus a line of YY */
	dstUVRB = (dstUVRB << 0) - (SInt32)(width << 0);						/* dstUVBump - 1 line  at a time minue a line of U (or V) */
	width--;																/* Anticipate a prime of the X pipeline */

	for ( ; height--; src0 += srcRB, src1 += srcRB, dstY0 += dstYRB, dstY1 += dstYRB, dstU += dstUVRB, dstV += dstUVRB) {

		/* Prime the pipeline */
		u0 = (UInt32)(*src0++) + (UInt32)(*src1++);							/* Add U(0,0) and U(0,1) */
		*dstY0++ = *src0++;													/* Y(0,0) */
		*dstY1++ = *src1++;													/* Y(0,1) */
		v0 = (UInt32)(*src0++) + (UInt32)(*src1++);							/* Add V(0,0) and V(0,1) */
		*dstY0++ = *src0++;													/* Y(1,0) */
		*dstY1++ = *src1++;													/* Y(1,1) */

		/* Core of the pipeline */
		for (i = width; i--; u0 = u1, v0 = v1) {							/* Actually width-1 */
			u1 = (UInt32)(*src0++) + (UInt32)(*src1++);						/* Add U(n,0) and U(n,1) */
			*dstY0++ = *src0++;												/* Y(2*n+2,0) */
			*dstY1++ = *src1++;												/* Y(2*n+2,1) */
			v1 = (UInt32)(*src0++) + (UInt32)(*src1++);						/* Add V(n,0) and V(n,1) */
			*dstY0++ = *src0++;												/* Y(2*n+3,0) */
			*dstY1++ = *src1++;												/* Y(2*n+3,1) */
			*dstU++ = C1024toC128((u0 << 2) - u0 + u1);						/* U(n+0,0), interpolated 1/4 of the way over */
			*dstV++ = C1024toC128((v0 << 2) - v0 + v1);						/* V(n+0,0), interpolated 1/4 of the way over */
		}
		
		/* Flush the pipeline */
		*dstU++ = C256toC128(u0);											/* Smear last chroma 1/4 pixel to the right */
		*dstV++ = C256toC128(v0);											/* Smear last chroma 1/4 pixel to the right */
	}
}


/********************************************************************************
 * FskYUYV422toYUV420								y u y v
 ********************************************************************************/

void
FskYUYV422toYUV420(
	UInt32			width,
	UInt32			height,
	
	const UInt32	*src,				/* Y' Cb' Y' Cr' src */
	SInt32			srcRB,
	
	UInt8			*dstY0,				/* Y'  dst */
	SInt32			dstYRB,
	UInt8			*dstU,				/* Cb' dst */
	UInt8			*dstV,				/* Cr' dst */
	SInt32			dstUVRB
)
{
	const UInt8		*src0	= (const UInt8*)src;
	const UInt8		*src1	= src0  + srcRB;
	UInt8			*dstY1	= dstY0 + dstYRB;
	UInt32			i, u0, u1, v0, v1;
	
	width  >>= 1;															/* Chroma blocks */
	height >>= 1;															/* Chroma blocks */
	if ((width == 0) ||(height == 0))
		return;
	
	srcRB   = (srcRB   << 1) - (SInt32)(width << 2);						/* srcBump   - 2 lines at a time minus a line of VYUY */
	dstYRB  = (dstYRB  << 1) - (SInt32)(width << 1);						/* dstYBump  - 2 lines at a time minus a line of YY */
	dstUVRB = (dstUVRB << 0) - (SInt32)(width << 0);						/* dstUVBump - 1 line  at a time minue a line of U (or V) */
	width--;																/* Anticipate a prime of the X pipeline */

	for ( ; height--; src0 += srcRB, src1 += srcRB, dstY0 += dstYRB, dstY1 += dstYRB, dstU += dstUVRB, dstV += dstUVRB) {

		/* Prime the pipeline */
		*dstY0++ = *src0++;													/* Y(0,0) */
		*dstY1++ = *src1++;													/* Y(0,1) */
		u0 = (UInt32)(*src0++) + (UInt32)(*src1++);							/* Add U(0,0) and U(0,1) */
		*dstY0++ = *src0++;													/* Y(1,0) */
		*dstY1++ = *src1++;													/* Y(1,1) */
		v0 = (UInt32)(*src0++) + (UInt32)(*src1++);							/* Add V(0,0) and V(0,1) */

		/* Core of the pipeline */
		for (i = width; i--; u0 = u1, v0 = v1) {							/* Actually width-1 */
			*dstY0++ = *src0++;												/* Y(2*n+2,0) */
			*dstY1++ = *src1++;												/* Y(2*n+2,1) */
			u1 = (UInt32)(*src0++) + (UInt32)(*src1++);						/* Add U(n,0) and U(n,1) */
			*dstY0++ = *src0++;												/* Y(2*n+3,0) */
			*dstY1++ = *src1++;												/* Y(2*n+3,1) */
			v1 = (UInt32)(*src0++) + (UInt32)(*src1++);						/* Add V(n,0) and V(n,1) */
			*dstU++ = C1024toC128((u0 << 2) - u0 + u1);						/* U(n+0,0), interpolated 1/4 of the way over */
			*dstV++ = C1024toC128((v0 << 2) - v0 + v1);						/* V(n+0,0), interpolated 1/4 of the way over */
		}
		
		/* Flush the pipeline */
		*dstU++ = C256toC128(u0);											/* Smear last chroma 1/4 pixel to the right */
		*dstV++ = C256toC128(v0);											/* Smear last chroma 1/4 pixel to the right */
	}
}


/********************************************************************************
 * FskFullUYVY422toYUV420								u y v y
 *	srcY in [0,  255],	srcC in [0,  255]
 *	dstY in [16, 235],	dstC in [16, 240]
 ********************************************************************************/

void
FskFullUYVY422toYUV420(
	UInt32			width,
	UInt32			height,
	
	const UInt32	*src,				/* Cb' Y' Cr' Y' src */
	SInt32			srcRB,
	
	UInt8			*dstY0,				/* Y'  dst */
	SInt32			dstYRB,
	UInt8			*dstU,				/* Cb' dst */
	UInt8			*dstV,				/* Cr' dst */
	SInt32			dstUVRB
)
{
	const UInt8		*src0	= (const UInt8*)src;
	const UInt8		*src1	= src0  + srcRB;
	UInt8			*dstY1	= dstY0 + dstYRB;
	UInt32			i, u0, u1, v0, v1;
	#ifndef USE_TABLES
		UInt32		tmp;
	#endif /* USE_TABLES */
	
	width  >>= 1;															/* Chroma blocks */
	height >>= 1;															/* Chroma blocks */
	if ((width == 0) ||(height == 0))
		return;
	
	srcRB   = (srcRB   << 1) - (SInt32)(width << 2);						/* srcBump   - 2 lines at a time minus a line of VYUY */
	dstYRB  = (dstYRB  << 1) - (SInt32)(width << 1);						/* dstYBump  - 2 lines at a time minus a line of YY */
	dstUVRB = (dstUVRB << 0) - (SInt32)(width << 0);						/* dstUVBump - 1 line  at a time minue a line of U (or V) */
	width--;																/* Anticipate a prime of the X pipeline */

	for ( ; height--; src0 += srcRB, src1 += srcRB, dstY0 += dstYRB, dstY1 += dstYRB, dstU += dstUVRB, dstV += dstUVRB) {

		/* Prime the pipeline */
		u0 = (UInt32)(*src0++) + (UInt32)(*src1++);							/* Add U(0,0) and U(0,1) */
		*dstY0++ = Y255toY219(*src0++, tmp);								/* Y(0,0) */
		*dstY1++ = Y255toY219(*src1++, tmp);								/* Y(0,1) */
		v0 = (UInt32)(*src0++) + (UInt32)(*src1++);							/* Add V(0,0) and V(0,1) */
		*dstY0++ = Y255toY219(*src0++, tmp);								/* Y(1,0) */
		*dstY1++ = Y255toY219(*src1++, tmp);								/* Y(1,1) */

		/* Core of the pipeline */
		for (i = width; i--; u0 = u1, v0 = v1) {							/* Actually width-1 */
			u1 = (UInt32)(*src0++) + (UInt32)(*src1++);						/* Add U(n,0) and U(n,1) */
			*dstY0++ = Y255toY219(*src0++, tmp);							/* Y(2*n+2,0) */
			*dstY1++ = Y255toY219(*src1++, tmp);							/* Y(2*n+2,1) */
			v1 = (UInt32)(*src0++) + (UInt32)(*src1++);						/* Add V(n,0) and V(n,1) */
			*dstY0++ = Y255toY219(*src0++, tmp);							/* Y(2*n+3,0) */
			*dstY1++ = Y255toY219(*src1++, tmp);							/* Y(2*n+3,1) */
			*dstU++ = C1024toC112((u0 << 2) - u0 + u1);						/* U(n+0,0), interpolated 1/4 of the way over */
			*dstV++ = C1024toC112((v0 << 2) - v0 + v1);						/* V(n+0,0), interpolated 1/4 of the way over */
		}
		
		/* Flush the pipeline */
		*dstU++ = C256toC112(u0);											/* Smear last chroma 1/4 pixel to the right */
		*dstV++ = C256toC112(v0);											/* Smear last chroma 1/4 pixel to the right */
	}
}


/********************************************************************************
 * FskFullYUYV422toYUV420								y u y v
 *	srcY in [0,  255],	srcC in [0,  255]
 *	dstY in [16, 235],	dstC in [16, 240]
 ********************************************************************************/

void
FskFullYUYV422toYUV420(
	UInt32			width,
	UInt32			height,
	
	const UInt32	*src,				/* Y' Cb' Y' Cr' src */
	SInt32			srcRB,
	
	UInt8			*dstY0,				/* Y'  dst */
	SInt32			dstYRB,
	UInt8			*dstU,				/* Cb' dst */
	UInt8			*dstV,				/* Cr' dst */
	SInt32			dstUVRB
)
{
	const UInt8		*src0	= (const UInt8*)src;
	const UInt8		*src1	= src0  + srcRB;
	UInt8			*dstY1	= dstY0 + dstYRB;
	UInt32			i, u0, u1, v0, v1;
	#ifndef USE_TABLES
		UInt32		tmp;
	#endif /* USE_TABLES */
	
	width  >>= 1;															/* Chroma blocks */
	height >>= 1;															/* Chroma blocks */
	if ((width == 0) ||(height == 0))
		return;
	
	srcRB   = (srcRB   << 1) - (SInt32)(width << 2);						/* srcBump   - 2 lines at a time minus a line of VYUY */
	dstYRB  = (dstYRB  << 1) - (SInt32)(width << 1);						/* dstYBump  - 2 lines at a time minus a line of YY */
	dstUVRB = (dstUVRB << 0) - (SInt32)(width << 0);						/* dstUVBump - 1 line  at a time minue a line of U (or V) */
	width--;																/* Anticipate a prime of the X pipeline */

	for ( ; height--; src0 += srcRB, src1 += srcRB, dstY0 += dstYRB, dstY1 += dstYRB, dstU += dstUVRB, dstV += dstUVRB) {

		/* Prime the pipeline */
		*dstY0++ = Y255toY219(*src0++, tmp);								/* Y(0,0) */
		*dstY1++ = Y255toY219(*src1++, tmp);								/* Y(0,1) */
		u0 = (UInt32)(*src0++) + (UInt32)(*src1++);							/* Add U(0,0) and U(0,1) */
		*dstY0++ = Y255toY219(*src0++, tmp);								/* Y(1,0) */
		*dstY1++ = Y255toY219(*src1++, tmp);								/* Y(1,1) */
		v0 = (UInt32)(*src0++) + (UInt32)(*src1++);							/* Add V(0,0) and V(0,1) */

		/* Core of the pipeline */
		for (i = width; i--; u0 = u1, v0 = v1) {							/* Actually width-1 */
			*dstY0++ = Y255toY219(*src0++, tmp);							/* Y(2*n+2,0) */
			*dstY1++ = Y255toY219(*src1++, tmp);							/* Y(2*n+2,1) */
			u1 = (UInt32)(*src0++) + (UInt32)(*src1++);						/* Add U(n,0) and U(n,1) */
			*dstY0++ = Y255toY219(*src0++, tmp);							/* Y(2*n+3,0) */
			*dstY1++ = Y255toY219(*src1++, tmp);							/* Y(2*n+3,1) */
			v1 = (UInt32)(*src0++) + (UInt32)(*src1++);						/* Add V(n,0) and V(n,1) */
			*dstU++ = C1024toC112((u0 << 2) - u0 + u1);						/* U(n+0,0), interpolated 1/4 of the way over */
			*dstV++ = C1024toC112((v0 << 2) - v0 + v1);						/* V(n+0,0), interpolated 1/4 of the way over */
		}
		
		/* Flush the pipeline */
		*dstU++ = C256toC112(u0);											/* Smear last chroma 1/4 pixel to the right */
		*dstV++ = C256toC112(v0);											/* Smear last chroma 1/4 pixel to the right */
	}
}


/********************************************************************************
 * FskFullSignedUYVY422toYUV420								u y v y
 *	srcY in [0,  255],	srcC in [-128, +127]
 *	dstY in [16, 235],	dstC in [16, 240]
 ********************************************************************************/

void
FskFullSignedUYVY422toYUV420(
	UInt32			width,
	UInt32			height,
	
	const UInt32	*src,				/* Cb' Y' Cr' Y' src */
	SInt32			srcRB,
	
	UInt8			*dstY0,				/* Y'  dst */
	SInt32			dstYRB,
	UInt8			*dstU,				/* Cb' dst */
	UInt8			*dstV,				/* Cr' dst */
	SInt32			dstUVRB
)
{
	const UInt8		*src0	= (const UInt8*)src;
	const UInt8		*src1	= src0  + srcRB;
	UInt8			*dstY1	= dstY0 + dstYRB;
	UInt32			i;
	SInt32			u0, u1, v0, v1;
	#ifndef USE_TABLES
		UInt32		tmp;
	#endif /* USE_TABLES */
	
	width  >>= 1;																	/* Chroma blocks */
	height >>= 1;																	/* Chroma blocks */
	if ((width == 0) ||(height == 0))
		return;
	
	srcRB   = (srcRB   << 1) - (SInt32)(width << 2);								/* srcBump   - 2 lines at a time minus a line of VYUY */
	dstYRB  = (dstYRB  << 1) - (SInt32)(width << 1);								/* dstYBump  - 2 lines at a time minus a line of YY */
	dstUVRB = (dstUVRB << 0) - (SInt32)(width << 0);								/* dstUVBump - 1 line  at a time minue a line of U (or V) */
	width--;																		/* Anticipate a prime of the X pipeline */

	for ( ; height--; src0 += srcRB, src1 += srcRB, dstY0 += dstYRB, dstY1 += dstYRB, dstU += dstUVRB, dstV += dstUVRB) {

		/* Prime the pipeline */
		u0 = ((SInt32)(*((SInt8*)src0++)) + (SInt32)(*((SInt8*)src1++)) + 256);		/* Add U(0,0) and U(0,1) */
		*dstY0++ = Y255toY219(*src0++, tmp);										/* Y(0,0) */
		*dstY1++ = Y255toY219(*src1++, tmp);										/* Y(0,1) */
		v0 = ((SInt32)(*((SInt8*)src0++)) + (SInt32)(*((SInt8*)src1++)) + 256);		/* Add V(0,0) and V(0,1) */
		*dstY0++ = Y255toY219(*src0++, tmp);										/* Y(1,0) */
		*dstY1++ = Y255toY219(*src1++, tmp);										/* Y(1,1) */

		/* Core of the pipeline */
		for (i = width; i--; u0 = u1, v0 = v1) {									/* Actually width-1 */
			u1 = ((SInt32)(*((SInt8*)src0++)) + (SInt32)(*((SInt8*)src1++)) + 256);	/* Add U(n,0) and U(n,1) */
			*dstY0++ = Y255toY219(*src0++, tmp);									/* Y(2*n+2,0) */
			*dstY1++ = Y255toY219(*src1++, tmp);									/* Y(2*n+2,1) */
			v1 = ((SInt32)(*((SInt8*)src0++)) + (SInt32)(*((SInt8*)src1++)) + 256);	/* Add V(n,0) and V(n,1) */
			*dstY0++ = Y255toY219(*src0++, tmp);									/* Y(2*n+3,0) */
			*dstY1++ = Y255toY219(*src1++, tmp);									/* Y(2*n+3,1) */
			*dstU++ = C1024toC112((u0 << 2) - u0 + u1);								/* U(n+0,0), interpolated 1/4 of the way over */
			*dstV++ = C1024toC112((v0 << 2) - v0 + v1);								/* V(n+0,0), interpolated 1/4 of the way over */
		}
		
		/* Flush the pipeline */
		*dstU++ = C256toC112(u0);													/* Smear last chroma 1/4 pixel to the right */
		*dstV++ = C256toC112(v0);													/* Smear last chroma 1/4 pixel to the right */
	}
}


/********************************************************************************
 * FskFullSignedYUYV422toYUV420								y u y v
 *	srcY in [0,  255],	srcC in [-128, +127]
 *	dstY in [16, 235],	dstC in [16, 240]
 ********************************************************************************/

void
FskFullSignedYUYV422toYUV420(
	UInt32			width,
	UInt32			height,
	
	const UInt32	*src,				/* Y' Cb' Y' Cr' src */
	SInt32			srcRB,
	
	UInt8			*dstY0,				/* Y'  dst */
	SInt32			dstYRB,
	UInt8			*dstU,				/* Cb' dst */
	UInt8			*dstV,				/* Cr' dst */
	SInt32			dstUVRB
)
{
	const UInt8		*src0	= (const UInt8*)src;
	const UInt8		*src1	= src0  + srcRB;
	UInt8			*dstY1	= dstY0 + dstYRB;
	UInt32			i;
	SInt32			u0, u1, v0, v1;
	#ifndef USE_TABLES
		UInt32		tmp;
	#endif /* USE_TABLES */
	
	width  >>= 1;																	/* Chroma blocks */
	height >>= 1;																	/* Chroma blocks */
	if ((width == 0) ||(height == 0))
		return;
	
	srcRB   = (srcRB   << 1) - (SInt32)(width << 2);								/* srcBump   - 2 lines at a time minus a line of VYUY */
	dstYRB  = (dstYRB  << 1) - (SInt32)(width << 1);								/* dstYBump  - 2 lines at a time minus a line of YY */
	dstUVRB = (dstUVRB << 0) - (SInt32)(width << 0);								/* dstUVBump - 1 line  at a time minue a line of U (or V) */
	width--;																		/* Anticipate a prime of the X pipeline */

	for ( ; height--; src0 += srcRB, src1 += srcRB, dstY0 += dstYRB, dstY1 += dstYRB, dstU += dstUVRB, dstV += dstUVRB) {

		/* Prime the pipeline */
		*dstY0++ = Y255toY219(*src0++, tmp);										/* Y(0,0) */
		*dstY1++ = Y255toY219(*src1++, tmp);										/* Y(0,1) */
		u0 = ((SInt32)(*((SInt8*)src0++)) + (SInt32)(*((SInt8*)src1++)) + 256);		/* Add U(0,0) and U(0,1) */
		*dstY0++ = Y255toY219(*src0++, tmp);										/* Y(1,0) */
		*dstY1++ = Y255toY219(*src1++, tmp);										/* Y(1,1) */
		v0 = ((SInt32)(*((SInt8*)src0++)) + (SInt32)(*((SInt8*)src1++)) + 256);		/* Add V(0,0) and V(0,1) */

		/* Core of the pipeline */
		for (i = width; i--; u0 = u1, v0 = v1) {									/* Actually width-1 */
			*dstY0++ = Y255toY219(*src0++, tmp);									/* Y(2*n+2,0) */
			*dstY1++ = Y255toY219(*src1++, tmp);									/* Y(2*n+2,1) */
			u1 = ((SInt32)(*((SInt8*)src0++)) + (SInt32)(*((SInt8*)src1++)) + 256);	/* Add U(n,0) and U(n,1) */
			*dstY0++ = Y255toY219(*src0++, tmp);									/* Y(2*n+3,0) */
			*dstY1++ = Y255toY219(*src1++, tmp);									/* Y(2*n+3,1) */
			v1 = ((SInt32)(*((SInt8*)src0++)) + (SInt32)(*((SInt8*)src1++)) + 256);	/* Add V(n,0) and V(n,1) */
			*dstU++ = C1024toC112((u0 << 2) - u0 + u1);								/* U(n+0,0), interpolated 1/4 of the way over */
			*dstV++ = C1024toC112((v0 << 2) - v0 + v1);								/* V(n+0,0), interpolated 1/4 of the way over */
		}
		
		/* Flush the pipeline */
		*dstU++ = C256toC112(u0);													/* Smear last chroma 1/4 pixel to the right */
		*dstV++ = C256toC112(v0);													/* Smear last chroma 1/4 pixel to the right */
	}
}


/********************************************************************************
 * FskSignedUYVY422toYUV420								u y v y
 *	srcY in [16, 240],	srcC in [-112, +112]
 *	dstY in [16, 235],	dstC in [16, 240]
 ********************************************************************************/

void
FskSignedUYVY422toYUV420(
	UInt32			width,
	UInt32			height,
	
	const UInt32	*src,				/* Cb' Y' Cr' Y' src */
	SInt32			srcRB,
	
	UInt8			*dstY0,				/* Y'  dst */
	SInt32			dstYRB,
	UInt8			*dstU,				/* Cb' dst */
	UInt8			*dstV,				/* Cr' dst */
	SInt32			dstUVRB
)
{
	const UInt8		*src0	= (const UInt8*)src;
	const UInt8		*src1	= src0  + srcRB;
	UInt8			*dstY1	= dstY0 + dstYRB;
	UInt32			i;
	SInt32			u0, u1, v0, v1;
	
	width  >>= 1;																	/* Chroma blocks */
	height >>= 1;																	/* Chroma blocks */
	if ((width == 0) ||(height == 0))
		return;
	
	srcRB   = (srcRB   << 1) - (SInt32)(width << 2);								/* srcBump   - 2 lines at a time minus a line of VYUY */
	dstYRB  = (dstYRB  << 1) - (SInt32)(width << 1);								/* dstYBump  - 2 lines at a time minus a line of YY */
	dstUVRB = (dstUVRB << 0) - (SInt32)(width << 0);								/* dstUVBump - 1 line  at a time minue a line of U (or V) */
	width--;																		/* Anticipate a prime of the X pipeline */

	for ( ; height--; src0 += srcRB, src1 += srcRB, dstY0 += dstYRB, dstY1 += dstYRB, dstU += dstUVRB, dstV += dstUVRB) {

		/* Prime the pipeline */
		u0 = (SInt32)(*((SInt8*)src0++)) + (SInt32)(*((SInt8*)src1++));				/* Add U(0,0) and U(0,1) */
		*dstY0++ = *src0++;															/* Y(0,0) */
		*dstY1++ = *src1++;															/* Y(0,1) */
		v0 = (SInt32)(*((SInt8*)src0++)) + (SInt32)(*((SInt8*)src1++));				/* Add V(0,0) and V(0,1) */
		*dstY0++ = *src0++;															/* Y(1,0) */
		*dstY1++ = *src1++;															/* Y(1,1) */

		/* Core of the pipeline */
		for (i = width; i--; u0 = u1, v0 = v1) {									/* Actually width-1 */
			u1 = (SInt32)(*((SInt8*)src0++)) + (SInt32)(*((SInt8*)src1++));			/* Add U(n,0) and U(n,1) */
			*dstY0++ = *src0++;														/* Y(2*n+2,0) */
			*dstY1++ = *src1++;														/* Y(2*n+2,1) */
			v1 = (SInt32)(*((SInt8*)src0++)) + (SInt32)(*((SInt8*)src1++));			/* Add V(n,0) and V(n,1) */
			*dstY0++ = *src0++;														/* Y(2*n+3,0) */
			*dstY1++ = *src1++;														/* Y(2*n+3,1) */
			*dstU++ = CS896toC112((u0 << 2) - u0 + u1);								/* U(n+0,0), interpolated 1/4 of the way over */
			*dstV++ = CS896toC112((v0 << 2) - v0 + v1);								/* V(n+0,0), interpolated 1/4 of the way over */
		}
		
		/* Flush the pipeline */
		*dstU++ = CS224toC112(u0);													/* Smear last chroma 1/4 pixel to the right */
		*dstV++ = CS224toC112(v0);													/* Smear last chroma 1/4 pixel to the right */
	}
}


/********************************************************************************
 * FskSignedYUYV422toYUV420								y u y v
 *	srcY in [16, 240],	srcC in [-112, +112]
 *	dstY in [16, 235],	dstC in [16, 240]
 ********************************************************************************/

void
FskSignedYUYV422toYUV420(
	UInt32			width,
	UInt32			height,
	
	const UInt32	*src,				/* Y' Cb' Y' Cr' src */
	SInt32			srcRB,
	
	UInt8			*dstY0,				/* Y'  dst */
	SInt32			dstYRB,
	UInt8			*dstU,				/* Cb' dst */
	UInt8			*dstV,				/* Cr' dst */
	SInt32			dstUVRB
)
{
	const UInt8		*src0	= (const UInt8*)src;
	const UInt8		*src1	= src0  + srcRB;
	UInt8			*dstY1	= dstY0 + dstYRB;
	UInt32			i;
	SInt32			u0, u1, v0, v1;
	
	width  >>= 1;																	/* Chroma blocks */
	height >>= 1;																	/* Chroma blocks */
	if ((width == 0) ||(height == 0))
		return;
	
	srcRB   = (srcRB   << 1) - (SInt32)(width << 2);								/* srcBump   - 2 lines at a time minus a line of VYUY */
	dstYRB  = (dstYRB  << 1) - (SInt32)(width << 1);								/* dstYBump  - 2 lines at a time minus a line of YY */
	dstUVRB = (dstUVRB << 0) - (SInt32)(width << 0);								/* dstUVBump - 1 line  at a time minue a line of U (or V) */
	width--;																		/* Anticipate a prime of the X pipeline */

	for ( ; height--; src0 += srcRB, src1 += srcRB, dstY0 += dstYRB, dstY1 += dstYRB, dstU += dstUVRB, dstV += dstUVRB) {

		/* Prime the pipeline */
		*dstY0++ = *src0++;															/* Y(0,0) */
		*dstY1++ = *src1++;															/* Y(0,1) */
		u0 = (SInt32)(*((SInt8*)src0++)) + (SInt32)(*((SInt8*)src1++));				/* Add U(0,0) and U(0,1) */
		*dstY0++ = *src0++;															/* Y(1,0) */
		*dstY1++ = *src1++;															/* Y(1,1) */
		v0 = (SInt32)(*((SInt8*)src0++)) + (SInt32)(*((SInt8*)src1++));				/* Add V(0,0) and V(0,1) */

		/* Core of the pipeline */
		for (i = width; i--; u0 = u1, v0 = v1) {									/* Actually width-1 */
			*dstY0++ = *src0++;														/* Y(2*n+2,0) */
			*dstY1++ = *src1++;														/* Y(2*n+2,1) */
			u1 = (SInt32)(*((SInt8*)src0++)) + (SInt32)(*((SInt8*)src1++));			/* Add U(n,0) and U(n,1) */
			*dstY0++ = *src0++;														/* Y(2*n+3,0) */
			*dstY1++ = *src1++;														/* Y(2*n+3,1) */
			v1 = (SInt32)(*((SInt8*)src0++)) + (SInt32)(*((SInt8*)src1++));			/* Add V(n,0) and V(n,1) */
			*dstU++ = CS896toC112((u0 << 2) - u0 + u1);								/* U(n+0,0), interpolated 1/4 of the way over */
			*dstV++ = CS896toC112((v0 << 2) - v0 + v1);								/* V(n+0,0), interpolated 1/4 of the way over */
		}
		
		/* Flush the pipeline */
		*dstU++ = CS224toC112(u0);													/* Smear last chroma 1/4 pixel to the right */
		*dstV++ = CS224toC112(v0);													/* Smear last chroma 1/4 pixel to the right */
	}
}



#if PRAGMA_MARK_SUPPORTED
#pragma mark -
#endif /* PRAGMA_MARK_SUPPORTED */
/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****				4:2:0 for H.262, H.264, MPEG-2, MPEG-4					*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/




/********************************************************************************
 * FskSignedUYVY422toMPEG4YUV420								u y v y
 *	srcY in [16, 240],	srcC in [-112, +112]
 *	dstY in [16, 235],	dstC in [16, 240]
 ********************************************************************************/

void
FskSignedUYVY422toMPEG4YUV420(
	UInt32			width,
	UInt32			height,
	
	const UInt32	*src,				/* Y' Cb' Y' Cr' src */
	SInt32			srcRB,
	
	UInt8			*dstY0,				/* Y'  dst */
	SInt32			dstYRB,
	UInt8			*dstU,				/* Cb' dst */
	UInt8			*dstV,				/* Cr' dst */
	SInt32			dstUVRB
)
{
	const UInt8		*src0	= (const UInt8*)src;
	const UInt8		*src1	= src0  + srcRB;
	UInt8			*dstY1	= dstY0 + dstYRB;
	UInt32			i;
	
	width  >>= 1;																	/* Chroma blocks */
	height >>= 1;																	/* Chroma blocks */
	if ((width == 0) ||(height == 0))
		return;
	
	srcRB   = (srcRB   << 1) - (SInt32)(width << 2);								/* srcBump   - 2 lines at a time minus a line of VYUY */
	dstYRB  = (dstYRB  << 1) - (SInt32)(width << 1);								/* dstYBump  - 2 lines at a time minus a line of YY */
	dstUVRB = (dstUVRB << 0) - (SInt32)(width << 0);								/* dstUVBump - 1 line  at a time minue a line of U (or V) */

	for ( ; height--; src0 += srcRB, src1 += srcRB, dstY0 += dstYRB, dstY1 += dstYRB, dstU += dstUVRB, dstV += dstUVRB) {
		for (i = width; i--; ) {
			*dstU++  = CS224toC112((SInt32)(*((SInt8*)src0++)) + (SInt32)(*((SInt8*)src1++)));	/* U(n+0,0) */
			*dstY0++ = *src0++;																	/* Y(2*n+2,0) */
			*dstY1++ = *src1++;																	/* Y(2*n+2,1) */
			*dstV++  = CS224toC112((SInt32)(*((SInt8*)src0++)) + (SInt32)(*((SInt8*)src1++)));	/* V(n+0,0) */
			*dstY0++ = *src0++;																	/* Y(2*n+3,0) */
			*dstY1++ = *src1++;																	/* Y(2*n+3,1) */
		}
	}
}


/********************************************************************************
 * FskSignedYUYV422toMPEG4YUV420								y u y v
 *	srcY in [16, 240],	srcC in [-112, +112]
 *	dstY in [16, 235],	dstC in [16, 240]
 ********************************************************************************/

void
FskSignedYUYV422toMPEG4YUV420(
	UInt32			width,
	UInt32			height,
	
	const UInt32	*src,				/* Y' Cb' Y' Cr' src */
	SInt32			srcRB,
	
	UInt8			*dstY0,				/* Y'  dst */
	SInt32			dstYRB,
	UInt8			*dstU,				/* Cb' dst */
	UInt8			*dstV,				/* Cr' dst */
	SInt32			dstUVRB
)
{
	const UInt8		*src0	= (const UInt8*)src;
	const UInt8		*src1	= src0  + srcRB;
	UInt8			*dstY1	= dstY0 + dstYRB;
	UInt32			i;
	
	width  >>= 1;																	/* Chroma blocks */
	height >>= 1;																	/* Chroma blocks */
	if ((width == 0) ||(height == 0))
		return;
	
	srcRB   = (srcRB   << 1) - (SInt32)(width << 2);								/* srcBump   - 2 lines at a time minus a line of VYUY */
	dstYRB  = (dstYRB  << 1) - (SInt32)(width << 1);								/* dstYBump  - 2 lines at a time minus a line of YY */
	dstUVRB = (dstUVRB << 0) - (SInt32)(width << 0);								/* dstUVBump - 1 line  at a time minue a line of U (or V) */

	for ( ; height--; src0 += srcRB, src1 += srcRB, dstY0 += dstYRB, dstY1 += dstYRB, dstU += dstUVRB, dstV += dstUVRB) {
		for (i = width; i--; ) {
			*dstY0++ = *src0++;																	/* Y(2*n+2,0) */
			*dstY1++ = *src1++;																	/* Y(2*n+2,1) */
			*dstU++  = CS224toC112((SInt32)(*((SInt8*)src0++)) + (SInt32)(*((SInt8*)src1++)));	/* U(n+0,0) */
			*dstY0++ = *src0++;																	/* Y(2*n+3,0) */
			*dstY1++ = *src1++;																	/* Y(2*n+3,1) */
			*dstV++  = CS224toC112((SInt32)(*((SInt8*)src0++)) + (SInt32)(*((SInt8*)src1++)));	/* V(n+0,0) */
		}
	}
}


/********************************************************************************
 * FskWideSignedYUYV422toMPEG4YUV420
 ********************************************************************************/

void
FskWideSignedYUYV422toMPEG4YUV420(
	UInt32			width,
	UInt32			height,
	
	const UInt32	*src,				/* Y' Cb' Y' Cr' src */
	SInt32			srcRB,
	
	UInt8			*dstY0,				/* Y'  dst */
	SInt32			dstYRB,
	UInt8			*dstU,				/* Cb' dst */
	UInt8			*dstV,				/* Cr' dst */
	SInt32			dstUVRB
)
{
	const UInt8		*src0	= (const UInt8*)src;
	const UInt8		*src1	= src0  + srcRB;
	UInt8			*dstY1	= dstY0 + dstYRB;
	UInt32			i;
	#ifndef USE_TABLES
		UInt32		tmp;
	#endif /* USE_TABLES */
	
	
	width  >>= 1;																	/* Chroma blocks */
	height >>= 1;																	/* Chroma blocks */
	if ((width == 0) ||(height == 0))
		return;
	
	srcRB   = (srcRB   << 1) - (SInt32)(width << 2);								/* srcBump   - 2 lines at a time minus a line of VYUY */
	dstYRB  = (dstYRB  << 1) - (SInt32)(width << 1);								/* dstYBump  - 2 lines at a time minus a line of YY */
	dstUVRB = (dstUVRB << 0) - (SInt32)(width << 0);								/* dstUVBump - 1 line  at a time minue a line of U (or V) */

	for ( ; height--; src0 += srcRB, src1 += srcRB, dstY0 += dstYRB, dstY1 += dstYRB, dstU += dstUVRB, dstV += dstUVRB) {
		for (i = width; i--; ) {
			*dstY0++ = Y255toY219(*src0++, tmp);												/* Y(0,0) */
			*dstY1++ = Y255toY219(*src1++, tmp);												/* Y(0,1) */
			*dstU++  = CS254toC112((SInt32)(*((SInt8*)src0++)) + (SInt32)(*((SInt8*)src1++)));	/* U(n+0,0) */
			*dstY0++ = Y255toY219(*src0++, tmp);												/* Y(1,0) */
			*dstY1++ = Y255toY219(*src1++, tmp);												/* Y(1,1) */
			*dstV++  = CS254toC112((SInt32)(*((SInt8*)src0++)) + (SInt32)(*((SInt8*)src1++)));	/* V(n+0,0) */
		}
	}
}


/********************************************************************************
 * FskWideSignedUYVY422toMPEG4YUV420
 ********************************************************************************/

void
FskWideSignedUYVY422toMPEG4YUV420(
	UInt32			width,
	UInt32			height,
	
	const UInt32	*src,				/* Cb' Y' Cr' Y' src */
	SInt32			srcRB,
	
	UInt8			*dstY0,				/* Y'  dst */
	SInt32			dstYRB,
	UInt8			*dstU,				/* Cb' dst */
	UInt8			*dstV,				/* Cr' dst */
	SInt32			dstUVRB
)
{
	const UInt8		*src0	= (const UInt8*)src;
	const UInt8		*src1	= src0  + srcRB;
	UInt8			*dstY1	= dstY0 + dstYRB;
	UInt32			i;
	#ifndef USE_TABLES
		UInt32		tmp;
	#endif /* USE_TABLES */
	
	
	width  >>= 1;																	/* Chroma blocks */
	height >>= 1;																	/* Chroma blocks */
	if ((width == 0) ||(height == 0))
		return;
	
	srcRB   = (srcRB   << 1) - (SInt32)(width << 2);								/* srcBump   - 2 lines at a time minus a line of VYUY */
	dstYRB  = (dstYRB  << 1) - (SInt32)(width << 1);								/* dstYBump  - 2 lines at a time minus a line of YY */
	dstUVRB = (dstUVRB << 0) - (SInt32)(width << 0);								/* dstUVBump - 1 line  at a time minue a line of U (or V) */

	for ( ; height--; src0 += srcRB, src1 += srcRB, dstY0 += dstYRB, dstY1 += dstYRB, dstU += dstUVRB, dstV += dstUVRB) {
		for (i = width; i--; ) {
			*dstU++  = CS254toC112((SInt32)(*((SInt8*)src0++)) + (SInt32)(*((SInt8*)src1++)));	/* U(n+0,0) */
			*dstY0++ = Y255toY219(*src0++, tmp);												/* Y(0,0) */
			*dstY1++ = Y255toY219(*src1++, tmp);												/* Y(0,1) */
			*dstV++  = CS254toC112((SInt32)(*((SInt8*)src0++)) + (SInt32)(*((SInt8*)src1++)));	/* V(n+0,0) */
			*dstY0++ = Y255toY219(*src0++, tmp);												/* Y(1,0) */
			*dstY1++ = Y255toY219(*src1++, tmp);												/* Y(1,1) */
		}
	}
}


/********************************************************************************
 * FskYUYV422toMPEG4YUV420
 ********************************************************************************/

void
FskYUYV422toMPEG4YUV420(
	UInt32			width,
	UInt32			height,
	
	const UInt32	*src,				/* Y' Cb' Y' Cr' src */
	SInt32			srcRB,
	
	UInt8			*dstY0,				/* Y'  dst */
	SInt32			dstYRB,
	UInt8			*dstU,				/* Cb' dst */
	UInt8			*dstV,				/* Cr' dst */
	SInt32			dstUVRB
)
{
	const UInt8		*src0	= (const UInt8*)src;
	const UInt8		*src1	= src0  + srcRB;
	UInt8			*dstY1	= dstY0 + dstYRB;
	UInt32			i;
	
	width  >>= 1;																	/* Chroma blocks */
	height >>= 1;																	/* Chroma blocks */
	if ((width == 0) ||(height == 0))
		return;
	
	srcRB   = (srcRB   << 1) - (SInt32)(width << 2);								/* srcBump   - 2 lines at a time minus a line of VYUY */
	dstYRB  = (dstYRB  << 1) - (SInt32)(width << 1);								/* dstYBump  - 2 lines at a time minus a line of YY */
	dstUVRB = (dstUVRB << 0) - (SInt32)(width << 0);								/* dstUVBump - 1 line  at a time minue a line of U (or V) */

	for ( ; height--; src0 += srcRB, src1 += srcRB, dstY0 += dstYRB, dstY1 += dstYRB, dstU += dstUVRB, dstV += dstUVRB) {
		for (i = width; i--; ) {
			*dstY0++ = *src0++;														/* Y(2*n+2,0) */
			*dstY1++ = *src1++;														/* Y(2*n+2,1) */
			*dstU++ = C224toC112(*src0++ + *src1++);								/* U(n+0,0) */
			*dstY0++ = *src0++;														/* Y(2*n+3,0) */
			*dstY1++ = *src1++;														/* Y(2*n+3,1) */
			*dstV++ = C224toC112(*src0++ + *src1++);								/* V(n+0,0) */
		}
	}
}


/********************************************************************************
 * FskUYVY422toMPEG4YUV420
 ********************************************************************************/

void
FskUYVY422toMPEG4YUV420(
	UInt32			width,
	UInt32			height,
	
	const UInt32	*src,				/* Cb' Y' Cr' Y' src */
	SInt32			srcRB,
	
	UInt8			*dstY0,				/* Y'  dst */
	SInt32			dstYRB,
	UInt8			*dstU,				/* Cb' dst */
	UInt8			*dstV,				/* Cr' dst */
	SInt32			dstUVRB
)
{
	const UInt8		*src0	= (const UInt8*)src;
	const UInt8		*src1	= src0  + srcRB;
	UInt8			*dstY1	= dstY0 + dstYRB;
	UInt32			i;
	
	width  >>= 1;																	/* Chroma blocks */
	height >>= 1;																	/* Chroma blocks */
	if ((width == 0) ||(height == 0))
		return;
	
	srcRB   = (srcRB   << 1) - (SInt32)(width << 2);								/* srcBump   - 2 lines at a time minus a line of VYUY */
	dstYRB  = (dstYRB  << 1) - (SInt32)(width << 1);								/* dstYBump  - 2 lines at a time minus a line of YY */
	dstUVRB = (dstUVRB << 0) - (SInt32)(width << 0);								/* dstUVBump - 1 line  at a time minue a line of U (or V) */

	for ( ; height--; src0 += srcRB, src1 += srcRB, dstY0 += dstYRB, dstY1 += dstYRB, dstU += dstUVRB, dstV += dstUVRB) {
		for (i = width; i--; ) {
			*dstU++ = C224toC112(*src0++ + *src1++);								/* U(n+0,0) */
			*dstY0++ = *src0++;														/* Y(2*n+2,0) */
			*dstY1++ = *src1++;														/* Y(2*n+2,1) */
			*dstV++ = C224toC112(*src0++ + *src1++);								/* V(n+0,0) */
			*dstY0++ = *src0++;														/* Y(2*n+3,0) */
			*dstY1++ = *src1++;														/* Y(2*n+3,1) */
		}
	}
}




/********************************************************************************
 * main for making tables
	cc -I.. -DMAKE_TABLES FskYUV422toYUV420.c
	./a.out
 ********************************************************************************/

#ifdef MAKE_TABLES
#include <stdio.h>

int
main(int argc, char ***argv)
{
	FILE	*fd		= stdout;
	int		f, g;
	
	/* Y */
	fprintf(fd, "static const UInt8 TabFullToVideoLuma[256] = {\n");
	for (f = 0; f < 256; f++) {
		g = (int)(f * 219.0 / 255.0 + 16.0 + 0.5);
		fprintf(fd, "\t%d,\t/* %d */\n", g, f);
	}
	fprintf(fd, "};\n");
	
	/* C */
	fprintf(fd, "static const UInt8 TabFullToVideoChroma[256] = {\n");
	for (f = 0; f < 256; f++) {
		g = (int)((f - 128) * 112.0 / 128.0 + 128.0 + 0.5);
		fprintf(fd, "\t%d,\t/* %3d or %4d */\n", g, f, f - 128);
	}
	fprintf(fd, "};\n");
	

	fprintf(fd, "static const UInt8 TabWideToVideoChroma[256] = {\n");
	for (f = 0; f < 256; f++) {
		g = (int)((f - 128) * 113.0 / 128.0 + 128.0 + 0.5);
		fprintf(fd, "\t%d,\t/* %3d or %4d */\n", g, f, f - 128);
	}
	fprintf(fd, "};\n");
	
}

#endif /* MAKE_TABLES */


#ifdef TEST
/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****	TEST - TEST - TEST - TEST - TEST - TEST - TEST - TEST - TEST - TEST	*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "KTYCbCr.h"

/********************************************************************************
 * main
	cc -DTEST -I.. -I/Volumes/Shannon/turk/Graphics/KTLib/inc \
	FskYUV422toYUV420.c /Volumes/Shannon/turk/Graphics/KTLib/lib/build/libKTLib.a
	./a.out
 ********************************************************************************/

int
main(int argc, char **argv)
{
	FILE			*fd				= NULL;
	UInt8			*src422			= NULL;
	UInt8			*mid420			= NULL;
	UInt32			*argb			= NULL;
	const UInt32	width			= 640;
	const UInt32	height			= 480;
	const UInt32	srcRowBytes		= width * 2;
	const UInt32	srcBytes		= srcRowBytes * height;
	const UInt32	midYRowBytes	= width;
	const UInt32	midCRowBytes	= width / 2;
	const UInt32	midBytes		= midYRowBytes * height + midCRowBytes * height / 2 * 2;
	const UInt32	argbRowBytes	= width * 4;
	const UInt32	argbBytes		= argbRowBytes * height;
	UInt8		*midU, *midV;
	
	if (	((src422 = malloc(srcBytes)) != NULL)
		&&	((mid420 = malloc(midBytes)) != NULL)
		&&	((argb   = malloc(argbBytes)) != NULL)
		&&	((fd = fopen("/Volumes/Shannon/turk/Downloads/image.422", "rb")) != NULL)
		&&	(fread(src422, 1, srcBytes, fd) == srcBytes)
	) {
		fclose(fd);
		fd = NULL;
		midU = mid420 + midYRowBytes * height;
		midV = midU   + midCRowBytes * height / 2;
#if 0
		FskYUYV422toYUV420(width, height, (UInt32*)src422, srcRowBytes, mid420, midYRowBytes, midU, midV, midCRowBytes);
#elif 0
		FskFullYUYV422toYUV420(width, height, (UInt32*)src422, srcRowBytes, mid420, midYRowBytes, midU, midV, midCRowBytes);
#elif 0
		FskFullSignedYUYV422toYUV420(width, height, (UInt32*)src422, srcRowBytes, mid420, midYRowBytes, midU, midV, midCRowBytes);
#elif 0
		FskSignedYUYV422toYUV420(width, height, (UInt32*)src422, srcRowBytes, mid420, midYRowBytes, midU, midV, midCRowBytes);
#else
		FskSignedYUYV422toMPEG4YUV420(width, height, (UInt32*)src422, srcRowBytes, mid420, midYRowBytes, midU, midV, midCRowBytes);
#endif
		KTCopyRectPlanarYUV420To32ARGB(mid420, midU, midV, midYRowBytes, midCRowBytes, (void*)argb, argbRowBytes, width, height);
		if ((fd = fopen("/Volumes/Shannon/turk/Downloads/image.argb", "wb")) != NULL) {
			fwrite(argb, 1, argbBytes, fd);
			fclose(fd);
			fd = NULL;
		}
	}
	
	if (src422 != NULL)	free(src422);
	if (mid420 != NULL)	free(mid420);
	if (argb != NULL)	free(argb);
	if (fd != NULL)		fclose(fd);
}

#endif /* TEST */
