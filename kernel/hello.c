#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>


#define IOCTL_SET_SIZE _IOW('a', 'a', int32_t*)
#define IOCTL_PUSH_DATA _IOW('a', 'b', struct queue_data*)
#define IOCTL_POP_DATA _IOR('a', 'c', struct queue_data*)


struct queue_data {
	int length;
	char *data;
};


struct circular_queue {
	char *buffer;
	int size;
	int head;
	int tail;
	int count;
};


static int dev_major;
static struct cdev my_cdev;
static struct class *my_class;
static struct device *my_device;
static struct circular_queue *queue = NULL;

static int dev_open(struct inode *inode, struct file *file) {
	printk(KERN_INFO "jill : device opened\n");
	return 0;
}


static int dev_release(struct inode *inode, struct file *file) {
	printk(KERN_INFO "jill : device closed\n");
	return 0;
}

static long dev_ioctl(struct file *file, unsigned int cmd, unsigned long srg) {
	printk(KERN_INFO "jill : ioctl called with cmd=%u\n", cmd);

	switch(cmd) {
		case IOCTL_SET_SIZE:
		printk(KERN_INFO "jill : SET_SIZE command received\n");
		break;

		case IOCTL_PUSH_DATA:
		printk(KERN_INFO "jill : PUSH_DATA command received\n");
		break;

		case IOCTL_POP_DATA:
		printk(KERN_INFO "jill : POP_DATA command received\n");
		break;

		default:
		printk(KERN_INFO "jill : unknownioctl command\n");
		return -EINVAL;
	}

	return 0;
}



static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = dev_open,
	.release = dev_release,
	.unlocked_ioctl = dev_ioctl,
};


static int __init jill_init(void) {
	dev_t dev;
	int ret;

	printk(KERN_INFO "jill : loading driver\n");

	ret = alloc_chrdev_region(&dev, 0, 1, "jill");
	if (ret < 0) {
		printk(KERN_INFO "jill : failed to allocate device number\n");
		return ret;
	}


	dev_major = MAJOR(dev);
	printk(KERN_INFO "jill : got major number %d\n", dev_major);

	cdev_init(&my_cdev, &fops);
	my_cdev.owner = THIS_MODULE;

	ret = cdev_add(&my_cdev, dev, 1);
	if (ret < 0) {
		unregister_chrdev_region(dev, 1);
		printk(KERN_ERR "jill : failed to add cdev\n");
		return ret;
	}


	my_class = class_create("jill");
	if(IS_ERR(my_class)) {
		cdev_del(&my_cdev);
		unregister_chrdev_region(dev, 1);
		printk(KERN_ERR "jill : failed to create class\n");
		return PTR_ERR(my_class);
	}


	my_device = device_create(my_class, NULL, dev, NULL, "jill");
	if(IS_ERR(my_device)) {
		class_destroy(my_class);
		cdev_del(&my_cdev);
		unregister_chrdev_region(dev, 1);
		printk(KERN_ERR "jill : failed to create device\n");
		return PTR_ERR(my_device);
	}

	printk(KERN_INFO "jill : device created sucessfully\n");
	return 0;
}


static void __exit jill_exit(void) {
	dev_t dev = MKDEV(dev_major, 0);

	printk(KERN_INFO "jill : unloading driver\n");

	device_destroy(my_class, dev);
	class_destroy(my_class);
	cdev_del(&my_cdev);
	unregister_chrdev_region(dev, 1);

	printk(KERN_INFO "jill : see you soon !\n");
}



module_init(jill_init);
module_exit(jill_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("JILL");
MODULE_DESCRIPTION("my first cirular queue driver");
