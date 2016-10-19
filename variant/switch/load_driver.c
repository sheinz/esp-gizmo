
#include "load_driver.h"
#include "esp/gpio.h"

static const uint8_t load_pins[NUMBER_OF_LOADS] = {5, 4, 16};
static bool load_states[NUMBER_OF_LOADS];

void load_driver_init()
{
    for (int i = 0; i < NUMBER_OF_LOADS; i++) {
        gpio_enable(load_pins[i], GPIO_OUTPUT);
        gpio_write(load_pins[i], true); // active low
        load_states[i] = false;
    }
}

bool load_driver_set(uint8_t load_index, bool on)
{
    if (load_states[load_index] != on) {
        // invert value as load on when pin is low
        gpio_write(load_pins[load_index], !on);
        load_states[load_index] = on;
        return true;
    } else {
        return false;
    }
}
