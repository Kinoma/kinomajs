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
#include "kinoma_ipp_common.h"

/*  quick sort */
static __inline int  partition(int data[], int x, int y)
{
    int  n = data[x], i = x + 1, j = y, temp;
    while(1)
    {
        while(data[i] <= n) ++i;
        while(data[j] > n) --j;
        if(i >= j) break;
        temp = data[i]; 
		data[i] = data[j]; 
		data[j]=temp;
    }
    data[x] = data[j];
    data[j] = n;
    return j;
}

void quick_sort(int data[], int x, int y)
{
	int q;
    if(x >= y)
		return;
    q = partition(data, x, y);
    quick_sort(data, x, q-1);
    quick_sort(data, q+1, y);
}


