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

#ifndef __KPDHCLIPPOLYGON__
#define __KPDHCLIPPOLYGON__


#ifndef __KPMATRIX__
	#include "KPMatrix.h"
#endif /* __KPMATRIX__ */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */





#define KPDHCLIP_PLANES	4			/* The number of clipping planes */
#define KPDHCLIP_ALLIN	0
#define KPDHCLIP_ALLOUT	((1<<KPDHCLIP_PLANES)-1)	/* 4 bits on */

/* Determine vertex location in 3D space */
#define KPDHCLIP_LEFT	0
#define KPDHCLIP_RIGHT	1
#define KPDHCLIP_TOP	2
#define KPDHCLIP_BOTTOM	3
#define KPDHCLIP_OUTLFT	(1<<KPDHCLIP_LEFT)
#define KPDHCLIP_OUTRGT	(1<<KPDHCLIP_RIGHT)
#define KPDHCLIP_OUTTOP	(1<<KPDHCLIP_TOP)
#define KPDHCLIP_OUTBOT	(1<<KPDHCLIP_BOTTOM)


/* Clipping rectangle */
typedef struct	KPRect	{	int xMin, xMax, yMin, yMax;	}	KPRect;


/* This generates the trivial clip codes inCode and outCode */
void	KPDHTrivialClipPolygon(int nPts, KPDVector3D *p, int *inCode, int *outCode);

void	KPDHClipPolygon(int *nPts, KPDVector3D *pts, KPDVector3D *temp, const KPRect *r);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __KPDHCLIPPOLYGON__ */

