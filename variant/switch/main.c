#include "espressif/esp_common.h"
#include "esp/uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "esp8266.h"

#include <stdio.h>
#include <string.h>

#include "ssid_config.h"

#include "key_task.h"


void main_task(void *pvParameters)
{
    key_event_t key_event;

    while (1) {
        if (xQueueReceive(key_queue, &key_event, portMAX_DELAY)) {
            printf("received event, key=%d, status=%s\n", 
                    key_event.key,
                    key_event.on ? "on" : "off");
        } else {
            printf("waiting for key event timeout\n");
        }
    }
}

void user_init(void)
{
    uart_set_baud(0, 115200);

    key_task_init();

    struct sdk_station_config config = {
        .ssid = WIFI_SSID,
        .password = WIFI_PASS,
    };

    /* required to call wifi_set_opmode before station_set_config */
    sdk_wifi_set_opmode(STATION_MODE);
    sdk_wifi_station_set_config(&config);

    xTaskCreate(main_task, (signed char *)"main", 1024, NULL, 2, NULL);
}
