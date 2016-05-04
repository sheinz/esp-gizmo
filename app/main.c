#include "osapi.h"
#include "user_interface.h"
#include "eagle_soc.h"
#include "user_config.h"
#include "mqtt.h"
#include "mem.h"
#include "gpio.h"

#define SSID         ""
#define PASSWORD     ""


MQTT_Client mqttClient;


static void wifi_handle_event_cb(System_Event_t *event)
{
    os_printf("event %x\n", event->event);

    switch (event->event) {
        case EVENT_STAMODE_GOT_IP:
            os_printf("Got IP\n");
            MQTT_Connect(&mqttClient);
            break;
        case EVENT_STAMODE_DISCONNECTED:
            os_printf("Disconnected from AP\n");
            MQTT_Disconnect(&mqttClient);
            break;
    } 
}

void mqtt_connect_cb(uint32_t *args)
{
    MQTT_Client* client = (MQTT_Client*)args;
    os_printf("MQTT: Connected\r\n");
    MQTT_Subscribe(client, "led-switch", 0);
    /* MQTT_Subscribe(client, "/mqtt/topic/0", 0); */
    /* MQTT_Subscribe(client, "/mqtt/topic/1", 1); */
    /* MQTT_Subscribe(client, "/mqtt/topic/2", 2); */
    /*  */
    /* MQTT_Publish(client, "/mqtt/topic/0", "hello0", 6, 0, 0); */
    /* MQTT_Publish(client, "/mqtt/topic/1", "hello1", 6, 1, 0); */
    /* MQTT_Publish(client, "/mqtt/topic/2", "hello2", 6, 2, 0); */
}

void mqtt_disconnect_cb(uint32_t *args)
{
    MQTT_Client* client = (MQTT_Client*)args;
    os_printf("MQTT: Disconnected\r\n");
}

void mqtt_publish_cb(uint32_t *args)
{
    MQTT_Client* client = (MQTT_Client*)args;
    os_printf("MQTT: Published\r\n");
}

void mqtt_data_cb(uint32_t *args, const char* topic, 
        uint32_t topic_len, const char *data, uint32_t data_len)
{
    char *topicBuf = (char*)os_zalloc(topic_len+1),
            *dataBuf = (char*)os_zalloc(data_len+1);

    MQTT_Client* client = (MQTT_Client*)args;

    os_memcpy(topicBuf, topic, topic_len);
    topicBuf[topic_len] = 0;

    os_memcpy(dataBuf, data, data_len);
    dataBuf[data_len] = 0;
    if (!os_strcmp(dataBuf, "on")) {
        os_printf("Led ON\n");
        gpio_output_set(0, BIT4, 0, 0);   // 4 pin output low
    } else if (!os_strcmp(dataBuf, "off")) {
        os_printf("Led OFF\n");
        gpio_output_set(BIT4, 0, 0, 0);   // 4 pin output high
    }

    os_printf("Receive topic: %s, data: %s \r\n", topicBuf, dataBuf);
    os_free(topicBuf);
    os_free(dataBuf);
}

static void init_wifi()
{
    struct station_config config;

    wifi_set_opmode(STATION_MODE);    // station mode
    
    os_memcpy(config.ssid, SSID, sizeof(SSID));
    os_memcpy(config.password, PASSWORD, sizeof(PASSWORD));
    config.bssid_set = 0;

    wifi_station_set_config_current(&config);

    wifi_set_event_handler_cb(wifi_handle_event_cb);
}

void ICACHE_FLASH_ATTR user_init()
{
    uart_div_modify(0, UART_CLK_FREQ / 115200);
    wifi_status_led_install(2, PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
    os_printf("\nStarted");
    os_delay_us(1000000);

    gpio_init();

    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4); 
    gpio_output_set(BIT4, 0, BIT4, 0);   // 4 pin output high

    MQTT_InitConnection(&mqttClient, "162.243.215.71", 1883, 0);
    MQTT_InitClient(&mqttClient, "esp8266_1", "", "", 120, 1);

    MQTT_OnConnected(&mqttClient, mqtt_connect_cb);
    MQTT_OnDisconnected(&mqttClient, mqtt_disconnect_cb);
    MQTT_OnData(&mqttClient, mqtt_data_cb);
    MQTT_OnPublished(&mqttClient, mqtt_publish_cb);

    init_wifi();

    os_printf("\nSystem started...\n");
}
