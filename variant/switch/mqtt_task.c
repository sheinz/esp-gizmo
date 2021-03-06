#include "mqtt_task.h"

#include <string.h>

#include "espressif/esp_common.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "paho_mqtt_c/MQTTESP8266.h"
#include "paho_mqtt_c/MQTTClient.h"

#define MQTT_HOST "192.168.0.111"
#define MQTT_PORT 1883
#define MQTT_BUF_SIZE   100

static struct mqtt_network network;
static mqtt_client_t client = mqtt_client_default;
static uint8_t mqtt_buf[MQTT_BUF_SIZE];
static uint8_t mqtt_read_buf[MQTT_BUF_SIZE];
static bool connected = false;

/**
 * Topic format: /<device type>/<location>/[lamp/switch]/<index>
 */
const static char topic_lamps[] = "/esp-gizmo-switch/kitchen/lamp/+";
const static char topic_switches[] = "/esp-gizmo-switch/kitchen/switch/";

#define SEND_QUEUE_SIZE     4
#define RECEIVE_QUEUE_SIZE  4

static QueueHandle_t send_queue;
static QueueHandle_t receive_queue;

static void topic_received(mqtt_message_data_t *md)
{
    printf("mqtt: topic received\n");

    mqtt_event_t ev;
    char *data = (char*)md->message->payload;
    char *topic = md->topic->lenstring.data;
    int topic_len = md->topic->lenstring.len;

    if (md->message->payloadlen != 1) {
        printf("mqtt: Invalid payload len\n");
        return;
    }
    if (topic_len != strlen(topic_lamps)) {
        printf("mqtt: Invalid topic len\n");
        return;
    }

    ev.param = topic[topic_len - 1] - '0';
    if (data[0] == '0') {
        ev.cmd = MQTT_CMD_TURN_OFF;
    } else if (data[0] == '1') {
        ev.cmd = MQTT_CMD_TURN_ON;
    } else if (data[0] == '?') {
        ev.cmd = MQTT_CMD_REQ_STATUS;
    } else {
        printf("mqtt: Invalid argument\n");
    }

    if (xQueueSend(receive_queue, &ev, 0) == pdFALSE) {
        printf("mqtt: Receive queue full\n");
    } else {
        taskYIELD();
    }
}

static bool send_status(const mqtt_status_t *status)
{
    char data[1] = "\0";
    char topic_buf[64];
    mqtt_message_t msg;

    data[0] = status->state ? '1' : '0';
    msg.payload = data;
    msg.payloadlen = 1;
    msg.dup = 0;
    msg.qos = MQTT_QOS1;
    msg.retained = 0;

    if (status->kind == STATUS_SWITCH) {
        sprintf(topic_buf, "%s%d", topic_switches, status->index);
    } else if (status->kind == STATUS_LOAD) {
        strcpy(topic_buf, topic_lamps);
        int len = strlen(topic_buf);
        topic_buf[len-1] = '0' + status->index;
    }
    printf("mqtt: publishing topic %s = %d", topic_buf, status->state);

    return mqtt_publish(&client, topic_buf, &msg) == MQTT_SUCCESS;
}

static inline bool init_mqtt_connection()
{
    int ret = 0;
    char client_id[] = "esp-gizmo-switch_2";

    mqtt_network_new(&network);
    ret = mqtt_network_connect(&network, MQTT_HOST, MQTT_PORT);
    if (ret) {
        printf("mqtt: Connection error %d\n", ret);
        return false;
    }

    mqtt_client_new(&client, &network, 5000, mqtt_buf, MQTT_BUF_SIZE,
            mqtt_read_buf, MQTT_BUF_SIZE);

    mqtt_packet_connect_data_t data = mqtt_packet_connect_data_initializer;

    data.willFlag       = 0;
    data.MQTTVersion    = 3;
    data.clientID.cstring   = client_id;
    data.username.cstring   = NULL;
    data.password.cstring   = NULL;
    data.keepAliveInterval  = 10;
    data.cleansession   = 0;

    ret = mqtt_connect(&client, &data);
    if (ret) {
        printf("mqtt: Sending MQTT connect error %d\n", ret);
        return false;
    }

    return true;
}

static void mqtt_task(void *pvParams)
{
    while (true) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
        connected = false;
        xQueueReset(send_queue);
        xQueueReset(receive_queue);
        if (sdk_wifi_station_get_connect_status() != STATION_GOT_IP) {
            printf("mqtt: Waiting for WiFi connection\n");
            vTaskDelay(1000/portTICK_PERIOD_MS);
            continue;
        }
        if (!init_mqtt_connection()) {
            continue;
        }
        printf("mqtt: Connection established\n");
        if (mqtt_subscribe(&client, topic_lamps, MQTT_QOS1, topic_received)) {
            printf("mqtt: Subsciption failed\n");
            continue;
        }
        connected = true;

        while (true) {
            mqtt_status_t status;
            while (xQueueReceive(send_queue, &status, 0) == pdTRUE) {
                if (!send_status(&status)) {
                    printf("mqtt: Send status error\n");
                    break;
                }
            }
            if (mqtt_yield(&client, 1) == MQTT_DISCONNECTED) {
                break;
            }
            taskYIELD();
        }
        printf("mqtt: Connection dropped\n");
        mqtt_network_disconnect(&network);
    }
}

void mqtt_task_init()
{
    send_queue = xQueueCreate(SEND_QUEUE_SIZE, sizeof(mqtt_status_t));
    receive_queue = xQueueCreate(RECEIVE_QUEUE_SIZE, sizeof(mqtt_event_t));

    xTaskCreate(mqtt_task, "mqtt_task", 512, NULL, 1, NULL);
}

bool mqtt_task_get_event(mqtt_event_t *ev)
{
    return xQueueReceive(receive_queue, ev, 0) == pdTRUE;
}

bool mqtt_task_send_status(const mqtt_status_t *status)
{
    if (!connected) {
        return false;
    }

    if (xQueueSend(send_queue, status, 0) != pdTRUE) {
        printf("mqtt: Send queue is full\n");
        return false;
    }

    return true;
}

