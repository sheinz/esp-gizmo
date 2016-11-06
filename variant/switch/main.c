#include <espressif/esp_common.h>
#include <esp/uart.h>
#include <FreeRTOS.h>
#include <task.h>
#include <esp8266.h>

#include <stdio.h>
#include <string.h>

#include <ssid_config.h>

#include <ds18b20/ds18b20.h>
#include <ota-tftp.h>
#include <rboot-api.h>

#include "key_task.h"
#include "load_driver.h"
#include "mqtt_task.h"


#define SENSOR_GPIO         0

void health_state(void *pvParameters)
{
    ds18b20_addr_t temp_sensor_addr;

    int sensor_count = ds18b20_scan_devices(SENSOR_GPIO, &temp_sensor_addr, 1);
    if (sensor_count != 1) {
        printf("No sensors found on one wire bus\n");
        vTaskDelete(NULL);
        return;
    }

    while (1) {
        if (!ds18b20_measure(SENSOR_GPIO, temp_sensor_addr, /*wait=*/false)) {
            printf("Measurement error\n");
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        float temp = ds18b20_read_temperature(SENSOR_GPIO, temp_sensor_addr);
        printf("Temperature: %f\n", temp);
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

void main_task(void *pvParameters)
{
    key_event_t key_event;
    mqtt_event_t mqtt_event;
    mqtt_status_t mqtt_status;

    while (1) {
        while (xQueueReceive(key_queue, &key_event, 0)) {
            printf("received event, key=%d, status=%s\n",
                    key_event.key_index,
                    key_event.on ? "on" : "off");

            mqtt_status.index = key_event.key_index;
            mqtt_status.state = key_event.on;
            mqtt_status.kind = STATUS_SWITCH;
            mqtt_task_send_status(&mqtt_status);

            if (load_driver_set(key_event.key_index, key_event.on)) {
                // if the load state has changed send also its new state
                mqtt_status.kind = STATUS_LOAD;
                mqtt_task_send_status(&mqtt_status);
            }
        }
        while (mqtt_task_get_event(&mqtt_event)) {
            printf("receved mqtt event, load=%d, status=%s\n",
                    mqtt_event.param,
                    mqtt_event.cmd == MQTT_CMD_TURN_ON ? "on" : "off");
            if (mqtt_event.cmd == MQTT_CMD_TURN_ON) {
                load_driver_set(mqtt_event.param, true);
            } else if (mqtt_event.cmd == MQTT_CMD_TURN_OFF) {
                load_driver_set(mqtt_event.param, false);
            } else if (mqtt_event.cmd == MQTT_CMD_REQ_STATUS){
                printf("Request status\n");
                // TODO: implement
            }
        }
        taskYIELD();
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
    mqtt_task_init();

    struct sdk_station_config config = {
        .ssid = WIFI_SSID,
        .password = WIFI_PASS,
    };

    /* required to call wifi_set_opmode before station_set_config */
    sdk_wifi_set_opmode(STATION_MODE);
    sdk_wifi_station_set_config(&config);

    printf("Starting TFTP server...");
    ota_tftp_init_server(TFTP_PORT);

    xTaskCreate(main_task, "main", 1024, NULL, 1, NULL);
    xTaskCreate(health_state, "health_state", 512, NULL, 1, NULL);
}
