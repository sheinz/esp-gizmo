#include "espressif/esp_common.h"
#include "esp/uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "esp8266.h"

#include <stdio.h>
#include <string.h>

#include "ssid_config.h"

#include "ota-tftp.h"
#include "rboot-api.h"

#include "key_task.h"
#include "load_driver.h"


void main_task(void *pvParameters)
{
    key_event_t key_event;

    while (1) {
        if (xQueueReceive(key_queue, &key_event, portMAX_DELAY)) {
            printf("received event, key=%d, status=%s\n", 
                    key_event.key_index,
                    key_event.on ? "on" : "off");
            load_driver_set(key_event.key_index, key_event.on);
        } else {
            printf("waiting for key event timeout\n");
        }
    }
}

void user_init(void)
{
    uart_set_baud(0, 115200);

    rboot_config conf = rboot_get_config();
    printf("esp-gizmo. Running on flash slot %d / %d\n",
           conf.current_rom, conf.count);

    key_task_init();
    load_driver_init();

    struct sdk_station_config config = {
        .ssid = WIFI_SSID,
        .password = WIFI_PASS,
    };

    /* required to call wifi_set_opmode before station_set_config */
    sdk_wifi_set_opmode(STATION_MODE);
    sdk_wifi_station_set_config(&config);

    printf("Starting TFTP server...");
    ota_tftp_init_server(TFTP_PORT);

    xTaskCreate(main_task, (signed char *)"main", 1024, NULL, 3, NULL);
}