#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/wait.h>


#define IOCTL_SET_SIZE _IOW('a', 'a', int*)
#define IOCTL_PUSH_DATA _IOW('a', 'b', struct data*)
#define IOCTL_POP_DATA _IOR('a', 'c', struct data*)


struct data {

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
static DECLARE_WAIT_QUEUE_HEAD(read_wait);
static DECLARE_WAIT_QUEUE_HEAD(write_wait);

static int dev_open(struct inode *inode, struct file *file) {

	return 0;
}


static int dev_release(struct inode *inode, struct file *file) {

	return 0;
}

static long dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {

	struct data user_data;
	int size;
	char *kbuf;
	int i;


	switch(cmd) {
		case IOCTL_SET_SIZE:
			if(queue != NULL) {
				return -EINVAL;
			}

			if(copy_from_user(&size, (int*)arg, sizeof(int))) {
				return -EFAULT;
			}

			queue = kmalloc(sizeof(struct circular_queue), GFP_KERNEL);
			if(!queue) {
				return -ENOMEM;
			}

			queue->buffer = kmalloc(size, GFP_KERNEL);
			if(!queue->buffer) {
				kfree(queue);
				queue = NULL;
				return -ENOMEM;
			}

			queue->size = size;
			queue->head = 0;
			queue->tail = 0;
			queue->count = 0;
			break;

		case IOCTL_PUSH_DATA:
			if(!queue) {
				return -EINVAL;
			}

			if(copy_from_user(&user_data, (struct data*)arg, sizeof(struct data))) {
				return -EFAULT;
			}

			if(user_data.length <= 0 || user_data.length > queue->size) {
				return -EINVAL;
			}

			if(wait_event_interruptible(write_wait, queue->count + user_data.length <= queue->size)) {
				return -ERESTARTSYS;
			}

			kbuf = kmalloc(user_data.length, GFP_KERNEL);
			if(!kbuf) {
				return -ENOMEM;
			}

			if(copy_from_user(kbuf, user_data.data, user_data.length)) {
				kfree(kbuf);
				return -EFAULT;
			}

			for(i = 0; i < user_data.length; i++) {
				queue->buffer[queue->head] = kbuf[i];
				queue->head = (queue->head + 1) % queue->size;
				queue->count++;
			}

			kfree(kbuf);
			wake_up_interruptible(&read_wait);
			break;

		case IOCTL_POP_DATA:
			if(!queue) {
				return -EINVAL;
			}

			if(copy_from_user(&user_data, (struct data*)arg, sizeof(struct data))) {
				return -EFAULT;
			}

			if(user_data.length <= 0) {
				return -EINVAL;
			}

			if(wait_event_interruptible(read_wait, queue->count >= user_data.length)) {
				return -ERESTARTSYS;
			}

			kbuf = kmalloc(user_data.length, GFP_KERNEL);
			if(!kbuf) {
				return -ENOMEM;
			}

			for(i = 0; i < user_data.length; i++) {
				kbuf[i] = queue->buffer[queue->tail];
				queue->tail = (queue->tail + 1) % queue->size;
				queue->count--;
			}

			if(copy_to_user(user_data.data, kbuf, user_data.length)) {
				kfree(kbuf);
				return -EFAULT;
			}

			kfree(kbuf);
			wake_up_interruptible(&write_wait);
			break;

		default:
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

	ret = alloc_chrdev_region(&dev, 0 ,1, "jill");
	if(ret < 0) {
		return ret;
	}

	dev_major = MAJOR(dev);

	cdev_init(&my_cdev, &fops);
	my_cdev.owner = THIS_MODULE;


	ret = cdev_add(&my_cdev, dev, 1);
	if(ret < 0) {
		unregister_chrdev_region(dev, 1);
		return ret;
	}

	my_class = class_create("jill");
	if(IS_ERR(my_class)) {
		cdev_del(&my_cdev);
		unregister_chrdev_region(dev, 1);
		return PTR_ERR(my_class);
	}

	my_device = device_create(my_class, NULL, dev, NULL, "jill");
	if(IS_ERR(my_device)) {
		class_unregister(my_class);
		cdev_del(&my_cdev);
		unregister_chrdev_region(dev, 1);
		return PTR_ERR(my_device);
	}

	return 0;
}


static void __exit jill_exit(void) {

	dev_t dev = MKDEV(dev_major, 0);

	if(queue) {
		if(queue->buffer) {
			kfree(queue->buffer);
		}
		kfree(queue);
	}

	device_destroy(my_class, dev);
	class_unregister(my_class);
	cdev_del(&my_cdev);
	unregister_chrdev_region(dev, 1);
}


module_init(jill_init);
module_exit(jill_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("JILL");
MODULE_DESCRIPTION("my first circular queue driver");


