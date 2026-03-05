#include "device.h"

static int my_runtime_suspend(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct my_device *mdev = i2c_get_clientdata(client);

    pr_info("sensor runtime suspend\n");

    /* tell ESP32 to sleep */
    return 0;
}

static int my_runtime_resume(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct my_device *mdev = i2c_get_clientdata(client);

    pr_info("sensor runtime resume\n");

    /* wake ESP32 */
    return 0;
}

const struct dev_pm_ops my_pm_ops = {
	.runtime_suspend = my_runtime_suspend,
	.runtime_resume = my_runtime_resume,
};
