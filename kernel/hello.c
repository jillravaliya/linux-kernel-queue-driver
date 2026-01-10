#include <linux/module.h>
#include <linux/kernel.h>

static int __init hello_init(void) {
	printk(KERN_INFO "hello from jill !");
	return 0;
}

static void __exit hello_exit(void) {
	printk(KERN_INFO "see you again !");
}


module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("JILL");
MODULE_DESCRIPTION("these is my first kernel module");
