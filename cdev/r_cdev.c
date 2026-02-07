#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/atomic.h>

/* Device lifecycle headers */
#include <linux/device.h>
#include <linux/cdev.h>

/* Basic operations for the char device; open, release, read, and write */
static int d_open(struct inode *, struct file*);
static int d_release(struct inode *, struct file *);
static ssize_t d_read(struct file*, char __user *, size_t, loff_t *);
static ssize_t d_write(struct file *, const char __user *, size_t, loff_t *);

/* Binding operations to file operation structure */
static struct file_operations cdev_fops = {
	.read = d_read,
	.write = d_write,
	.open = d_open,
	.release = d_release,
};

/* Device initialization */
#define DEVICE_NAME "c_dev"

static int major;
static struct class *cls;

static int __init chardev_init(void)
{
	major = register_chrdev(0, DEVICE_NAME, &cdev_fops);

	if (major < 0){
		pr_alert("Regist char device failed with %d \n", major);
		return major;
	}

	pr_info("char device created, major number %d \n", major );
	cls = class_create(DEVICE_NAME);
	device_create(cls, NULL, MKDEV(major,0), NULL, DEVICE_NAME);

	return 0;
}

static void __exit chardev_exit(void)
{
	device_destroy(cls, MKDEV(major, 0));
	class_destroy(cls);

	unregister_chrdev(major, DEVICE_NAME);
}

/* Device operations */

/* Device state; using atomic operations to ensure consistency */
enum {
	CDEV_NOT_USED,
	CDEV_EXCLUSIVE_OPEN,
};

static atomic_t already_open = ATOMIC_INIT(CDEV_NOT_USED);

static int d_open(struct inode *inode, struct file *file)
{
	static int counter = 0;

	if(atomic_cmpxchg(&already_open, CDEV_NOT_USED, CDEV_EXCLUSIVE_OPEN))
		return -EBUSY;

	pr_info("Open %d times \n", counter++);

	return 0;
}

static int d_release(struct inode *inode, struct file *file)
{
	atomic_set(&already_open, CDEV_NOT_USED);

	return 0;
}

static ssize_t d_read(struct file *file, char __user *buffer, size_t length, loff_t *offset)
{
        const char msg[] = "Hi user\n";
        size_t msg_len = sizeof(msg);

        /* EOF */
        if (*offset >= msg_len)
                return 0;

        /* Clamp read length */
        if (length > msg_len - *offset)
                length = msg_len - *offset;

        if (copy_to_user(buffer, msg + *offset, length))
                return -EFAULT;

        *offset += length;
        return length;
}

static ssize_t d_write(struct file *file, const char __user *buff, size_t len, loff_t *off)
{
	pr_alert("Yet\n");
	return -EINVAL;
}

module_init(chardev_init);
module_exit(chardev_exit);

MODULE_LICENSE("GPL");
