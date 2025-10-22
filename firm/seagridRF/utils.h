#ifndef UTILS_H
#define UTILS_H

#include "stdint.h"

/**
 * reads 16bytes from 0x10000FF0 to given Pointer
 */
void utils_getUCID(uint32_t* ucid);

#endif