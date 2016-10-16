#ifndef __MQTT_TASK_H__
#define __MQTT_TASK_H__

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    MQTT_CMD_TURN_OFF = 0,
    MQTT_CMD_TURN_ON,
    MQTT_CMD_REQ_STATUS
} mqtt_cmd_enum_t;

typedef struct {
    mqtt_cmd_enum_t cmd; 
    uint32_t param;
} mqtt_event_t;

#define NUMBER_OF_LOADS     3
#define NUMBER_OF_SWITCHES  3

typedef struct {
    bool loads[NUMBER_OF_LOADS];
    bool switches[NUMBER_OF_LOADS];
} mqtt_status_t;

void mqtt_task_init();

bool mqtt_task_get_event(mqtt_event_t *ev);
bool mqtt_task_send_status(const mqtt_status_t *status);
bool mqtt_task_is_connected();

#endif /* end of include guard: __MQTT_TASK_H__ */
