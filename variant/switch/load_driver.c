
#include "load_driver.h"
#include "esp/gpio.h"

static const uint8_t load_pins[NUMBER_OF_LOADS] = {5, 4, 16};

void load_driver_init()
{
    for (int i = 0; i < NUMBER_OF_LOADS; i++) {
        gpio_enable(load_pins[i], GPIO_OUTPUT);
        gpio_write(load_pins[i], true); // active low
    }
}

void load_driver_set(uint8_t load_index, bool on)
{
    // invert value as load on when pin is low
    gpio_write(load_pins[load_index], !on);
}
