
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h> /* printk() */
#include <linux/fs.h>     /* everything... */
#include <linux/errno.h>  /* error codes */
#include <linux/types.h>  /* size_t */
#include <linux/vmalloc.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/hdreg.h>

#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/spinlock_types.h>

#include "socket.h"
#include "httpdisk.h"

MODULE_LICENSE("Dual BSD/GPL");

static int major_num = 0;
module_param(major_num, int, 0);
static int hardsect_size = 512;
module_param(hardsect_size, int, 0);
static int nsectors = 1024;  /* How big the drive is */
module_param(nsectors, int, 0);

#define MAX_SBD_CACHE_THREAD 1
#define MAX_CACHE 32
static struct task_struct *task[MAX_SBD_CACHE_THREAD];

#define PORT (htons(80))
#define IP (htonl(0xc0a84b01)) 

#define KERNEL_SECTOR_SIZE 512


struct sbd_cache{
	unsigned long sector;
	unsigned long nsect;
	int write;
	void * buffer;
	struct list_head queuelist;
};

struct sbd_device {
    	unsigned long size;
    	spinlock_t lock;
    	struct gendisk *gd;

	int number_of_error;
	int number_of_hit;
	int number_of_write_request;
	int number_of_read_request;
	int number_of_request;

	int pre_cache_count;
	spinlock_t pre_cache_lock;
	struct list_head pre_cache_list;
	wait_queue_head_t pre_cache_wait_queue;
	
	int cache_count;
	spinlock_t cache_lock;
	struct list_head cache_list;
	wait_queue_head_t cache_wait_queue;
};
static struct sbd_device dev={0};

struct sbd_cache * sbd_predict_cache(unsigned long sector,unsigned long nsect,int write)
{
	struct sbd_cache * tmp=NULL;

	tmp = vmalloc(sizeof(struct sbd_cache));
	if(tmp==NULL){
		return NULL;
	}
	tmp->write = write;
	tmp->sector = sector;
	tmp->nsect = nsect*2;
	tmp->sector = sector + nsect;
	//tmp->buffer = vmalloc(tmp->nsect * KERNEL_SECTOR_SIZE);
	return tmp;
}
void sbd_free_cache(struct sbd_cache *cache)
{
	if(!cache)
		return;
	//vfree(cache->buffer);
	vfree(cache);
	
}
bool sbd_get_cache_before_transfer(struct sbd_device *dev,unsigned long sector,unsigned long nsect,void * buffer,int write)
{
	bool find =false;
	struct sbd_cache *cache=NULL;
	struct list_head *pos;
	if(write){//we do not cope with write operation temporarily
		return false;
	}

	//check to see whether this request has been cached 
	find = false;
	spin_lock(&dev->cache_lock);
	list_for_each(pos, &dev->cache_list)
	{
		cache = list_entry(pos, struct sbd_cache, queuelist);
		if(cache->sector<=sector && cache->nsect>=nsect){
			find = true;
			dev->number_of_hit++;
			break;
		}
	}	
	spin_unlock(&dev->cache_lock);
	
	if(find && cache){//if found cached
		//unsigned long nbytes = nsect * KERNEL_SECTOR_SIZE;
		//unsigned long offset = (sector-cache->sector)*KERNEL_SECTOR_SIZE;
		//memcpy(buffer,cache->buffer+offset,nbytes);
		//return true;
	}

	//predict next to be pre cached
	struct sbd_cache * tmp = sbd_predict_cache(sector,nsect,write);
	if(tmp==NULL){
		return false;
	}

       /*
 	* check to see whether this request has been pre cached 
	* and find the last pre cache 
	*/
	find = false;
	cache = NULL;
	spin_lock_irq(&dev->pre_cache_lock);
	if(!list_empty(&dev->pre_cache_list)){
		list_for_each(pos, &dev->pre_cache_list)
		{
			cache = list_entry(pos, struct sbd_cache, queuelist);

			//check to see whether this request has been pre cached
			if(!find){
				if(cache->sector<=sector && cache->nsect>=nsect){
					find = true;
				}
			}
		}
		// cache is the last  pre cache now
	}	
	

	if(!find ){
		if(dev->pre_cache_count<=MAX_CACHE){//if pre cache list is not full
			dev->pre_cache_count++;
		}else{//delete the last entry from list
			if(cache!=NULL){//in case 
				list_del_init(&cache->queuelist);//delete candidate pre cache from list
				sbd_free_cache(cache);
			}
		}
		list_add_tail(&tmp->queuelist,&dev->pre_cache_list);
	}
	spin_unlock_irq(&dev->pre_cache_lock);
	wake_up(&dev->pre_cache_wait_queue);

	if(find){//if this request already in pre cache list
		sbd_free_cache(tmp);
	}
	return false;
}

static void sbd_transfer(struct sbd_device *dev,unsigned long sector,unsigned long nsect,void * buffer,int write,bool cached)
{
	unsigned long offset = sector*KERNEL_SECTOR_SIZE;
	unsigned long nbytes = nsect * KERNEL_SECTOR_SIZE;

    	if (offset + nbytes> dev->size) {
		printk("sbd(total:%ld):Beyond-end write (%ld %ld)\n",dev->size,offset,nbytes);
		//return;
    	}

	if(cached){
        	sbd_get_cache_before_transfer(dev,sector,nsect,buffer,write);
		/*
        	if(sbd_get_cache_before_transfer(dev,sector,nsect,buffer,write)){
			return ;
		}
		*/
	}

    	if(write){
		http_set_block(IP,PORT,"upload.php","192.168.75.1",offset,nbytes,buffer);
    	} else {
		http_get_block(IP,PORT,"console.img","192.168.75.1",offset,nbytes,buffer);
	}
}

static int sbd_clear_queue(struct sbd_device *dev)
{
	return 0;
    	struct  sbd_cache *cache = NULL;
	spin_lock_irq(&dev->pre_cache_lock);
	while(!list_empty(&dev->pre_cache_list)){
		cache = list_entry(dev->pre_cache_list.next,struct sbd_cache,queuelist);
		list_del_init(&cache->queuelist);
		sbd_free_cache(cache);
	}
	dev->pre_cache_count = 0;
	spin_unlock_irq(&dev->pre_cache_lock);
	wake_up(&dev->pre_cache_wait_queue);

	spin_lock_irq(&dev->cache_lock);
	while(!list_empty(&dev->cache_list)){
		cache = list_entry(dev->cache_list.next,struct sbd_cache,queuelist);
		list_del_init(&cache->queuelist);
		sbd_free_cache(cache);
	}
	dev->cache_count = 0;
	spin_unlock_irq(&dev->cache_lock);

	return 0;
}





static int sbd_cache_thread(void *data)
{
	struct sbd_device *dev = data;
	set_user_nice(current,-20);
	allow_signal(SIGKILL);
	while(!kthread_should_stop()){
    		struct sbd_cache *cache = NULL;

		wait_event_interruptible(dev->pre_cache_wait_queue,kthread_should_stop()||!list_empty(&dev->pre_cache_list));
		
		if(signal_pending(current))
			break;

		spin_lock(&dev->pre_cache_lock);
		if(list_empty(&dev->pre_cache_list)){
			spin_unlock(&dev->pre_cache_lock);
        		schedule_timeout(HZ);
			continue;
		}
		cache = list_entry(dev->pre_cache_list.next,struct sbd_cache,queuelist);
		list_del(&cache->queuelist);
		dev->pre_cache_count--;
		spin_unlock(&dev->pre_cache_lock);
		
		if(cache==NULL){//in case
        		schedule_timeout(HZ);
			continue;
		}
		//printk("sbd_cache_thread says : offset is %d length is %d\n",(int)cache->sector * KERNEL_SECTOR_SIZE,(int)cache->nsect*KERNEL_SECTOR_SIZE);
		//sbd_transfer(dev,cache->sector,cache->nsect,cache->buffer,cache->write,false);
		
		spin_lock(&dev->cache_lock);
		if(dev->cache_count>=MAX_CACHE){
			struct sbd_cache *tmp;
			tmp = list_entry(dev->cache_list.next,struct sbd_cache,queuelist);
			list_del(&tmp->queuelist);
			sbd_free_cache(tmp);
		}else{
			dev->cache_count++;
		}
		list_add_tail(&cache->queuelist,&dev->cache_list);
		spin_unlock(&dev->cache_lock);
    	} 
    	return 0;	
}
		
static void sbd_request(request_queue_t *q)
{
    	struct request *req;
	struct sbd_device *dev ;
	int write;
    	while ((req = elv_next_request(q)) != NULL) {
			
		if (!blk_fs_request(req)) {
			spin_unlock_irq(q->queue_lock);	
	    		end_request(req, 0);
			spin_lock_irq(q->queue_lock);	
	    		continue;
		}

		dev = req->rq_disk->private_data;
		write = rq_data_dir(req);	
		dev->number_of_request++;
		if(write){
			dev->number_of_write_request++;
		}else{
			dev->number_of_read_request++;
		}

		spin_unlock_irq(q->queue_lock);	

		sbd_transfer(dev,req->sector,req->nr_sectors,req->buffer,write,true);
		end_request(req, 1);
		wake_up(&dev->pre_cache_wait_queue);
		printk("sbd_request says : totalreq is %d readreq is %d hit is %d cache_queue is %d\n",dev->number_of_request,dev->number_of_read_request,dev->number_of_hit,dev->cache_count);

		spin_lock_irq(q->queue_lock);	
    }
}

int sbd_ioctl (struct inode *inode, struct file *filp,unsigned int cmd, unsigned long arg)
{
	long size;
	struct hd_geometry geo;

	switch(cmd) {
	    case HDIO_GETGEO:
		size = dev.size/KERNEL_SECTOR_SIZE;
		geo.cylinders = (size & ~0x3f) >> 6;
		geo.heads = 4;
		geo.sectors = 16;
		geo.start = 4;
		if (copy_to_user((void *) arg, &geo, sizeof(geo)))
			return -EFAULT;
		return 0;
    }
    return -ENOTTY; /* unknown command */
}

static struct block_device_operations sbd_ops = {
    .owner           = THIS_MODULE,
    .ioctl	     = sbd_ioctl
};

static int __init sbd_init(void)
{
    	int devsize,i;
	struct request_queue *Queue;

 	i = 0 ;
   	printk("---------------------------\n");
    	socket_init();
    	devsize = http_head_info(IP,PORT,"console.img","192.168.75.1" );
    	printk("get block size :%d \n",devsize);

	for(i=0; i<MAX_SBD_CACHE_THREAD; i++)
	{
    		task[i] = NULL;
	}
	
	for(i=0; i<MAX_SBD_CACHE_THREAD; i++)
	{
    		task[i] = kthread_create(sbd_cache_thread,&dev,"sbd");
    		if(IS_ERR(task[i])){
        		printk("create thread error\n");
			return -ENOMEM;
    		}
	}

    	if(devsize<=0){
        	dev.size = nsectors*hardsect_size;
    	}else{
        	dev.size = devsize;
        	nsectors = devsize/hardsect_size;
    	}
    	spin_lock_init(&dev.lock);

    	Queue = blk_init_queue(sbd_request, &dev.lock);
    	if (Queue == NULL)
	    	goto out;
    	blk_queue_hardsect_size(Queue, hardsect_size);

    	major_num = register_blkdev(major_num, "sbd");
    	if (major_num <= 0) {
		printk(KERN_WARNING "sbd: unable to get major number\n");
		goto out;
    	}

    	dev.gd = alloc_disk(1);
    	if (! dev.gd)
		goto out_unregister;

	spin_lock_init(&dev.pre_cache_lock);
	INIT_LIST_HEAD(&dev.pre_cache_list);
	init_waitqueue_head(&dev.pre_cache_wait_queue);
	dev.pre_cache_count = 0;


	spin_lock_init(&dev.cache_lock);
	INIT_LIST_HEAD(&dev.cache_list);
	init_waitqueue_head(&dev.cache_wait_queue);
	dev.cache_count = 0;

	dev.number_of_error = 0;
	dev.number_of_write_request = 0;
	dev.number_of_read_request = 0;
	dev.number_of_request = 0;
	

    	dev.gd->major = major_num;
    	dev.gd->first_minor = 0;
    	dev.gd->fops = &sbd_ops;
    	dev.gd->private_data = &dev;
    	strcpy (dev.gd->disk_name, "sbd0");
    	set_capacity(dev.gd,nsectors*(hardsect_size/KERNEL_SECTOR_SIZE) );
    	dev.gd->queue = Queue;

	for(i =0 ;i<MAX_SBD_CACHE_THREAD;i++)
	{
    		wake_up_process(task[i]);
	}

    	add_disk(dev.gd);
    	return 0;

  out_unregister:
    	unregister_blkdev(major_num, "sbd");
  out:
    	return -ENOMEM;
}

static void __exit sbd_exit(void)
{
	int i;
	struct request_queue *Queue = dev.gd->queue;
	for(i=0; i<MAX_SBD_CACHE_THREAD; i++)
	{
		if(task[i]){
			force_sig(SIGKILL,task[i]);
		}
	}
	sbd_clear_queue(&dev);
    	del_gendisk(dev.gd);
    	put_disk(dev.gd);
    	unregister_blkdev(major_num, "sbd");
    	blk_cleanup_queue(Queue);
}
	
module_init(sbd_init);
module_exit(sbd_exit);
