/*
 * keyboard.c
 *
 * Example of kernel module that implements a keyboard interrupt
 * handler.  The module creates the entry /dev/kbd that applications
 * can use to catch keyboard events. By reading from that device, a
 * process will block until a key is pressed or released. For
 * simplicity, the device does not support writing and more complex
 * configuration.
 *
 * REQUIREMENTS
 * 
 * This module binds to the keyboard interrupt and shares it with the
 * native Linux keyboard interrupt handler. The native handler needs
 * to be modified to support interrupt sharing. See
 * src/include/asm-i386/keyboard.h for more information.
 *
 * Device File System (devfs), Linux Kernel 2.4.x
 *
 * AUTHOR
 *
 * Emanuele Altieri
 * ealtieri@hampshire.edu
 * June 2002
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/fs.h>
#include <linux/devfs_fs_kernel.h>
#include <asm/uaccess.h>
#include <asm/io.h>

static void kbd_irq_handler(int, void*, struct pt_regs*);
static ssize_t kbd_read(struct file*, char*, size_t, loff_t*);

/* file operations */

static struct file_operations kbd_fops = {
	read:    kbd_read,
};

/* data */

static devfs_handle_t kbd_handle; // devfs : Système de fichiers qui traite les fichiers spéciaux
static char* kbd_device = "kbd";
static unsigned short kbd_buffer = 0x0000; /* HByte = Status, LByte = Scancode */
static wait_queue_head_t kbd_irq_waitq; // a queue of processes that are waiting for an event

int __init kbd_init(void)
{
	/* register device */
	SET_MODULE_OWNER(&kbd_fops);
	kbd_handle =  devfs_register(NULL, kbd_device, //Enregistre une entrée périphérique
				    DEVFS_FL_AUTO_DEVNUM | DEVFS_FL_AUTO_OWNER, // Automatically generate device number donc 0, 0 | Auto owner
				    0, 0, S_IFCHR | S_IRUSR, // the file type of character-oriented device file | Read permission bit for the owner of the file
				    &kbd_fops, NULL); 
	if (kbd_handle <= 0) {
		printk(KERN_ERR "%s: cannot register device\n", kbd_device);
		return(-EBUSY);
	}
	/* request interrupt line */
	if (request_irq(1, kbd_irq_handler, SA_SHIRQ, 		// 1 : IRQ of the keyboard, kbd_irq_handler : function to call, SA_SHIRQ : partage de l'interuption avec d'autres gestionnaires
			kbd_device, (void*) &kbd_handle) < 0) { 	// name, numéro de périphérique du gestionnaire
		printk(KERN_CRIT "%s: interrupt line busy\n",kbd_device);
		devfs_unregister(kbd_handle); // Supprime l'entrée périphérique
		return(-EBUSY);
	}
	/* initialize waiting queue */
	init_waitqueue_head(&kbd_irq_waitq); // Initialisation de la queue
	return(0);
} // kbd_init()

void __exit kbd_cleanup(void)
{
	devfs_unregister(kbd_handle); 		// Supprime l'entrée périphérique
	free_irq(1, (void*) &kbd_handle); 	// free an interrupt allocated with request_irq
} // kbd_cleanup()

// Put current process to sleep. The process will be awaken by the
// interrupt handler when an keyboard interrupt occurs. The character
// code and keyboard status is then sent to the process.
static ssize_t 
kbd_read(struct file *filp, char *buf, size_t count, loff_t* f_pos)
{
	interruptible_sleep_on(&kbd_irq_waitq);
	copy_to_user((void*) buf, (void*) &kbd_buffer, sizeof(kbd_buffer));
	return(sizeof(kbd_buffer));
} // kbd_read()

// Keyboard interrupt handler. Retrieves the character code (scancode)
// and keyboard status from the keyboard I/O ports. Awakes processes
// waiting for a keyboard event.
static void kbd_irq_handler(int irq, void* dev_id, struct pt_regs *regs)
{
	unsigned char status, scancode;
	status = inb(0x64);
	scancode = inb(0x60);
	kbd_buffer = (unsigned short) ((status << 8) | (scancode & 0x00ff));
	wake_up_interruptible(&kbd_irq_waitq);
} // kbd_irq_handler()

MODULE_AUTHOR("Emanuele Altieri (ealtieri@hampshire.edu)");
MODULE_DESCRIPTION("Simple keyboard device driver");
MODULE_LICENSE("GPL");

module_init(kbd_init);
module_exit(kbd_cleanup);
