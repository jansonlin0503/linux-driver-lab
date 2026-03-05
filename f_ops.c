#include <linux/fs.h>

#include "device.h"

const struct file_operations my_fops = {
	.owner = THIS_MODULE,
};
