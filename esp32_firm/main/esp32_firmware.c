#include "esp32_proto.h"
#include "esp_sleep.h"
#include <stdio.h>
#include "driver/i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"

static p_state_t device_state = P_STATE_ACTIVE;
static uint8_t state_buf[2] = {DEV_POWER_MODE_MSG, P_STATE_ACTIVE};

#define TAG "I2C_SLAVE"

#define I2C_SLAVE_NUM      I2C_NUM_0

#define I2C_SLAVE_SDA_IO   6
#define I2C_SLAVE_SCL_IO   7

#define I2C_SLAVE_ADDR     0x28

#define I2C_SLAVE_RX_BUF_LEN 128
#define I2C_SLAVE_TX_BUF_LEN 128

typedef struct {
    uint32_t light_sleep_us;
    uint32_t deep_sleep_us;
} pm_policy_t;

static pm_policy_t pm_policy = {
    .light_sleep_us = 2000000,
    .deep_sleep_us = 10000000
};

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

void irq_gpio_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << ESP32_GPIO_OUT),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(&io_conf);

    gpio_set_level(ESP32_GPIO_OUT, 0);   // default low
}

#define WAKE_LEVEL 1
static esp_err_t esp32_register_gpio_wakeup(void)
{
    gpio_config_t config = {
        .pin_bit_mask = 1ULL << ESP32_GPIO_IN,
        .mode = GPIO_MODE_INPUT,
        .pull_down_en = false,
        .pull_up_en = false,
        .intr_type = GPIO_INTR_DISABLE
    };

    ESP_ERROR_CHECK(gpio_config(&config));

    ESP_ERROR_CHECK(
        gpio_wakeup_enable(
            ESP32_GPIO_IN,
            WAKE_LEVEL ? GPIO_INTR_HIGH_LEVEL : GPIO_INTR_LOW_LEVEL
        )
    );

    ESP_ERROR_CHECK(esp_sleep_enable_gpio_wakeup());

    ESP_LOGI("PM", "GPIO wakeup source registered");

    return ESP_OK;
}

/* =========================
 * I2C command handler
 * =========================
 */

static int resume_process(void)
{
	uint8_t state;
	device_state = P_STATE_ACTIVE;
	state = device_state;

	i2c_reset_tx_fifo(I2C_SLAVE_NUM);

	i2c_slave_write_buffer(
		I2C_SLAVE_NUM,
		&state,
		1,
		portMAX_DELAY
	);

	gpio_set_level(ESP32_GPIO_OUT, 1);
	esp_rom_delay_us(50);
	gpio_set_level(ESP32_GPIO_OUT,0);

	return 0;
}

static int i2c_handler(uint8_t *rx_buf, int len)
{
    uint8_t cmd = rx_buf[0];

    switch (cmd) {

    case DEV_WAKE: {

        ESP_LOGI(TAG, "CMD: WAKE");

        break;
    }


    case DEV_SLEEP: {

        ESP_LOGI(TAG, "CMD: SLEEP");
	uart_wait_tx_idle_polling(CONFIG_ESP_CONSOLE_UART_NUM);
	
	/* Idle timer for deep sleep mode */
	esp_sleep_enable_timer_wakeup(pm_policy.deep_sleep_us);

	esp_light_sleep_start();

	if(esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER){
		ESP_LOGI(TAG, "Enter deep sleep");
		return 0;
	}

	ESP_LOGI(TAG, "Waked up by host GPIO");
	resume_process();
	
	break;


    }
    case DEV_PREPARE_SLEEP: {
			     
	uint8_t state;

	ESP_LOGI(TAG, "CMD: Prepare to sleep");
	device_state = P_STATE_PREPARE_SLEEP;
	state = device_state;

	/* Reset Tx buffer */
	i2c_reset_tx_fifo(I2C_SLAVE_NUM);

	/* Write prepare state into buffer */
	i2c_slave_write_buffer(
		I2C_SLAVE_NUM,
		&state,
		1,
		portMAX_DELAY
	);

	/* Pull up GPIO and wait */
	gpio_set_level(ESP32_GPIO_OUT, 1);
	esp_rom_delay_us(50);
	gpio_set_level(ESP32_GPIO_OUT,0);
	ESP_LOGI(TAG, "GPIO out pull up");

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
    irq_gpio_init();
    gpio_wakeup_enable(ESP32_GPIO_IN, GPIO_INTR_HIGH_LEVEL);
    esp32_register_gpio_wakeup();

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
