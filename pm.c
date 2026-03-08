#include <linux/delay.h>
#include "device.h"


static int my_runtime_suspend(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct my_device *mdev = i2c_get_clientdata(client);

    pr_info("sensor runtime suspend\n");

    prepare_sleep(client);
    atomic_set(&mdev->p_state, P_STATE_PREPARE_SLEEP);

    return 0;
}

static int my_runtime_resume(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev)
    struct my_device *mdev = i2c_get_clientdata(client);

    pr_info("sensor runtime resume\n");

    /* wake ESP32 */
    gpio_set_value(HOST_GPIO_OUT, 1);

    usleep_range(1000, 2000);

    gpio_set_value(HOST_GPIO_OUT, 0);

    return 0;
}

const struct dev_pm_ops my_pm_ops = {
	.runtime_suspend = my_runtime_suspend,
	.runtime_resume = my_runtime_resume,
};
