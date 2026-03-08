#include <linux/workqueue.h>

#include "device.h"

static void esp32_irq_sync_work(struct work_struct *work)
{
    struct my_device *dev =
        container_of(work, struct my_device, irq_sync_work);
    struct i2c_client *client = dev->client;
    uint8_t buf;
    int ret;

    mutex_lock(&dev->i2c_lock);
    ret = i2c_master_recv(client, &buf, 1);
    mutex_unlock(&dev->i2c_lock);

    if (ret != 1) {
        pr_err("I2C Sensor: failed to receive IRQ sync response\n");
        atomic_set(&dev->p_state, P_STATE_ACTIVE);
        return;
    }

    buf = DEV_SLEEP;

    mutex_lock(&dev->i2c_lock);
    ret = i2c_master_send(client, &buf, 1);
    mutex_unlock(&dev->i2c_lock);

    if (ret != 1) {
        pr_err("I2C Sensor: failed to send sleep commit\n");
        atomic_set(&dev->p_state, P_STATE_ACTIVE);
        return;
    }

    pr_info("I2C Sensor: Sleep command sent successful \n");
}

int work_init(struct my_device *dev)
{
	INIT_WORK(&dev->irq_sync_work, esp32_irq_sync_work);
	
	return 0;
}

