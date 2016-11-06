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
#include "lwip/inet.h"

#include <stdio.h>
#include <string.h>

#include "bmp280/bmp280.h"
#include "bmp180/bmp180.h"
#include "i2c/i2c.h"
#include "dht.h"

#include "ssid_config.h"

const uint8_t scl_pin = 5;
const uint8_t sda_pin = 4;
uint8_t const dht_pin = 12;

static bmp280_t bmp280_dev;
static bmp180_constants_t bmp180_constants;

#define API_KEY "SQR1QEVFNW0SHPPU"
#define THINGSPEAK_HOST     "api.thingspeak.com"
#define THINGSPEAK_PORT     80
#define THINGSPEAK_GET_URL  "http://" THINGSPEAK_HOST "/update?api_key="\
    API_KEY "&field1=%f&field2=%f&field3=%d&field4=%f&field5=%f"

#define BUFFER_SIZE 512

static void send_data(float pressure1, float temperature1, int humidity,
        float pressure2, float temperature2)
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
    
    sprintf(buf, req, pressure1, temperature1, humidity,
            pressure2, temperature2);
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

static bool init_sensors()
{
    bmp280_params_t  params;

    // dht22 initialization
    gpio_set_pullup(dht_pin, false, false);

    bmp280_init_default_params(&params);
    bmp280_dev.i2c_addr = BMP280_I2C_ADDRESS_0;
    bmp280_dev.id = BMP280_CHIP_ID;

    params.mode = BMP280_MODE_FORCED;

    i2c_init(scl_pin, sda_pin);

    if (!bmp280_init(&bmp280_dev, &params)) {
        printf("bmp280 initialization failed\n");
        return false;
    }

    if (!bmp180_init(scl_pin, sda_pin)) {
        printf("bmp180 initialization failed\n");
        return false;
    }
    if (!bmp180_is_available()) {
        printf("bmp180 not found\n");
        return false;
    }
    if (!bmp180_fillInternalConstants(&bmp180_constants)) {
        printf("failed reading calibration constants\n");
        return false;
    }

    return true;
}

void main_task(void *pvParameters)
{
    float bmp280_pressure, bmp280_temperature, dummy;
    uint32_t bmp180_pressure;
    int32_t bmp180_temperature;
    int16_t dht_temp, humidity;

    while (1) {
        while (!init_sensors()) {
            printf("Sensors initialization failed\n");
            vTaskDelay(1000 / portTICK_RATE_MS);
        }

        vTaskDelay(1000 / portTICK_RATE_MS);
        while(1) {
            if (!bmp280_force_measurement(&bmp280_dev)) {
                printf("Failed initiating measurement\n");
                break;
            }
            while (bmp280_is_measuring(&bmp280_dev)) {}; // wait for measurement to complete

            if (!bmp280_read_float(&bmp280_dev, &bmp280_temperature, 
                        &bmp280_pressure, &dummy)) {
                printf("bmp280: Temperature/pressure reading failed\n");
                break;
            }
            printf("bmp280: Pressure: %.2f Pa, Temperature: %.2f C\n", 
                    bmp280_pressure, bmp280_temperature);

            if (!dht_read_data(dht_pin, &humidity, &dht_temp)) {
                printf("Humidity reading failed\n");
                break;
            }
            printf("Humidity: %d%% Temp: %dC\n", humidity / 10, dht_temp / 10);

            if (!bmp180_measure(&bmp180_constants, &bmp180_temperature,
                        &bmp180_pressure, 3)) {
                printf("bmp180: Temperature/pressure reading failed\n");
                break;
            }
            printf("bmp180: Pressure: %d Pa, Temperature: %.2f C\n", 
                    bmp180_pressure, (float)bmp180_temperature/10);

            send_data(bmp280_pressure, bmp280_temperature, humidity/10,
                    bmp180_pressure, (float)bmp180_temperature/10);

            /* sdk_system_deep_sleep(5 * 60 * 1000000); */
            vTaskDelay(5 * 60 * 1000 / portTICK_RATE_MS);
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
