#ifndef TCAN_H
#define TCAN_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

typedef struct
{
	uint16_t id;
	uint16_t ide;
	struct {
		int8_t rtr : 1;
		int8_t ide : 1;
		uint8_t length : 4;
	} header;
	uint8_t data[8];
} tCAN;

#ifdef __cplusplus
}
#endif

#endif // TCAN_H