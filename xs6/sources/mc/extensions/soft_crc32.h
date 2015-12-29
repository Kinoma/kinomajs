#include <stdint.h>

extern int mc_soft_crc32_init();
extern uint32_t mc_soft_crc32(const void *data, int data_size, uint32_t crc);
extern void mc_soft_crc32_fin();
