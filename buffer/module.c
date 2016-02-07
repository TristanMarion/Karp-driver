#include <linux/kernel.h>
#include <linux/cdev.h>         // char device : cdev
#include <linux/fs.h>           // open/close --- /read/write
#include <linux/module.h>       // module
#include <linux/semaphore.h>    // Permissions d'accès
#include <asm/uaccess.h>        // copy to/from user

#define DEVICE_NAME	 	"samynaceri"

struct device {
  char data[1000];
  struct semaphore sem; // Gestion des permissions d'accès
} virtual_device;

// On declare les variables en global pour eviter de saturer la stack du kernel

struct cdev *my_cdev;   // création du périphérique
int major_number;       // nombre majeur
int ret;                // variable des returns

dev_t dev_num;          // contiendra les nombres majeurs et mineurs

int device_open(struct inode *inode, struct file *file_p){
  if (down_interruptible(&virtual_device.sem) != 0) // Si le périphérique n'est pas déjà utilisé
  {
    printk(KERN_ALERT "--== SAMY NACERI ==-- Test device: impossible d'utiliser le périphérique");
    return -1;
  }
  
  printk(KERN_INFO "--== SAMY NACERI ==-- Test device : périphérique ouvert !");
  return 0;
}

ssize_t	device_read(struct file* file_p, char* buffer, size_t taille, loff_t* curOffset)
{
  printk(KERN_INFO "--== SAMY NACERI ==-- Test device : lecture du périphérique");
  ret = copy_to_user(buffer, virtual_device.data, taille);    // Envoie le contenu du buffer du périphérique à l'utilisateur
  return ret;
}

ssize_t	device_write(struct file* file_p, const char* buffer, size_t taille, loff_t* curOffset){
  printk(KERN_INFO "--== SAMY NACERI ==-- Test device: écriture dans le périphérique");
  ret = copy_from_user(virtual_device.data, buffer, taille);  // Envoie le contenu de l'utilisateur au buffer du périphérique
  return ret;
}

int	device_close(struct inode *inode, struct file *file_p){
  up(&virtual_device.sem);        // Libère le périphérique
  printk(KERN_INFO "--== SAMY NACERI ==-- Test device : périphérique fermé...");
  return 0;
}

// Définition des fonctions et informations liées au périphérique
struct file_operations fops = {
  .owner = THIS_MODULE,
  .open = device_open,
  .release = device_close,
  .write = device_write,
  .read = device_read
};

// Fonction exécutée lors du lancement du module
static int driver_entry(void)
{ 
  ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME); // Allocation dynamique des numéros de périphérique
  if (ret < 0)
  {
    printk(KERN_ALERT "--== SAMY NACERI ==-- Test device : erreur d'allocation");
    return ret;
  }
  major_number = MAJOR(dev_num); // Extraction du nombre majeur
  printk(KERN_INFO "--== SAMY NACERI ==-- Test device : le nombre majeur est %d", major_number);
  printk(KERN_INFO "--== SAMY NACERI ==-- Utilisez \"mknod /dev/%s c %d 0\" pour créer le périphérique.", DEVICE_NAME, major_number);
  my_cdev = cdev_alloc();  // Allocation d'un character device
  my_cdev->ops = &fops; // Attribution des operations liées au périphérique
  my_cdev->owner = THIS_MODULE; // Attribution du owner
  ret = cdev_add(my_cdev, dev_num, 1); // Ajout du périphérique au kernel
  if (ret < 0)
  {
    printk(KERN_ALERT "--== SAMY NACERI ==-- Test device : impossible d'add cdev au kernel");
    return ret;
  }
  sema_init(&virtual_device.sem,1);   /*  Initialisation du semaphore, il permet de bloquer l'accès au périphérique
                                          si la ressource est deja trop utilisée */
  printk(KERN_INFO "--== SAMY NACERI ==-- Test device: module chargé");
  return 0;
}

static void driver_exit(void){
  cdev_del(my_cdev);                      // On supprime le périphérique du kernel
  unregister_chrdev_region(dev_num, 1);   // On "free" le alloc_chrdev_region
  printk(KERN_ALERT "--== SAMY NACERI ==-- Test device: module déchargé");
}

module_init(driver_entry);  // On précise la fonction à exécuter lors d'un "insmod"
module_exit(driver_exit);   // et celle à exécuter lors d'un "rmmod"
