/******************************************
* Device driver for Shared Queues 
* Author: Balarupini Rajendran
* Date: 10/02/17
********************************************/

#include "thread_msg.h"
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/string.h>
#include <linux/device.h>
#include <linux/jiffies.h>
#include <linux/pci.h>
#include <linux/param.h>
#include <linux/list.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/errno.h>

#define DEV_NAME "squeuen"
#define CLASS_NAME "sqdeviceclass"
#define DEV_NAME1 "dataqueue1"
#define DEV_NAME2 "dataqueue2"
#define NUM_OF_Q 2
#define MAX 10

static struct class*  sq_dev_class = NULL;
static struct device* myDevice1 = NULL;
static struct device* myDevice2 = NULL;
static dev_t majorNumber;

static int __init sqdriver_init(void);
int sqdriver_open(struct inode *inode, struct file *file);
int sqdriver_release(struct inode *inode, struct file *file);
static void __exit sqdriver_exit(void);
ssize_t sqdriver_write(struct file *file, const char *buf, size_t count, loff_t *ppos);
ssize_t sqdriver_read(struct file *file, char *buf, size_t count, loff_t *ppos);

//structure of the shared queue device
struct mysqueue_dev
{
  struct cdev cdev;
  char name_of_device[20];
  struct thread_msg *queuen[MAX];
  int front;
  int rear;

};
// structure for file operations
static struct file_operations fops =
{
  .owner = THIS_MODULE,
  .open = sqdriver_open,
  .read = sqdriver_read,
  .write = sqdriver_write,
  .release = sqdriver_release,
};
// initializing the device driver. Runs when the module is installed.
static int __init sqdriver_init(void){
  printk(KERN_INFO "sqdriver initialized\n");
  majorNumber = register_chrdev(0, DEV_NAME, &fops);
   if (majorNumber<0){
      printk(KERN_ALERT "Failed to register a major number\n");
      return majorNumber;
    }
    printk(KERN_INFO "Registered successfully with major number %d \n",majorNumber);
    sq_dev_class = class_create(THIS_MODULE, CLASS_NAME);
       if (IS_ERR(sq_dev_class)){
          unregister_chrdev(majorNumber, DEV_NAME);
          printk(KERN_ALERT "Device class was not registered\n");
          return PTR_ERR(sq_dev_class);
       }
       printk(KERN_INFO "Device class registered correctly\n");
// creating the device of the type sq_dev_class
    myDevice1 = device_create(sq_dev_class, NULL, MKDEV(majorNumber, 0), NULL, DEV_NAME1);
         if (IS_ERR(myDevice1)){
            class_destroy(sq_dev_class);
            unregister_chrdev(majorNumber, DEV_NAME1);
            printk(KERN_ALERT "Failed to create dataqueue1\n");
            return PTR_ERR(myDevice1);
         }
    myDevice2 = device_create(sq_dev_class, NULL, MKDEV(majorNumber, 1), NULL, DEV_NAME2);
        if (IS_ERR(myDevice2)){
            class_destroy(sq_dev_class);
            unregister_chrdev(majorNumber, DEV_NAME2);
            printk(KERN_ALERT "Failed to create dataqueue2\n");
            return PTR_ERR(myDevice2);
          }
         printk(KERN_INFO "device class created correctly\n");
         return 0;

}
//Opening the device
int sqdriver_open(struct inode *inode, struct file *file)
{
  printk("inside sqdriver_open");
  struct mysqueue_dev* sq_dev_ptr;

  sq_dev_ptr = container_of(inode->i_cdev, struct mysqueue_dev, cdev);
  sq_dev_ptr->front =-1;
  sq_dev_ptr->rear =-1;
  file->private_data = sq_dev_ptr;
  printk("Opening the queue device %s\n", sq_dev_ptr->name_of_device);
  return 0;
}

// Releasing the device
int sqdriver_release(struct inode *inode, struct file *file)
{
  struct mysqueue_dev* sq_dev_ptr ;
  printk(KERN_INFO "Driver: close()\n");
  sq_dev_ptr = file->private_data;
  printk("Closing the queue device %s\n", sq_dev_ptr->name_of_device);
  return 0;
}
// exit function. Executes when the module is removed
static void __exit sqdriver_exit(void){
   device_destroy(sq_dev_class, MKDEV(majorNumber, 0));     // remove the device
   class_unregister(sq_dev_class);                          // unregister the device class
   class_destroy(sq_dev_class);                             // remove the device class
   unregister_chrdev(majorNumber, DEV_NAME);             // unregister the major number
   printk(KERN_INFO "mydev_exit is called!\n");
}

// writing into the device
ssize_t sqdriver_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{

	int res =0;
  
  struct mysqueue_dev *sq_dev_ptr;
  sq_dev_ptr = file->private_data;
  printk(KERN_INFO "Driver: write()\n");
  if((sq_dev_ptr->front==0 && sq_dev_ptr->rear==MAX-1) || (sq_dev_ptr->front==sq_dev_ptr->rear+1))
  {
    printk(KERN_INFO "Queue is Full");
    return -1;
  }

  sq_dev_ptr->queuen[sq_dev_ptr->rear]=(struct thread_msg*) kmalloc(sizeof(struct thread_msg), GFP_KERNEL);

  res = copy_from_user(sq_dev_ptr->queuen[sq_dev_ptr->rear], buf, sizeof(struct thread_msg));
  if(sq_dev_ptr->front == -1 )
      sq_dev_ptr->front=0;

  if(sq_dev_ptr->rear==MAX-1)
      sq_dev_ptr->rear=0;
  else
      sq_dev_ptr->rear=sq_dev_ptr->rear+1;

  return res;

}
// reading from the device
ssize_t sqdriver_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
  int res = 0;
  struct thread_msg* temp_msg;
  struct mysqueue_dev* sq_dev_ptr;
  ssize_t size;

  size = (sizeof(struct thread_msg) + 80);
  sq_dev_ptr = file->private_data;
  printk(KERN_INFO "Driver: read()\n");


  if (sq_dev_ptr->front == -1)
  {
    printk("Queue is empty");
    return (-1);
  }


  temp_msg = (struct thread_msg*) kmalloc(sizeof(struct thread_msg), GFP_KERNEL);
  temp_msg=sq_dev_ptr->queuen[sq_dev_ptr->front];
  res = copy_to_user(buf, temp_msg, sizeof(struct thread_msg)); // Copying to user space

  if(sq_dev_ptr->front==sq_dev_ptr->rear)
  {
      sq_dev_ptr->front=-1;
      sq_dev_ptr->rear=-1;
  }
  else if(sq_dev_ptr->front==MAX-1)
      sq_dev_ptr->front=0;
  else
      sq_dev_ptr->front=sq_dev_ptr->front+1;

  kfree(temp_msg);
  return res;

}

module_init (sqdriver_init);
module_exit (sqdriver_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Balarupini Rajendran");

