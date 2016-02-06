#include <linux/kernel.h>
#include <linux/cdev.h>    // char driver, cdev
#include <linux/fs.h>     // open/close --- /read/write
#include <linux/module.h>
#include <linux/semaphore.h>  // Videur de boite de nuit
#include <asm/uaccess.h>   //copy to/from user

struct device {
  char data[1000];
  struct semaphore sem; // c'est ce qui empeche trop de gens de rentrer dans la boite de nuit
} virtual_device;

// On declare les variables en global pour eviter de saturer la stack du kernel

struct cdev *my_cdev;
int major_number; // variable pour garder le nombre majeur
int ret; //variable des returns

dev_t dev_num; // stoque les nombres majeurs et mineurs

#define DEVICE_NAME	 	"samynaceri"

int device_open(struct inode *inode, struct file *filp){
  if (down_interruptible(&virtual_device.sem) != 0){ //Regarde s'il reste de la place dans la boite de nuit
    printk(KERN_ALERT "test device: could not lock device during open");
    return -1;
  }
  
  printk(KERN_INFO "test device : device opened !");
  return 0;
}

ssize_t	device_read(struct file* filp, char* bufStoreData, size_t bufCount, loff_t* curOffset){
  printk(KERN_INFO "test device : lecture du device");
  ret = copy_to_user(bufStoreData, virtual_device.data, bufCount); //Envoie les infos du kernel au user
  return ret;
}

ssize_t	device_write(struct file* filp, const char* bufSourceData, size_t bufCount, loff_t* curOffset){
  printk(KERN_INFO "test device: ecriture dans le device");
  ret = copy_from_user(virtual_device.data, bufSourceData, bufCount); //Envoie les infos du user au kernel
  return ret;
}

int	device_close(struct inode *inode, struct file *filp){
  up(&virtual_device.sem); //Libere une place dans la boite de nuit
  printk(KERN_INFO "test device : closed...");
  return 0;
}

// On dit au driver/module quelles sont les fonctions permettant d'ouvrir, fermer, lire et ecrire
struct file_operations fops = {
  .owner = THIS_MODULE,
  .open = device_open,
  .release = device_close,
  .write = device_write,
  .read = device_read
};

static int driver_entry(void){ // point d'entree 
  ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME); //allocation dynamique d'un numero de device
  if (ret < 0){
    printk(KERN_ALERT "test device : erreur d'allocation");
    return ret;
  }
  major_number = MAJOR(dev_num); //extraction du nombres majeurs avec la macro MAJOR
  printk(KERN_INFO "test device : le nombre majeur est %d", major_number);
  printk(KERN_INFO "\tuse \"mknod /dev/%s c %d 0\" for device file",DEVICE_NAME,major_number);
  my_cdev = cdev_alloc();  // Allocation d'un cdev
  my_cdev->ops = &fops; // Attribution des operations
  my_cdev->owner = THIS_MODULE; // Attribution du owner
  ret = cdev_add(my_cdev, dev_num, 1); //on add le cdev au kernel
  if (ret < 0){
    printk(KERN_ALERT "test device : impossible d'add cdev au kernel");
    return ret;
  }
  sema_init(&virtual_device.sem,1); // initialisation du semaphore, il permet de synchro l'acces aux ressources et de bloquer si la ressource est deja trop utilisee
  return 0;
}

static void driver_exit(void){
  cdev_del(my_cdev); // on del le cdev du kernel
  unregister_chrdev_region(dev_num, 1); // on "free" le alloc_chrdev_region
  printk(KERN_ALERT "test device: unloaded module");
}

module_init(driver_entry); //on precise au module ou commencer
module_exit(driver_exit); // et ou finir
