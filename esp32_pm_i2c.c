#include <linux/i2c.h>

#include "device.h"
#include "esp32_proto.h"

int prepare_sleep (struct i2c_client *client)
{
	struct my_device *dev = i2c_get_clientdata(client);
	uint8_t buf;
       	int ret;

	buf = DEV_PREPARE_SLEEP;

	mutex_lock(&dev->i2c_lock);
	ret = i2c_master_send(client, &buf, sizeof(buf));
	if(ret < 0){
		pr_err("I2C Sensor: Preparing sleep fail !\n");
	}
	mutex_unlock(&dev->i2c_lock);

	return 0;
}

int power_state_get(struct i2c_client *client)
{
    struct my_device *dev;
    struct i2c_msg msg[2];
    uint8_t cmd = DEV_GET_POWER_MODE;
    uint8_t state[2];
    int ret;

    if (!client)
        return -ENODEV;

    dev = i2c_get_clientdata(client);
    if (!dev)
        return -ENODEV;

    mutex_lock(&dev->i2c_lock);

    /* message 0: write command */
    msg[0].addr  = client->addr;
    msg[0].flags = 0;
    msg[0].len   = 1;
    msg[0].buf   = &cmd;

    /* message 1: read response */
    msg[1].addr  = client->addr;
    msg[1].flags = I2C_M_RD;
    msg[1].len   = 2;
    msg[1].buf   = state;

    ret = i2c_transfer(client->adapter, msg, 2);
    if (ret != 2) {
        pr_err("I2C Sensor: transfer failed (%d)\n", ret);
        ret = -EIO;
        goto out;
    }

    if (state[0] != DEV_POWER_MODE_MSG) {
        pr_err("I2C Sensor: invalid message header (0x%02x)\n", state[0]);
        ret = -EIO;
        goto out;
    }

    ret = state[1];

out:
    mutex_unlock(&dev->i2c_lock);
    return ret;
}
