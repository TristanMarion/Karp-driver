#include <linux/kernel.h>
#include <linux/cdev.h>    // char driver, peut utiliser cdev
#include <linux/fs.h>     // open/close --- /read/write
#include <linux/module.h>
#include <linux/semaphore.h>  // syncro
#include <asm/uaccess.h>   //copy to/from user

struct device {
		char data[100];
		struct semaphore sem; // prevent corruption
} virtual_device;

struct cdev *mcdev;
int major_number; // variable pour stocker nombres
int ret; //variable pour return

dev_t dev_num;

#define DEVICE_NAME	 	"testdevice"

int device_open(struct inode *inode, struct file *filp){

		if (down_interruptible(&virtual_device.sem) != 0){
			printk(KERN_ALERT "test device: could not lock device during open");
			return -1;
		}

		printk(KERN_INFO "test device : device opened !");
		return 0;
}

ssize_t	device_read(struct file* filp, char* bufStoreData, size_t bufCount, loff_t* curOffset){
	
	printk(KERN_INFO "test device : lecture du device");
	ret = copy_to_user(bufStoreData, virtual_device.data,bufCount);
	return ret;
}

ssize_t	device_write(struct file* filp, char* bufSourceData, size_t bufCount, loff_t* curOffset){

	printk(KERN_INFO "test device: ecriture dans le device");
	ret = copy_from_user(virtual_device.data, bufSourceData, bufCount);
	return ret;
}

ssize_t	device_close(struct inode *inode, struct file *filp){
		up(&virtual_device.sem);
		printk(KERN_INFO "test device : closed...");
		return 0;
}

struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = device_open,
	.release = device_close,
	.write = device_write,
	.read = device_read
};

static int driver_entry(void){
	ret = alloc_chrdev_region(&dev_num,0,1,DEVICE_NAME);
	if (ret < 0){
		printk(KERN_ALERT "test device : erreur d'allocation");
		return ret;
	}
	major_number = MAJOR(dev_num);
	printk(KERN_INFO "test device : le nombre majeur est %d", major_number);
	printk(KERN_INFO "\tuse \"mknod /dev/%s c %d 0\" for device file",DEVICE_NAME,major_number);  //dmesg

	mcdev = cdev_alloc();
	mcdev->ops = &fops;
	mcdev->owner = THIS_MODULE;

	ret = cdev_add(mcdev, dev_num, 1);
	if (ret < 0){
		printk(KERN_ALERT "test device : impossible d'add cdev au kernel");
		return ret;
	}

	sema_init(&virtual_device.sem,1);

	return 0;
}

static void driver_exit(void){
	cdev_del(mcdev);
	unregister_chrdev_region(dev_num, 1);
	printk(KERN_ALERT "test device: unloaded module");
}

module_init(driver_entry);
module_exit(driver_exit);