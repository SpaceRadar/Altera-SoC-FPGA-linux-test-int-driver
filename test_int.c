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
/*
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
*/
    void* regs;
    regs=ioremap_nocache(0xC000FFF0,64);
    iowrite32(0,regs);
    iounmap(regs);
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



void show_resource(struct platform_device *dev)
{
	int i;
    printk(KERN_INFO DEVNAME":device total resources %d\n", dev->num_resources);
	for (i = 0; i < dev->num_resources; i++) 
    {
		struct resource *r = &dev->resource[i];
		printk(KERN_INFO DEVNAME":resource %d -type %x -name %s\n", i,resource_type(r),r->name);
	}
}

static int __test_int_driver_probe(struct platform_device* pdev)
{
	int irq_num,ret;
    void* ptr;
    unsigned int in_data_addr,out_data_addr;
    struct device_data_t* device_data;
    struct resource* res=NULL;
    struct device_node* nod;

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

    nod=of_get_parent(pdev->dev.of_node);
    if(nod)
    	printk(KERN_INFO DEVNAME": name %s parent name %s\n", pdev->dev.of_node->name, nod->name);


    show_resource(pdev);    

    ptr=of_get_property(pdev->dev.of_node,"in_data_addr",NULL);
    if(ptr)
    {
        in_data_addr=be32_to_cpup(ptr);
     	printk(KERN_INFO DEVNAME":device resources in %x\n", in_data_addr);
    }
    else
           	printk(KERN_INFO DEVNAME":device resources in not found\n");
    ptr=of_get_property(pdev->dev.of_node,"out_data_addr",NULL);
    if(ptr)
    {
        out_data_addr=be32_to_cpup(ptr);
      	printk(KERN_INFO DEVNAME":device resources out %x\n", out_data_addr);
    }
    else
        printk(KERN_INFO DEVNAME":device resources out not found\n");

    int reg;
    if(!of_property_read_u32(pdev->dev.of_node,"in_data_addr",&reg))
       printk(KERN_INFO DEVNAME":device resources id data new %x\n",reg);    


    char* buff;
    if(!of_property_read_string(pdev->dev.of_node,"status",&buff))
       printk(KERN_INFO DEVNAME":device resources id status new %s\n",buff); 

    int regg[64];
    if(!of_property_read_u32_array(pdev->dev.of_node,"reg",&regg,1))
       printk(KERN_INFO DEVNAME":device resources id reg new %x %x %x %x\n",regg[0],regg[1],regg[2],regg[3]);
    else
       printk(KERN_INFO DEVNAME":array not found\n");


    int regs[64];
    if(!of_property_read_u32_array(pdev->dev.of_node,"reg",&regg,4))
       printk(KERN_INFO DEVNAME":device resources id reg new %x %x %x %x\n",regg[0],regg[1],regg[2],regg[3]);
    else
       printk(KERN_INFO DEVNAME":array not found\n");


    res=platform_get_resource_byname(pdev,IORESOURCE_MEM,"<b>csr</b>");
    if(res)
    {
      	printk(KERN_INFO DEVNAME":device resources by name csr %d %d\n", res->start,resource_size(res));
        int* p;
        p=devm_ioremap_resource(&pdev->dev,res);
        if(!IS_ERR(p))
          	printk(KERN_INFO DEVNAME":device resources by name csr val%d\n", *p);
        else
          	printk(KERN_INFO DEVNAME":device resources can not remap\n");
    }
    else
        printk(KERN_INFO DEVNAME":device resources by name csr not found\n");
    

    res=platform_get_resource_byname(pdev,IORESOURCE_MEM,"<b>data</b>");
    if(res)
    {
      	printk(KERN_INFO DEVNAME":device resources by name data %d %d\n", res->start,resource_size(res));
        int* p;
        p=devm_ioremap_resource(&pdev->dev,res);
        if(!IS_ERR(p))
          	printk(KERN_INFO DEVNAME":device resources by name data val%d\n", *p);
        else
          	printk(KERN_INFO DEVNAME":device resources can not remap\n");
    }
    else
        printk(KERN_INFO DEVNAME":device resources by name data not found\n");






/*

    res=platform_get_resource(pdev,IORESOURCE_MEM,0);
    if(res)
    {
      	printk(KERN_INFO DEVNAME":device resources by id %d %d %d\n", res->start,resource_size(res),*((int*)res->start));
    }
    else
        printk(KERN_INFO DEVNAME":device resources by id not found\n");
*/






//	printk(KERN_INFO DEVNAME":device resources in %x -out %x\n", in_data_addr,out_data_addr);
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

