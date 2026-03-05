#ifndef DEVICE_H
#define DEVICE_H

#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/kfifo.h>
#include <linux/cdev.h>
#include <linux/wait.h>

#include "device.h"
#include "esp32_proto.h"

#define DEVICE_NAME "tao"
#define FIFO_SIZE 128

struct my_device {
        /* I2C device */
        struct i2c_client *client;
        struct mutex i2c_lock;

        /* cached power state */
        p_state_t p_state;

        /* char device interface */
        struct cdev cdev;
        dev_t devt;
        struct device *device;

        /* data buffer */
        struct kfifo fifo;
        struct mutex fifo_lock;
        wait_queue_head_t wq;

        /* device usage */
        int open_count;
};

extern const struct file_operations my_fops;
int power_state_get(struct i2c_client *client);

/* PM callback */
extern const struct dev_pm_ops my_pm_ops;

#endif
