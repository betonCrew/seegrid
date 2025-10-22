#include "utils.h"

void getUCID(uint32_t* ucid) {
    const uint32_t* ucidPtr = (uint32_t*)0x10000FF0;
    for (int i = 0; i < 4; i++) {
        *(ucid+i) = *(ucidPtr+i);
    }
}