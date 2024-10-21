#include "delay.h"


void Delay_ms(uint32_t ms) {
    for(uint32_t i = 0; i < ms * 4000; i++) {
        __NOP();
    }
}