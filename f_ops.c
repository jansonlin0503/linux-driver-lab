#include <linux/fs.h>

#include "device.h"

static int dev_open(struct inode *inode, struct file *file)
{
	struct my_device *dev = container_of(inode->i_cdev, struct my_device, cdev);

	file->private_data = dev;
	dev->open_count ++;

	return 0;
}

static int dev_release(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t dev_read(struct file *file,
                        char __user *buf,
                        size_t len,
                        loff_t *ppos)
{
        struct my_device *dev = file->private_data;
        uint8_t state = 0x88;
        int ret;

        if (len < 1)
                return -EINVAL;

        /* avoid repeated reads like cat */
        if (*ppos > 0)
                return 0;

	pm_runtime_get_sync(&dev->client->dev);


	pm_runtime_mark_last_busy(&dev->client->dev);
        pm_runtime_put_autosuspend(&dev->client->dev);

        if (ret < 0)
                return ret;

        state = (uint8_t)ret;

        if (copy_to_user(buf, &state, 1))
                return -EFAULT;

        *ppos += 1;

        return 1;
}

const struct file_operations my_fops = {
	.owner = THIS_MODULE,
	.open = dev_open,
	.release = dev_release,
	.read = dev_read,
};
