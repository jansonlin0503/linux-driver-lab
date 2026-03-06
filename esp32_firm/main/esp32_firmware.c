#include "esp32_proto.h"

#include <stdio.h>
#include "driver/i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static p_state_t device_state = P_STATE_ACTIVE;
static uint8_t state_buf[2] = {DEV_POWER_MODE_MSG, P_STATE_ACTIVE};

#define TAG "I2C_SLAVE"

#define I2C_SLAVE_NUM      I2C_NUM_0

#define I2C_SLAVE_SDA_IO   6
#define I2C_SLAVE_SCL_IO   7

#define I2C_SLAVE_ADDR     0x28

#define I2C_SLAVE_RX_BUF_LEN 128
#define I2C_SLAVE_TX_BUF_LEN 128

static void i2c_slave_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_SLAVE,
        .sda_io_num = I2C_SLAVE_SDA_IO,
        .scl_io_num = I2C_SLAVE_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,

        .slave = {
            .addr_10bit_en = 0,
            .slave_addr = I2C_SLAVE_ADDR,
        },

        .clk_flags = 0,
    };

    ESP_ERROR_CHECK(i2c_param_config(I2C_SLAVE_NUM, &conf));

    ESP_ERROR_CHECK(
        i2c_driver_install(
            I2C_SLAVE_NUM,
            I2C_MODE_SLAVE,
            I2C_SLAVE_RX_BUF_LEN,
            I2C_SLAVE_TX_BUF_LEN,
            0
        )
    );

    ESP_LOGI(TAG, "I2C Slave ready at address 0x%02X", I2C_SLAVE_ADDR);
}

/* =========================
 * Sleep handler
 * =========================
 */


static int sleep_handler(p_state_t state)
{
    switch (state) {

    case P_STATE_ACTIVE:
        ESP_LOGI(TAG, "Entering ACTIVE mode");
        break;

    case P_STATE_LIGHT_SLEEP:
        ESP_LOGI(TAG, "Entering LIGHT SLEEP");
        /* TODO: call esp_light_sleep_start() */
        break;

    case P_STATE_DEEP_SLEEP:
        ESP_LOGI(TAG, "Entering DEEP SLEEP");
        /* TODO: call esp_deep_sleep_start() */
        break;

    default:
        ESP_LOGW(TAG, "Unknown power state");
        return -1;
    }

    device_state = state;
    return 0;
}


/* =========================
 * I2C command handler
 * =========================
 */

static int i2c_handler(uint8_t *rx_buf, int len)
{
    uint8_t cmd = rx_buf[0];

    switch (cmd) {

    case DEV_GET_POWER_MODE: {

	int written;
        /* prepare response */
	state_buf[1] = device_state;
        written = i2c_slave_write_buffer(
            I2C_SLAVE_NUM,
            state_buf,
            2,
	    portMAX_DELAY
        );

	if(written != 2)
		ESP_LOGI(TAG, "I2C: Power state sent failed!");

        ESP_LOGI(TAG, "I2C: Power state %d sent", state_buf[1]);

        break;
    }


    case DEV_SET_POWER_MODE: {

        ESP_LOGI(TAG, "CMD: SET_POWER_MODE");

        if (len < 2)
            return -1;

        p_state_t state = rx_buf[1];

        sleep_handler(state);

        break;
    }


    default:

        ESP_LOGW(TAG, "Unknown CMD: 0x%02x", cmd);
        break;
    }

    return 0;
}

void app_main(void)
{
    uint8_t rx_buf[8];
    i2c_slave_init();
    ESP_LOGI(TAG, "ESP32 sensor started");

    while (1) {

        int len = i2c_slave_read_buffer(
            I2C_SLAVE_NUM,
            rx_buf,
            sizeof(rx_buf),
	    pdMS_TO_TICKS(100)
        );

        if (len > 0) {

            ESP_LOGI(TAG, "I2C RX len=%d", len);

            i2c_handler(rx_buf, len);
        }
    }
}
