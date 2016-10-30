
#include "FreeRTOS.h"
#include "task.h"
#include "esp/gpio.h"

#include "key_task.h"

#define KEY_QUEUE_SIZE      8
#define KEY_SCAN_PERIOD     100  // scan period in ms
#define NUMBER_OF_KEYS      3

xQueueHandle key_queue;

typedef enum {
    KEY_STATE_UNKNWON = 0,
    KEY_STATE_ON,
    KEY_STATE_OFF
} key_state_enum_t;

static key_state_enum_t key_states[NUMBER_OF_KEYS];
static const uint8_t key_pins[NUMBER_OF_KEYS] = {13, 12, 14};

static inline void send_key_event(uint8_t key, bool on)
{
    key_event_t event;
    event.key_index = key;    
    event.on = on;
    if (xQueueSend(key_queue, &event, 0) != pdTRUE) {
        //TODO: handle send fail
    }
}

static void scan_key_task(void *pvParameters)
{
    while (1) {
        for (int i = 0; i < NUMBER_OF_KEYS; i++) {
            bool value = !gpio_read(key_pins[i]);    // invert value as it is active when low
            if (key_states[i] == KEY_STATE_UNKNWON) {
                send_key_event(i, value);
            } else if (key_states[i] == KEY_STATE_ON && !value) {
                // key has just turned off
                send_key_event(i, value);   
            } else if (key_states[i] == KEY_STATE_OFF && value) {
                // key has just turned on
                send_key_event(i, value);
            }
            key_states[i] = value ? KEY_STATE_ON : KEY_STATE_OFF;
        }
        
        vTaskDelay(KEY_SCAN_PERIOD / portTICK_RATE_MS);
    }
}

void key_task_init()
{
    for (int i = 0; i < NUMBER_OF_KEYS; i++) {
        key_states[i] = KEY_STATE_UNKNWON;
        gpio_enable(key_pins[i], GPIO_INPUT);
        gpio_set_pullup(key_pins[i], true, /*enabled_during_sleep=*/false);
    }

    key_queue = xQueueCreate(KEY_QUEUE_SIZE, sizeof(key_event_t));

    xTaskCreate(scan_key_task, "scan_key_task", 256, NULL, 1, NULL);
}
