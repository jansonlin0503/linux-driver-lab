#ifndef DEVICE_H
#define DEVICE_H

#include <linux/i2c.h>
#include <linux/mutex.h>

#include "device.h"
#include "esp32_proto.h"

struct my_device {
	struct i2c_client *client;
	struct mutex i2c_lock;

	p_state_t p_state;
};

#endif
