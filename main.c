#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/pm_runtime.h>

#include "device.h"

static struct class *class;
/* Sensor registration steps */

/* Probe and remove functions */
static int dev_probe(struct i2c_client *client)
{
    struct my_device *my_dev;
    int ret;

    my_dev = kzalloc(sizeof(*my_dev), GFP_KERNEL);
    if (!my_dev)
        return -ENOMEM;

    my_dev->client = client;
    i2c_set_clientdata(client, my_dev);

    mutex_init(&my_dev->i2c_lock);

    /* Allocate char device number */
    ret = alloc_chrdev_region(&my_dev->devt, 0, 1, DEVICE_NAME);
    if (ret) {
        pr_err("I2C Sensor: char device registration failed\n");
        goto err_alloc_chrdev;
    }

    /* Initialize cdev */
    cdev_init(&my_dev->cdev, &my_fops);

    ret = cdev_add(&my_dev->cdev, my_dev->devt, 1);
    if (ret) {
        pr_err("I2C Sensor: cdev add failed\n");
        goto err_cdev_add;
    }

    /* Create device class */
    class = class_create(DEVICE_NAME);
    if (IS_ERR(class)) {
        ret = PTR_ERR(class);
        goto err_class_create;
    }

    /* Create /dev node */
    my_dev->device = device_create(class, NULL,
                                   my_dev->devt, NULL,
                                   DEVICE_NAME);
    if (IS_ERR(my_dev->device)) {
        ret = PTR_ERR(my_dev->device);
        goto err_device_create;
    }

    /* Initialize FIFO */
    ret = kfifo_alloc(&my_dev->fifo, FIFO_SIZE, GFP_KERNEL);
    if (ret)
        goto err_fifo;

    ret = irq_init(my_dev);
    if(ret){
	    goto err_irq;
    }

    mutex_init(&my_dev->fifo_lock);
    init_waitqueue_head(&my_dev->wq);
    my_dev->open_count = 0;

    atomic_set(&my_dev->p_state, P_STATE_ACTIVE);

    /* Set up runtime pm */
    pm_runtime_enable(&client->dev);
    pm_runtime_set_autosuspend_delay(&client->dev, 3000);
    pm_runtime_use_autosuspend(&client->dev);

    pr_info("I2C Sensor: Sensor registered\n");
    return 0;

err_irq:
err_fifo:
    device_destroy(class, my_dev->devt);

err_device_create:
    class_destroy(class);

err_class_create:
    cdev_del(&my_dev->cdev);

err_cdev_add:
    unregister_chrdev_region(my_dev->devt, 1);

err_alloc_chrdev:
    kfree(my_dev);

    return ret;
}

static void dev_remove(struct i2c_client *client)
{
    struct my_device *dev = i2c_get_clientdata(client);

    /* Disable runtime PM */
    pm_runtime_disable(&client->dev);

    kfifo_free(&dev->fifo);

    device_destroy(class, dev->devt);
    class_destroy(class);

    cdev_del(&dev->cdev);
    unregister_chrdev_region(dev->devt, 1);

    kfree(dev);

    pr_info("I2C Sensor: Sensor removed\n");
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
	    .pm = &my_pm_ops,
        },
    .probe = dev_probe,
    .remove = dev_remove,
    .id_table = my_device_id,
};

module_i2c_driver(my_device_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Janson Lin");
MODULE_DESCRIPTION("PM Test driver");
