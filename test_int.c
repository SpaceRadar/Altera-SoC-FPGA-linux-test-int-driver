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

#define DEVNAME                     "test_int"
#define DEV_FILE_NAME_MAX_LENGTH    (sizeof(DEVNAME)+5)

struct device_data_t
{
    wait_queue_head_t queue;
    int               sign; 
    struct miscdevice dev;
    char              file_name[DEV_FILE_NAME_MAX_LENGTH];   
};

static int mopen_open( struct inode *n, struct file *f ) 
{
   return 0;
}

ssize_t null_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    char mess[]="hello world\n";
    struct device_data_t* device_data;
    size_t rd_count=min(count,sizeof(mess)-1);
    device_data=container_of(filp->private_data,struct device_data_t, dev);
	printk(KERN_INFO DEVNAME":device_data dev pointer %x\n", (int)filp->private_data);
    copy_to_user(buf,mess,sizeof(mess)-1);
    printk(KERN_ALERT"%d bytes asked to read\n", (int)count);
    if(!*f_pos)
    {
        wait_event_interruptible(device_data->queue,device_data->sign==1);
        device_data->sign=0;
        printk(KERN_ALERT"%d asked bytes readed\n", (int)count);
        *f_pos+=rd_count;
    	return rd_count;
    }
    else
        return 0;
}


static const struct file_operations fops = {
   .owner  = THIS_MODULE,
   .open =    mopen_open,
//   .release = mopen_release,
   .read   =  null_read,
//   .write  =  mopen_write,
};

irqreturn_t __test_isr(int irq, void* dev_id)
{
    struct device_data_t* device_data=(struct device_data_t*) dev_id;    
	printk(KERN_INFO DEVNAME":ISR %d\n",irq);
    device_data->sign=1; 
    wake_up_interruptible(&device_data->queue);
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
    struct device_data_t* device_data;

	device_data = devm_kzalloc(&pdev->dev, sizeof(struct device_data_t),GFP_KERNEL);
	irq_num=platform_get_irq(pdev,0);
    device_data->dev.minor=MISC_DYNAMIC_MINOR;
    device_data->dev.fops=&fops;
    device_data->dev.mode=0666;

    create_file_name(device_data->file_name,"test_int_",irq_num,0);
    device_data->dev.name=device_data->file_name;
    device_data->sign=0;
    init_waitqueue_head(&device_data->queue);
    ret = misc_register(&device_data->dev); 
    platform_set_drvdata(pdev,device_data); 

	printk(KERN_INFO DEVNAME":IRQ %d about to be registered!\n", irq_num);
	printk(KERN_INFO DEVNAME":device_data pointer %x -dev %x\n", (int)device_data, (int)&device_data->dev);
    ret=devm_request_irq(&pdev->dev,irq_num,__test_isr,0,DEVNAME,device_data);
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

