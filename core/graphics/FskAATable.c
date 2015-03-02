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
//#define GENERATE_TABLES
#ifdef GENERATE_TABLES

#include <stdio.h>
#include <math.h>

#define TWO_SQRT_LN_2	1.6651092223153955127


/********************************************************************************
 * PrintGaussianPSF
 ********************************************************************************/

static void
PrintGaussianPSF(FILE *fd, double width, double increment, double scale, char *name)
{
	double	a, x;
	long	n, g;
	
	a = TWO_SQRT_LN_2 / width;
	a = -a * a;
	for (n = 0, x = 0; 0 != lround(exp(a * x * x) * scale); n++, x += increment) ;
	fprintf(fd, "extern %s[%d];	/* Width = %g, inc = 1/%g */\n", name, n, width, 1.0/increment);
	fprintf(fd, "%s[%d] = {	/* Width = %g, inc = 1/%g */\n", name, n, width, 1.0/increment);
	
	for (x = 0; n--; x += increment) {
		g = lround(exp(a * x * x) * scale);
		fprintf(fd, "\t%d%c	/* %7.5f %s*/\n", g, ((n == 0) ? ' ' : ','), x, ((g == 0) ? "(wasted) " : ""));
	}
	fprintf(fd, "};\n\n");
}


/********************************************************************************
 * PrintErf
 ********************************************************************************/

static void
PrintErf(FILE *fd, double width, double increment, double scale, char *name)
{
	double	a, x;
	long	n, g;
	
	a = TWO_SQRT_LN_2 / width;
	for (n = 0, x = 0; scale != lround(erf(a * x) * scale); n++, x += increment) ;
	fprintf(fd, "extern %s[%d];	/* Width = %g, inc = 1/%g */\n", name, n, width, 1.0/increment);
	fprintf(fd, "%s[%d] = {	/* Width = %g, inc = 1/%g */\n", name, n, width, 1.0/increment);

	for (x = 0; n--; x += increment) {
		g = lround(erf(a * x) * scale);
		fprintf(fd, "\t%d%c	/* %7.5f %s*/\n", g, ((n == 0) ? ' ' : ','), x, ((g == scale) ? "(wasted) " : ""));
	}
	fprintf(fd, "};\n\n");
}


/********************************************************************************
 * PrintErfc
 ********************************************************************************/

static void
PrintErfc(FILE *fd, double width, double increment, double scale, char *name)
{
	double	a, x;
	long	n, g;
	
	a = TWO_SQRT_LN_2 / width;
	for (n = 0, x = 0; 0 != lround(erfc(a * x) * scale); n++, x += increment) ;
	fprintf(fd, "extern %s[%d];	/* Width = %g, inc = 1/%g */\n", name, n, width, 1.0/increment);
	fprintf(fd, "%s[%d] = {	/* Width = %g, inc = 1/%g */\n", name, n, width, 1.0/increment);

	for (x = 0; n--; x += increment) {
		g = lround(erfc(a * x) * scale);
		fprintf(fd, "\t%d%c	/* %7.5f %s*/\n", g, ((n == 0) ? ' ' : ','), x, ((g == 0) ? "(wasted) " : ""));
	}
	fprintf(fd, "};\n\n");
}


/********************************************************************************
 * PrintGaussConvolved
 ********************************************************************************/

static void
PrintGaussConvolved(FILE *fd, double width, double increment, double scale, char *name)
{
	double	a, x, s;
	long	n, g;
	
	a = TWO_SQRT_LN_2 / width;
	s = erf(a * 0.5) - erf(a * -0.5);
	for (n = 0, x = 0; 0 != lround((erf(a * (x + 0.5)) - erf(a * (x - 0.5))) / s * scale); n++, x += increment) ;
	fprintf(fd, "extern %s[%d];	/* Width = %g, inc = 1/%g */\n", name, n, width, 1.0/increment);
	fprintf(fd, "%s[%d] = {	/* Width = %g, inc = 1/%g */\n", name, n, width, 1.0/increment);

	for (x = 0; n--; x += increment) {
		g = lround((erf(a * (x + 0.5)) - erf(a * (x - 0.5))) / s * scale);
		fprintf(fd, "\t%d%c	/* %7.5f %s*/\n", g, ((n == 0) ? ' ' : ','), x, ((g == 0) ? "(wasted) " : ""));
	}
	fprintf(fd, "};\n\n");
}


/********************************************************************************
 * main
 ********************************************************************************/

int
main(int argc, char **argv)
{
	char	*outFilename = "./filter.out.c";
	FILE 	*fd;
	double	inc;
	
//	if ((fd = fopen(outFilename, "w")) == NULL)
		fd = stdout;

	inc		= 1.0 / 32.0;

	PrintGaussianPSF(   fd, 1,       inc, 255, "const UInt8 gFskGaussianNarrowLineFilter255_32");
	PrintGaussianPSF(   fd, sqrt(2), inc, 255, "const UInt8 gFskGaussianWideLineFilter255_32");
	PrintGaussConvolved(fd, 1,       inc, 255, "const UInt8 gFskGaussRectConvLineFilter255_32");
	PrintErfc       (   fd, 1,       inc, 127, "const UInt8 gFskGaussianNarrowPolygonFilter127_32");
	PrintErfc       (   fd, sqrt(2), inc, 127, "const UInt8 gFskGaussianWidePolygonFilter127_32");

#if 0
	PrintErf        (   fd, 1,       inc, 255, "const UInt8 gFskGaussianNarrowPolygonFilter255_32");
	PrintErf        (   fd, sqrt(2), inc, 255, "const UInt8 gFskGaussianWidePolygonFilter255_32");
	PrintGaussianPSF(   fd, 1,       inc, 128, "const UInt8 gFskGaussianNarrowLineFilter128_32");
	PrintErf        (   fd, 1,       inc, 128, "const UInt8 gFskGaussianNarrowPolygonFilter128_32");
	PrintGaussianPSF(   fd, sqrt(2), inc, 128, "const UInt8 gFskGaussianWideLineFilter128_32");
	PrintErf        (   fd, sqrt(2), inc, 128, "const UInt8 gFskGaussianWidePolygonFilter128_32");
	PrintErf        (   fd, 1,       inc, 256, "const UInt8 gFskGaussianNarrowPolygonFilter256_32");
	PrintErf        (   fd, sqrt(2), inc, 256, "const UInt8 gFskGaussianWidePolygonFilter256_32");
#endif

	if (fd != stdout)
		fclose(fd);
}
#endif /* GENERATE_TABLES */


#include "FskAATable.h"


/***************************************************************/
/* DO NOT REMOVE THIS LINE OR YOU WILL SUFFER THE CONSEQUENCES */
/***************************************************************/



const UInt8 gFskGaussRectConvLineFilter255_32[57] = {	/* Width = 1, inc = 1/32 */
	255,	/* 0.00000 */
	255,	/* 0.03125 */
	253,	/* 0.06250 */
	251,	/* 0.09375 */
	248,	/* 0.12500 */
	245,	/* 0.15625 */
	240,	/* 0.18750 */
	235,	/* 0.21875 */
	229,	/* 0.25000 */
	222,	/* 0.28125 */
	215,	/* 0.31250 */
	208,	/* 0.34375 */
	200,	/* 0.37500 */
	191,	/* 0.40625 */
	183,	/* 0.43750 */
	174,	/* 0.46875 */
	164,	/* 0.50000 */
	155,	/* 0.53125 */
	146,	/* 0.56250 */
	137,	/* 0.59375 */
	127,	/* 0.62500 */
	118,	/* 0.65625 */
	110,	/* 0.68750 */
	101,	/* 0.71875 */
	93,	/* 0.75000 */
	85,	/* 0.78125 */
	77,	/* 0.81250 */
	70,	/* 0.84375 */
	63,	/* 0.87500 */
	57,	/* 0.90625 */
	51,	/* 0.93750 */
	45,	/* 0.96875 */
	40,	/* 1.00000 */
	35,	/* 1.03125 */
	31,	/* 1.06250 */
	27,	/* 1.09375 */
	24,	/* 1.12500 */
	20,	/* 1.15625 */
	18,	/* 1.18750 */
	15,	/* 1.21875 */
	13,	/* 1.25000 */
	11,	/* 1.28125 */
	9,	/* 1.31250 */
	8,	/* 1.34375 */
	7,	/* 1.37500 */
	6,	/* 1.40625 */
	5,	/* 1.43750 */
	4,	/* 1.46875 */
	3,	/* 1.50000 */
	3,	/* 1.53125 */
	2,	/* 1.56250 */
	2,	/* 1.59375 */
	1,	/* 1.62500 */
	1,	/* 1.65625 */
	1,	/* 1.68750 */
	1,	/* 1.71875 */
	1 	/* 1.75000 */
};

const UInt8 gFskGaussianNarrowPolygonFilter127_32[40] = {	/* Width = 1, inc = 1/32 */
	127,	/* 0.00000 */
	120,	/* 0.03125 */
	112,	/* 0.06250 */
	105,	/* 0.09375 */
	98,	/* 0.12500 */
	91,	/* 0.15625 */
	84,	/* 0.18750 */
	77,	/* 0.21875 */
	71,	/* 0.25000 */
	64,	/* 0.28125 */
	59,	/* 0.31250 */
	53,	/* 0.34375 */
	48,	/* 0.37500 */
	43,	/* 0.40625 */
	38,	/* 0.43750 */
	34,	/* 0.46875 */
	30,	/* 0.50000 */
	27,	/* 0.53125 */
	24,	/* 0.56250 */
	21,	/* 0.59375 */
	18,	/* 0.62500 */
	16,	/* 0.65625 */
	13,	/* 0.68750 */
	11,	/* 0.71875 */
	10,	/* 0.75000 */
	8,	/* 0.78125 */
	7,	/* 0.81250 */
	6,	/* 0.84375 */
	5,	/* 0.87500 */
	4,	/* 0.90625 */
	3,	/* 0.93750 */
	3,	/* 0.96875 */
	2,	/* 1.00000 */
	2,	/* 1.03125 */
	2,	/* 1.06250 */
	1,	/* 1.09375 */
	1,	/* 1.12500 */
	1,	/* 1.15625 */
	1,	/* 1.18750 */
	1 	/* 1.21875 */
};

const UInt8 gFskGaussianWidePolygonFilter127_32[56] = {	/* Width = 1.41421, inc = 1/32 */
	127,	/* 0.00000 */
	122,	/* 0.03125 */
	116,	/* 0.06250 */
	111,	/* 0.09375 */
	106,	/* 0.12500 */
	101,	/* 0.15625 */
	96,	/* 0.18750 */
	91,	/* 0.21875 */
	86,	/* 0.25000 */
	81,	/* 0.28125 */
	77,	/* 0.31250 */
	72,	/* 0.34375 */
	68,	/* 0.37500 */
	63,	/* 0.40625 */
	59,	/* 0.43750 */
	55,	/* 0.46875 */
	51,	/* 0.50000 */
	48,	/* 0.53125 */
	44,	/* 0.56250 */
	41,	/* 0.59375 */
	38,	/* 0.62500 */
	35,	/* 0.65625 */
	32,	/* 0.68750 */
	29,	/* 0.71875 */
	27,	/* 0.75000 */
	25,	/* 0.78125 */
	22,	/* 0.81250 */
	20,	/* 0.84375 */
	18,	/* 0.87500 */
	17,	/* 0.90625 */
	15,	/* 0.93750 */
	14,	/* 0.96875 */
	12,	/* 1.00000 */
	11,	/* 1.03125 */
	10,	/* 1.06250 */
	9,	/* 1.09375 */
	8,	/* 1.12500 */
	7,	/* 1.15625 */
	6,	/* 1.18750 */
	5,	/* 1.21875 */
	5,	/* 1.25000 */
	4,	/* 1.28125 */
	4,	/* 1.31250 */
	3,	/* 1.34375 */
	3,	/* 1.37500 */
	2,	/* 1.40625 */
	2,	/* 1.43750 */
	2,	/* 1.46875 */
	2,	/* 1.50000 */
	1,	/* 1.53125 */
	1,	/* 1.56250 */
	1,	/* 1.59375 */
	1,	/* 1.62500 */
	1,	/* 1.65625 */
	1,	/* 1.68750 */
	1 	/* 1.71875 */
};


