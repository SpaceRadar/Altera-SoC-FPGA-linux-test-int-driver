#include<linux/module.h> /*Needed by all modules*/
#include<linux/kernel.h> /*Needed for KERN_INFO*/
#include<linux/init.h>   /*Needed for the macros*/
#include<linux/interrupt.h>
#include<linux/sched.h>
#include<linux/platform_device.h>
#include<linux/io.h>
#include<linux/of.h>
#include<linux/miscdevice.h>
#include<linux/fs.h>
#include<linux/uaccess.h>
#include<linux/slab.h>
#include<linux/dma-mapping.h>


#define DEVNAME                     "test_int"
#define DEV_FILE_NAME_MAX_LENGTH    (sizeof(DEVNAME)+5)

struct device_data_t
{
    wait_queue_head_t    queue;
    int                  sign;
    struct semaphore     ready;
    struct miscdevice    dev;
    char                 file_name[DEV_FILE_NAME_MAX_LENGTH];   
    int*                 data; 
    int*                 regs;
    volatile long unsigned int                  ready_atomic;
    struct completion    done;
};

unsigned long a;

static int mopen_open( struct inode *n, struct file *f ) 
{
   return 0;
}

ssize_t read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    void* regs;
    struct device_data_t* device_data;
    //size_t rd_count=min(count,sizeof(mess)-1);
    device_data=container_of(filp->private_data,struct device_data_t, dev);
	printk(KERN_INFO DEVNAME":device_data dev pointer %x\n", (int)filp->private_data);


    regs=ioremap_nocache(0xFF200004,64);
    iowrite32(count,regs);
    iounmap(regs);

    wait_event_interruptible(device_data->queue,device_data->sign==1);
    device_data->sign=0;


    if(copy_to_user(buf,device_data->data,count*2))
        return -EFAULT;


    return count;
}



ssize_t write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
//    u32 res,timer,timer_to_irq,total_timer;
    u32 result[4];
    unsigned long loop;
    struct device_data_t* device_data;
    device_data=container_of(filp->private_data,struct device_data_t, dev);
//	printk(KERN_INFO DEVNAME":device_data dev pointer %x\n", (int)filp->private_data);

    do{
        smp_mb__before_atomic(); 
        loop=test_and_set_bit(1,&device_data->ready_atomic);      
        smp_mb__after_atomic(); 
    }while(loop);

//    while(test_and_set_bit(1,&device_data->ready_atomic));
//    down(&device_data->ready);
    device_data->sign=0;
//	printk(KERN_INFO DEVNAME": using buffer\n");
    if(copy_from_user(device_data->data,buf,count))
    {
        up(&device_data->ready);         
        return -EFAULT;
    }

//    regs=ioremap_nocache(0xFF200004,64);
    iowrite32((count>>3)-1,device_data->regs+1);
//    iounmap(regs);

//    wait_event_interruptible(device_data->queue,device_data->sign==1);
    device_data->sign=0;
    wait_for_completion(&device_data->done);

    result[0]=ioread32(device_data->regs+0);
    result[1]=ioread32(device_data->regs+1);
    result[2]=ioread32(device_data->regs+2);
    result[3]=ioread32(device_data->regs+3);
    if(copy_to_user(buf,result,sizeof(result)))
    {
        up(&device_data->ready);         
        return -EFAULT;
    }
    
//    up(&device_data->ready);  
    
    smp_mb__before_atomic(); 
    clear_bit(1,&device_data->ready_atomic);      
    smp_mb__after_atomic(); 

//	printk(KERN_INFO DEVNAME": result %d timer %d timer_to_irq %d total_timer %u\n",res,timer,timer_to_irq,total_timer);
//	printk(KERN_INFO DEVNAME": unlock sema\n");
    return result[0];
}

static const struct file_operations fops = {
   .owner  = THIS_MODULE,
   .open =    mopen_open,
//   .release = mopen_release,
   .read   =  read,
   .write  =  write,
};

irqreturn_t __test_isr(int irq, void* dev_id)
{
    struct device_data_t* device_data=(struct device_data_t*) dev_id;    
    device_data->sign=1; 
    wake_up_interruptible(&device_data->queue);
    complete(&device_data->done);
//	printk(KERN_INFO DEVNAME":ISR %d\n",irq);
	return IRQ_HANDLED;
}

static int create_file_name(char* file_name, char* pattern, int idx, unsigned long max_file_name)
{
    while(*pattern) *file_name++=*pattern++;
    if(idx>999)
        return -1;
    if(idx>99)
    {  
        *file_name++=(idx/100)+'0';
        idx%=100;
    }
    if(idx>9)
    {  
        *file_name++=(idx/10)+'0';
        idx%=10;
    }
    *file_name++=idx+'0';
    *file_name='\0';
    return 0;
}


static int __test_int_driver_probe(struct platform_device* pdev)
{
	int irq_num,ret;
    dma_addr_t dma_addr;
    struct device_data_t* device_data;

  	printk(KERN_ERR DEVNAME":crc driver version %s- %s\n", __DATE__, __TIME__); 

	irq_num=platform_get_irq(pdev,0);
	printk(KERN_INFO DEVNAME":IRQ %d about to be registered!\n", irq_num);

	device_data = devm_kzalloc(&pdev->dev, sizeof(struct device_data_t),GFP_KERNEL);
    if(!device_data)
    {
     	printk(KERN_ERR DEVNAME":Can not allocate mem for service buffer for irq %d\n", irq_num); 
        return -ENOMEM;
    }
	printk(KERN_INFO DEVNAME":Service buffer memory is allocated for irq %d\n", irq_num); 


    device_data->dev.minor=MISC_DYNAMIC_MINOR;
    device_data->dev.fops=&fops;
    device_data->dev.mode=0666;

    create_file_name(device_data->file_name,"test_int_",irq_num,0);
    device_data->dev.name=device_data->file_name;
    device_data->sign=0;
    init_waitqueue_head(&device_data->queue);
    init_completion(&device_data->done);
    sema_init(&device_data->ready,1);
    device_data->ready_atomic=0;

    ret = misc_register(&device_data->dev); 
    if(ret)
    {
     	printk(KERN_ERR DEVNAME"Can not register character file for irq %d\n", irq_num); 
        return ret;       
    }
   	printk(KERN_INFO DEVNAME":Character file is registered for irq %d\n", irq_num);


    device_data->data=dmam_alloc_coherent(&pdev->dev,4096,&dma_addr,GFP_KERNEL);
    if(!device_data->data)
    {
     	printk(KERN_ERR DEVNAME":Can not allocate mem for buffer for irq %d\n", irq_num); 
        ret=-ENOMEM;
        goto misc_deregister_label;         
    }
	printk(KERN_INFO DEVNAME":Buffer memory is allocated for irq %d\n", irq_num); 


    ret=devm_request_irq(&pdev->dev,irq_num,__test_isr,0,DEVNAME,device_data);    
    if(ret)
    {
     	printk(KERN_ERR DEVNAME":Can not register irq %d\n", irq_num); 
        goto misc_deregister_label; 
    }
	printk(KERN_INFO DEVNAME":Handler is registered for irq %d\n",irq_num);   


    device_data->regs=ioremap(0xFF200000,32);
    if(!device_data->regs)
    {
     	printk(KERN_ERR DEVNAME":Can not remap registers for irq %d\n", irq_num); 
        ret=-ENXIO;
        goto misc_deregister_label;         
    }
	printk(KERN_INFO DEVNAME":Registers is remapped for irq %d\n",irq_num); 
 
    iowrite32(dma_addr,device_data->regs);

    platform_set_drvdata(pdev,device_data); 
	return 0;

misc_deregister_label:
    misc_deregister(&device_data->dev);  
    return ret;
}

static int __test_int_driver_remove(struct platform_device* pdev)
{
	int irq_num;
    struct device_data_t* device_data;
    device_data=platform_get_drvdata(pdev); 
	printk(KERN_INFO DEVNAME"remove :device_data pointer %x\n", (int)device_data);
	irq_num=platform_get_irq(pdev,0);
    misc_deregister(&device_data->dev);    
//    kfree(device_data);
    iounmap(device_data->regs);
	printk(KERN_INFO"test_int:IRQ %d about to be freed!\n",irq_num);
//	free_irq(irq_num,NULL);
    platform_set_drvdata(pdev,NULL); 
	return 0;
}

static const struct of_device_id __test_int_driver_id[]=
{
	{.compatible="altr ,socfpga-mysoftip"},
	{}
};

static struct platform_driver __test_int_driver=
{
	.driver=
	{
		.name=DEVNAME,
		.owner=THIS_MODULE,
		.of_match_table=of_match_ptr(__test_int_driver_id),
	},
	.probe=__test_int_driver_probe,
	.remove=__test_int_driver_remove
};

module_platform_driver(__test_int_driver);
MODULE_LICENSE("GPL");
