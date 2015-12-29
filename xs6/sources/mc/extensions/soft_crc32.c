#include "mc_stdio.h"
#include "soft_crc32.h"
#if mxMC
#include <soft_crc.h>
#endif

int mc_soft_crc32_init(void)
{
	return 0;
}

uint32_t mc_soft_crc32(const void *data, int data_size, uint32_t crc)
{
#if mxMC
	return soft_crc32(data, data_size, crc);
#else
	return 0;
#endif
}

void mc_soft_crc32_fin()
{
}
