/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include "aesdchar.h"
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Amey Sharad Dashaputre"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

 /*************************************************************************************************
 * Function Description : open call used to get the character device(cdev) from aesd_dev structure.
 * Parameters           : kernel inode structure, file *filp kernel file structure passed from caller
 * Returns              : int: status
 *************************************************************************************************/
int aesd_open(struct inode *inode, struct file *filp)
{
	struct aesd_dev *dev;
	PDEBUG("open");
	dev = container_of(inode->i_cdev, struct aesd_dev, cdev); //store cdev in inode.ic_dev and store in private data for future reference
	filp->private_data = dev;
	return 0;
}

/*************************************************************************************************
 * Function Description : release system call used to release the kernel resource
 * Parameters           : kernel inode structure, file *filp kernel file structure passed from caller
 * Returns              : int: status
 *************************************************************************************************/
int aesd_release(struct inode *inode, struct file *filp)
{
	PDEBUG("release");

	return 0;
}

/*************************************************************************************************
 * Function Description : aesd_read
 * Parameters             buf: the buffer pointer at which the read data needs to be stored,
 			   count the number of bytes required to be read from kernel buffer,
 			   f_pos the entry offset location in kernel buffer from where data will be read
 * Returns              : int: status
 *************************************************************************************************/
ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
	ssize_t retval = 0;
	struct aesd_dev *dev;

	//entry and offset for circular buffer
	struct aesd_buffer_entry *read_entry = NULL;
	ssize_t read_offset = 0, unread_data_bytes =0;
	
	PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
	printk(KERN_DEBUG "read %zu bytes with offset %lld",count,*f_pos);

	//get the skull device from file structure
	dev = (struct aesd_dev*) filp->private_data;
	
	//put error checks here, if count is zero, all other parameters are not null
	
	
	if (count == 0)
	{
		return 0;
	}
	
	if(filp == NULL || buf == NULL || f_pos == NULL)
	{
		return -EFAULT; //bad address
	}
	
	
	if(mutex_lock_interruptible(&dev->lock)) //lock on mutex here, preferrable interruptable, check for error
	{
		PDEBUG(KERN_ERR "mutex lock couldn't be acquired ");
		return -ERESTARTSYS; //check this
	}
	
	read_entry = aesd_circular_buffer_find_entry_offset_for_fpos(&(dev->cir_buff), *f_pos, &read_offset);  //find the read entry, and offset for given f_pos
	
	if(read_entry == NULL)
	{
		goto error_exit;
	}
	else
	{
		if(count > (read_entry->size - read_offset)) //check if count is greater that current max read size, then limit
		{
			count = read_entry->size - read_offset;
		}
	}
	
	unread_data_bytes = copy_to_user(buf, (read_entry->buffptr + read_offset), count); 	//now read data using copy_to_user
	retval = count - unread_data_bytes; //return whatever is copied and update fpos accordingly
	*f_pos += retval;

//handle errors
error_exit:
	mutex_unlock(&(dev->lock));
	return retval;
}

/*************************************************************************************************
 * Function Description : aesd_write
 * Parameters           : struct file *filp kernel file structure passed from caller, 
 			   buf: the buffer pointer at which the read data needs to be stored,
 			   count the number of bytes required to be read from kernel buffer,
 			   f_pos the entry offset location in kernel buffer from where data will be read
 * Returns              : int: no of bytes successfully read
 *************************************************************************************************/
ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
	struct aesd_dev *dev;
	const char *new_entry = NULL;
	ssize_t retval = -ENOMEM;
	ssize_t unwritten_data_bytes = 0;
	
	PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
	
	//check arguement errors
	if(count == 0)
	{
		return 0;
	}
	if(filp == NULL || buf == NULL || f_pos== NULL)
	{
		return -EFAULT;
	}
	
	dev = (struct aesd_dev*) filp->private_data; //cast the aesd_device from private data

	if(mutex_lock_interruptible(&(dev->lock))) // check for lock the mutex
	{
		PDEBUG(KERN_ERR "mutex lock couldn't be acquired ");
		return -ERESTARTSYS;
	}
	
	if(dev->buff_entry.size == 0) //allocate the buffer using kmalloc, store address in buffptr
	{
		dev->buff_entry.buffptr = kmalloc(count*sizeof(char), GFP_KERNEL);
		if(dev->buff_entry.buffptr == NULL)
		{
			PDEBUG("kmalloc error");
			goto exit_error;
		}
	}
	
	else 	//realloc if already allocated
	{
		dev->buff_entry.buffptr = krealloc(dev->buff_entry.buffptr, (dev->buff_entry.size + count)*sizeof(char), GFP_KERNEL);
		if(dev->buff_entry.buffptr == NULL)
		{
			PDEBUG("krealloc error");
			goto error_exit;
		}
	}
	unwritten_data_bytes = copy_from_user((void *)(dev->buff_entry.buffptr + dev->buff_entry.size), buf, count); //copy data from user space buffer to current command
	retval = count - unwritten_data_bytes; //actual bytes written
	dev->buff_entry.size += retval;

	//Check for \n in command, if found add the entry in circular buffer
	if(memchr(dev->buff_entry.buffptr, '\n', dev->buff_entry.size))
	{
		new_entry = aesd_circular_buffer_add_entry(&dev->cir_buff, &dev->buff_entry); 
		if(new_entry)
		{
			kfree(new_entry);// !doubt about this
		}
		//clear entry parameters
		dev->buff_entry.buffptr = NULL;
		dev->buff_entry.size = 0;
	}
	PDEBUG("not doing k_free for now");

error_exit:
	mutex_unlock(&dev->lock);
	return retval;
}
struct file_operations aesd_fops = {
	.owner =    THIS_MODULE,
	.read =     aesd_read,
	.write =    aesd_write,
	.open =     aesd_open,
	.release =  aesd_release,
};

/*************************************************************************************************
 * Function Description : initializes the device and add it.
 * Parameters           : aesd_dev *dev
 * Returns              : NA
 *************************************************************************************************/
static int aesd_setup_cdev(struct aesd_dev *dev)
{
	int err, devno = MKDEV(aesd_major, aesd_minor);

	cdev_init(&dev->cdev, &aesd_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &aesd_fops;
	err = cdev_add (&dev->cdev, devno, 1);
	if (err) {
		printk(KERN_ERR "Error %d adding aesd cdev", err);
	}
	return err;
}


/*************************************************************************************************
 * Function Description : register the device and initialize the kernel
 * Parameters           : NA
 * Returns              : NA
 *************************************************************************************************/

int aesd_init_module(void)
{
	dev_t dev = 0;
	int result;
	result = alloc_chrdev_region(&dev, aesd_minor, 1,
			"aesdchar");
	aesd_major = MAJOR(dev);
	if (result < 0) 
	{
		printk(KERN_WARNING "Can't get major %d\n", aesd_major);
		return result;
	}
	memset(&aesd_device,0,sizeof(struct aesd_dev));
	
	mutex_init(&aesd_device.lock); //Initialize the mutex and circular buffer
	aesd_circular_buffer_init(&aesd_device.cir_buff);
	result = aesd_setup_cdev(&aesd_device);
	
	if( result ) 
	{
		unregister_chrdev_region(dev, 1);
	}
	return result;

}


/*************************************************************************************************
 * Function Description : unregisters the device and deallocated all the kernel data structures and delte the device
 * Parameters           : NA
 * Returns              : NA
 *************************************************************************************************/
void aesd_cleanup_module(void)
{
	struct aesd_buffer_entry *entry = NULL; //free circular buffer entries
	uint8_t index = 0; 
	dev_t devno = MKDEV(aesd_major, aesd_minor);
	cdev_del(&aesd_device.cdev);

	kfree(aesd_device.buff_entry.buffptr); //free the buff_entry buffpte

	AESD_CIRCULAR_BUFFER_FOREACH(entry, &aesd_device.cir_buff, index)
	{
		if(entry->buffptr != NULL)
		{
			kfree(entry->buffptr);
		}
	}

	unregister_chrdev_region(devno, 1);
}

module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
