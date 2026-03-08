#ifndef ESP32_PROTO_H
#define ESP32_PROTO_H

#define ESP32_GPIO_IN 0
#define ESP32_GPIO_OUT 4

#define HOST_GPIO_IN 538
#define HOST_GPIO_OUT 529

typedef enum power_state{
	P_STATE_ACTIVE = 0,
	P_STATE_PREPARE_SLEEP,
	P_STATE_LIGHT_SLEEP,
	P_STATE_DEEP_SLEEP,
}p_state_t;

/* I2C protocol: cmd(msg) + 1 byte data
 * DEV_GET_POWER_MODE: no further data required 
 * DEV_SET_LIGHT_POWER_MODE: with p_stat_t data
 *
 * DEV_POWER_MODE: power mode message */

enum device_cmd {
	DEV_GET_POWER_MODE = 0x01,
	DEV_SET_POWER_MODE = 0x02,

	DEV_PREPARE_SLEEP = 0x03,
};

enum device_msg {
	DEV_POWER_MODE_MSG = 0x11,
};

#endif

