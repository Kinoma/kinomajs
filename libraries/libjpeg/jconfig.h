#include "Fsk.h"

#if TARGET_OS_WIN32
	#include "jconfig.vc"
#elif TARGET_OS_MAC
	#if TARGET_OS_IPHONE
		#include "jconfig.iphone"
	#else
		#include "jconfig.mac"
	#endif
#elif TARGET_OS_LINUX
	#include "jconfig.lnx"
#elif TARGET_OS_KPL
	#include "jconfig.lnx"
#else
	#pragma error("yikes")
#endif
