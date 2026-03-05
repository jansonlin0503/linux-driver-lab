#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/mutex.h>

#include "device.h"

/* Sensor registration steps */

/* Probe and remove functions */
static int dev_probe(struct i2c_client *client)
{
    struct my_device *my_dev;

    /* Device dynamic allocation */
    my_dev = kzalloc(sizeof(*my_dev), GFP_KERNEL);
    if (!my_dev)
        return -ENOMEM;

    /* Binding I2C client */
    my_dev->client = client;
    i2c_set_clientdata(client, my_dev);

    /* Device resource initialization */
    mutex_init(&my_dev->i2c_lock);

    pr_info("I2C Sensor: Sensor registered \n");
    return 0;
}

static void dev_remove(struct i2c_client *client)
{
    struct my_dev *dev = i2c_get_clientdata(client);

    kfree(dev);
    pr_info("I2C Sensor: Sensor removed \n");
}

static const struct i2c_device_id my_device_id[] = {
    {"esp32-sensor", 0},
    {},
};
MODULE_DEVICE_TABLE(i2c, my_device_id);

static struct i2c_driver my_device_driver = {
    .driver =
        {
            .name = "esp32-sensor",
        },
    .probe = dev_probe,
    .remove = dev_remove,
    .id_table = my_device_id,
};

module_i2c_driver(my_device_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Janson Lin");
MODULE_DESCRIPTION("PM Test driver");
