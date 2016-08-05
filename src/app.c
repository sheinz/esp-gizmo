#include "espressif/esp_common.h"
#include "esp/uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "esp8266.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include <stdio.h>
#include <string.h>

#include "bmp280/bmp280.h"
#include "dht.h"

#include "ssid_config.h"

const uint8_t scl_pin = 5;
const uint8_t sda_pin = 4;
uint8_t const dht_pin = 12;

#define API_KEY ""
#define THINGSPEAK_HOST     "api.thingspeak.com"
#define THINGSPEAK_PORT     80
#define THINGSPEAK_GET_URL  "http://" THINGSPEAK_HOST "/update?api_key="\
    API_KEY "&field1=%f&field2=%f&field3=%d"

#define BUFFER_SIZE 256

static void send_data(float pressure, float temperature, int humidity)
{
    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *res;
    int err = getaddrinfo(THINGSPEAK_HOST, "80", &hints, &res);

    if(err != 0 || res == NULL) {
        printf("DNS lookup failed err=%d res=%p\n", err, res);
        if(res)
            freeaddrinfo(res);
        return;
    }

    struct in_addr *addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
    printf("DNS lookup succeeded. IP=%s\n", inet_ntoa(*addr));

    int s = socket(res->ai_family, res->ai_socktype, 0);
    if(s < 0) {
        printf("Failed to allocate socket.\n");
        freeaddrinfo(res);
        return;
    }

    if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
        close(s);
        freeaddrinfo(res);
        printf("Socket connect failed.\n");
        return;
    }

    printf("connected\n");
    freeaddrinfo(res);

    static char buf[BUFFER_SIZE];

    const char *req =
        "GET " THINGSPEAK_GET_URL "\r\n"
        "User-Agent: esp-open-rtos/0.1 esp8266\r\n"
        "\r\n";
    
    sprintf(buf, req, pressure, temperature, humidity);
    printf("request: \n%s", buf);

    if (write(s, buf, strlen(buf)) < 0) {
        printf("... socket send failed\n");
        close(s);
        return;
    }

    printf("Socket send success\n");

    int r;
    do {
        bzero(buf, BUFFER_SIZE);
        r = read(s, buf, BUFFER_SIZE - 1);
        if(r > 0) {
            printf("%s", buf);
        }
    } while(r > 0);

    printf("done reading from socket. Last read return=%d errno=%d\n", r, errno);
    close(s);
}

void main_task(void *pvParameters)
{
    bmp280_params_t  params;
    float pressure, temperature;
    int16_t dht_temp, humidity;

    bmp280_init_default_params(&params);
    params.mode = BMP280_MODE_FORCED;

    gpio_set_pullup(dht_pin, false, false);

    while (1) {
        while (!bmp280_init(&params, scl_pin, sda_pin)) {
            printf("BMP280 initialization failed\n");
            vTaskDelay(1000 / portTICK_RATE_MS);
        }

        while(1) {
            vTaskDelay(10000 / portTICK_RATE_MS);
            if (!bmp280_force_measurement()) {
                printf("Failed initiating measurement\n");
                break;
            }
            while (bmp280_is_measuring()) {}; // wait for measurement to complete

            if (!bmp280_read(&temperature, &pressure)) {
                printf("Temperature/pressure reading failed\n");
                break;
            }
            printf("Pressure: %.2f Pa, Temperature: %.2f C\n", pressure, temperature);

            if (!dht_read_data(dht_pin, &humidity, &dht_temp)) {
                printf("Humidity reading failed\n");
            }
            printf("Humidity: %d%% Temp: %dC\n", humidity / 10, dht_temp / 10);

            send_data(pressure, temperature, humidity/10);
        }
    }
}

void user_init(void)
{
    uart_set_baud(0, 115200);

    struct sdk_station_config config = {
        .ssid = WIFI_SSID,
        .password = WIFI_PASS,
    };

    /* required to call wifi_set_opmode before station_set_config */
    sdk_wifi_set_opmode(STATION_MODE);
    sdk_wifi_station_set_config(&config);

    xTaskCreate(main_task, (signed char *)"main", 1024, NULL, 2, NULL);
}
