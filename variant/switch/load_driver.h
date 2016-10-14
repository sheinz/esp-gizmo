#ifndef __LOAD_DIRVER_H__
#define __LOAD_DIRVER_H__

#include <stdbool.h>
#include <stdint.h>

#define NUMBER_OF_LOADS   3

void load_driver_init();

/**
 * Turn load on/off
 */
void load_driver_set(uint8_t load_index, bool on);

#endif /* end of include guard: __LOAD_DIRVER_H__ */
