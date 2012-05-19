/*
 *  mma8452.c - Linux kernel modules for 3-Axis Orientation/Motion
 *  Detection Sensor 
 *
 *  Copyright (C) 2009-2010 Freescale Semiconductor Hong Kong Ltd.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/hwmon.h>
#include <linux/miscdevice.h>
#include "jz_sensor_mma8452.h"
#include <linux/input-polldev.h>
#include <linux/input.h>
#include <linux/earlysuspend.h>
#include <asm/jzsoc.h>
#include <linux/linux_sensors.h>



/*
 * Defines
 */

#if 1
#define assert(expr)\
        if(!(expr)) {\
        printk( "Assertion failed! %s,%d,%s,%s\n",\
        __FILE__,__LINE__,__func__,#expr);\
        }
#else
#define assert(expr) do{} while(0)
#endif

#define MMA8452_DRV_NAME	"mma8452"
#define MMA8452_I2C_ADDR	0x1C
#define MMA8452_ID		0x2A

#define POLL_INTERVAL		100
#define INPUT_FUZZ	32
#define INPUT_FLAT	32

#define MODE_CHANGE_DELAY_MS 100

#define I2C_RETRY_DELAY		5
#define I2C_RETRIES		5
/* mma8452 status */
struct mma8452_status {
	u8 mode;
	u8 ctl_reg1;
};

static struct mma8452_status mma_status = {
	.mode 		= 0,
	.ctl_reg1	= 0
};
static struct {
	unsigned int cutoff;
	unsigned int mask;
} odr_table[] = {
	{
	3,	MMA8452_ODR400  },{
	10,	MMA8452_ODR200  },{ 
	20,	MMA8452_ODR100  },{ 
	100,	MMA8452_ODR50   },{ 
	300,	MMA8452_ODR12   },{
	500,	MMA8452_ODR6   },{
	1000,	MMA8452_ODR1    },
};
static int test_test_mode = 1;
static struct i2c_client *mma8452_i2c_client;
//------------------------------------
static struct workqueue_struct *mma_wqueue;
struct mma8452_data {
	struct mma8452_platform_data *pdata;
	struct mutex lock;
	atomic_t enabled;
	int enabled_save;
	int is_suspend;
	int delay_save;
	struct delayed_work mma_work;
	struct input_dev *input_dev;
	struct work_struct pen_event_work;
	struct early_suspend early_suspend;
		
};
struct mma8452_data *mma8452_misc_data;
//-------------------------------------
static int mma8452_i2c_read(u8* reg,u8 *buf, int len)
{
	int err;
	struct i2c_msg msgs[] = {
		{
		 .addr = mma8452_i2c_client->addr,
		 .flags =  0 | I2C_M_NOSTART,
		 .len = 1,
		 .buf = reg,
		 },
		{
		 .addr = mma8452_i2c_client->addr,
		 .flags = I2C_M_RD | I2C_M_NOSTART,
		 .len = len,
		 .buf = buf,
		 },
	};

	err = i2c_transfer(mma8452_i2c_client->adapter, msgs, 2);
	if(err < 0){
		printk("read msg error\n");
	}
	return  0;
}

static int mma8452_i2c_write(u8 *buf, int len)
{
	int err;
	struct i2c_msg msgs[] = {
		{
		 .addr = mma8452_i2c_client->addr,
		 .flags = 0 | I2C_M_NOSTART,
		 .len = len,
		 .buf = buf,
		 },
	};

	err = i2c_transfer(mma8452_i2c_client->adapter, msgs, 1);
	if(err < 0){
		printk("read msg error\n");
	}
	return  0;
}

static int mma8452_i2c_read_data(u8 reg,u8* rxData, int length)
{
	int ret;

	ret = mma8452_i2c_read(&reg, rxData, length);
	if (ret < 0)
		return ret;
	else
		return 0;
}

static int mma8452_i2c_write_data(u8 reg,char *txData, int length)
{
	char buf[80];
	int ret;

	buf[0] = reg;
	memcpy(buf+1, txData, length);
	ret = mma8452_i2c_write(buf, length+1);
	if (ret < 0)
		return ret;
	else {
		return 0;
	}
}
//----------------------------------
/*
 * Initialization function
 */

static int mma8452_init_client(struct i2c_client *client)
{
	u8 buf[3]={0,0,0};
	if(test_test_mode)
	{
		buf[0] = 0x0;
		mma8452_i2c_write_data(MMA8452_CTRL_REG1,buf,1);
		buf[0] = 0x0;
		mma8452_i2c_write_data(MMA8452_XYZ_DATA_CFG,buf,1);
		buf[0] = 0xff;
		mma8452_i2c_write_data(MMA8452_CTRL_REG5,buf,1);
		buf[0] = 0x01;
		mma8452_i2c_write_data(MMA8452_CTRL_REG4,buf,1);
		buf[0] = 0x01;
		mma8452_i2c_write_data(MMA8452_CTRL_REG3,buf,1);
		buf[0] = 0x00;
		mma8452_i2c_write_data(MMA8452_CTRL_REG2,buf,1);
		buf[0] = 0x21;
		mma8452_i2c_write_data(MMA8452_CTRL_REG1,buf,1);
	}

	mdelay(MODE_CHANGE_DELAY_MS);
	return 0;
}
/***************************************************************
*
* read sensor data from mma8452
*
***************************************************************/ 				
static int mma8452_set_delay(int delay);


static int mma8452_read_data(short *x, short *y, short *z) {
	u8	tmp_data[7];
	u8 buf[3]={0,0,0};
	int hw_d[3] ={0};
	struct mma8452_data *mma = i2c_get_clientdata(mma8452_i2c_client);

	if (mma8452_i2c_read_data(MMA8452_OUT_X_MSB,tmp_data,7) < 0) {
		printk("i2c block read failed\n");
			return -3;
	}

	hw_d[0] = ((tmp_data[0] << 8) & 0xff00) | tmp_data[1];
	hw_d[1] = ((tmp_data[2] << 8) & 0xff00) | tmp_data[3];
	hw_d[2] = ((tmp_data[4] << 8) & 0xff00) | tmp_data[5];

	hw_d[0] = (short)(hw_d[0]) >> 4;
	hw_d[1] = (short)(hw_d[1]) >> 4;
	hw_d[2] = (short)(hw_d[2]) >> 4;
	
	if (mma_status.mode==MMA8452_4G){
		hw_d[0] = (short)(hw_d[0]) <<1;
		hw_d[1] = (short)(hw_d[1]) <<1;
		hw_d[2] = (short)(hw_d[2]) <<1;
	}
	else if (mma_status.mode==MMA8452_8G){
		hw_d[0] = (short)(hw_d[0])<<2;
		hw_d[1] = (short)(hw_d[1])<<2;
		hw_d[2] = (short)(hw_d[2])<<2;
	}
	*x = ((mma->pdata->negate_x) ? (-hw_d[mma->pdata->axis_map_x])
			: (hw_d[mma->pdata->axis_map_x]));
	*y = ((mma->pdata->negate_y) ? (-hw_d[mma->pdata->axis_map_y])
			: (hw_d[mma->pdata->axis_map_y]));
	*z = ((mma->pdata->negate_z) ? (-hw_d[mma->pdata->axis_map_z])
			: (hw_d[mma->pdata->axis_map_z]));

	mma8452_i2c_read_data(MMA8452_INT_SOURCE,buf,1);
	if(buf[0] == 1){
		mma8452_set_delay(mma->delay_save);
	}
	return 0;
}
#ifdef CONFIG_SENSORS_JZ_MMA8452_ORI
extern void orientation_report_values(int x,int y,int z);
#endif
static void report_abs(void)
{
	short 	x,y,z;
	u8 buf[3]={0,0,0};
	struct mma8452_data *data = i2c_get_clientdata(mma8452_i2c_client);
	
	mma8452_i2c_read_data(MMA8452_STATUS,buf,1);
	if(!(buf[0] & 0x01)){
		return;	
	}

	if (mma8452_read_data(&x,&y,&z) != 0) {
		return;
	}
	
	input_report_abs(data->input_dev, ABS_X, x);
	input_report_abs(data->input_dev, ABS_Y, y);
	input_report_abs(data->input_dev, ABS_Z, z);
	//printk("x:%d y:%d z:%d\n",x,y,z);
	input_sync(data->input_dev);
#ifdef CONFIG_SENSORS_JZ_MMA8452_ORI
orientation_report_values(x,y,z);
#endif
}
struct linux_sensor_t hardware_data = {                                       
	"mma8452 3-axis Accelerometer",
	"ST sensor",
	SENSOR_TYPE_ACCELEROMETER,0,1024,1, 1, { } 
};      

static int mma8452_device_power_off(struct mma8452_data *mma)
{
	int result;
	u8 buf[3]={0,0,0}; 
	mma8452_i2c_read_data(MMA8452_CTRL_REG1,buf,1);
	mma_status.ctl_reg1 = buf[0];
	mma_status.ctl_reg1 &=0xFE;

	buf[0] = 0x0;
	mma8452_i2c_write_data(MMA8452_CTRL_REG1,buf,1);
	buf[0] = mma_status.ctl_reg1;
	result = mma8452_i2c_write_data(MMA8452_CTRL_REG1,buf,1);
	if(result < 0){
		printk("POWER OFF ERROR\n");
		return result;
	}
	return 0;
}

static int mma8452_device_power_on(struct mma8452_data *mma)
{
	int result;
	u8 buf[3] = {0,0,0};
	mma8452_i2c_read_data(MMA8452_CTRL_REG1,buf,1);
	mma_status.ctl_reg1 = buf[0];
	mma_status.ctl_reg1|=0x01;

	buf[0] = 0x0;
	mma8452_i2c_write_data(MMA8452_CTRL_REG1,buf,1);
	udelay(100);
	buf[0] = mma_status.ctl_reg1;
	result = mma8452_i2c_write_data(MMA8452_CTRL_REG1,buf,1);
	if(result < 0){
		printk("POWER ON ERROR\n");
		return result;
	}
	return 0;
}
static int mma8452_enable(struct mma8452_data *mma)
{
	int result;
	if(mma->is_suspend == 0 && !atomic_cmpxchg(&mma->enabled,0,1)){
		result=mma8452_device_power_on(mma);
		if(result < 0){
			printk("ENABLE ERROR\n");
			atomic_set(&mma->enabled,0);
			return result;
		}
    		enable_irq(mma8452_i2c_client->irq);
	}
	return 0;
}

static int mma8452_disable(struct mma8452_data *mma)
{
	int result;
	if(atomic_cmpxchg(&mma->enabled,1,0)){
		disable_irq_nosync(mma8452_i2c_client->irq);
		flush_workqueue(mma_wqueue);
		cancel_delayed_work_sync(&mma->mma_work);
		result=mma8452_device_power_off(mma);
		if(result < 0){
			printk("DISABLE ERROR\n");
			atomic_set(&mma->enabled,1);
			return result;
		}
	}
	return 0;
}
static int mma8452_set_delay(int delay)
{
	int result;
	int i;
	struct mma8452_data *mma = i2c_get_clientdata(mma8452_i2c_client);
	u8 buf[3] = {0,0,0};
	mma->delay_save = delay;

	for(i = 0;i < ARRAY_SIZE(odr_table);i++){
		mma8452_i2c_read_data(MMA8452_CTRL_REG1,buf,1);
		mma_status.ctl_reg1 = buf[0];
		mma_status.ctl_reg1 &=0xc7;
		mma_status.ctl_reg1 |= odr_table[i].mask;
		if(delay < odr_table[i].cutoff){
			break;	
		}
	}
	buf[0] = 0x0;
	mma8452_i2c_write_data(MMA8452_CTRL_REG1,buf,1);
	buf[0] = mma_status.ctl_reg1;
	result = mma8452_i2c_write_data(MMA8452_CTRL_REG1,buf,1);
	assert(result==0);
	return result;
}
int mma8452_input_open(struct input_dev *input)
{
	struct mma8452_data *mma = i2c_get_clientdata(mma8452_i2c_client);
	return mma8452_enable(mma);
}
int mma8452_input_close(struct input_dev *input)
{
	struct mma8452_data *mma = i2c_get_clientdata(mma8452_i2c_client);
	return mma8452_disable(mma);
}
static int mma8452_misc_open(struct inode *inode, struct file *file)
{       
	int err;
	err = nonseekable_open(inode, file);
	if (err < 0)
		return err;
	return 0;
}

long mma8452_misc_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	int interval;
	struct mma8452_data *mma = i2c_get_clientdata(mma8452_i2c_client);

	switch (cmd) {
	case SENSOR_IOCTL_GET_DELAY:
		interval = mma->pdata->poll_interval;
		if (copy_to_user(argp, &interval, sizeof(interval)))
			return -EFAULT;
		break;
	case SENSOR_IOCTL_SET_DELAY:
		if (copy_from_user(&interval, argp, sizeof(interval)))
			return -EFAULT;
		interval *= 10;  //for Sensor_new
		if (interval < mma->pdata->min_interval )
			interval = mma->pdata->min_interval;
		else if (interval > mma->pdata->max_interval)
			interval = mma->pdata->max_interval;
		mma->pdata->poll_interval = interval;
		mma8452_set_delay(mma->pdata->poll_interval);
		break;
	case SENSOR_IOCTL_SET_ACTIVE:
		if (copy_from_user(&interval, argp, sizeof(interval)))
			return -EFAULT;
		if (interval > 1)
			return -EINVAL;
		if (interval){
			mma8452_enable(mma);
		}else
			mma8452_disable(mma);
		break;
	case SENSOR_IOCTL_GET_ACTIVE:
		interval = atomic_read(&mma->enabled);
		if (copy_to_user(argp, &interval, sizeof(interval)))
			return -EINVAL;

		break;	
	case SENSOR_IOCTL_GET_DATA:
		if (copy_to_user(argp, &hardware_data, sizeof(hardware_data)))
			return -EINVAL;
		break;
	case SENSOR_IOCTL_GET_DATA_MAXRANGE:
		if (copy_to_user(argp, &mma->pdata->g_range, sizeof(mma->pdata->g_range)))
			return -EFAULT;
		break;

	case SENSOR_IOCTL_WAKE:
		input_event(mma->input_dev, EV_SYN,SYN_CONFIG, 0);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}
static void mma8452_work(struct work_struct *work)
{
	report_abs();
    	enable_irq(mma8452_i2c_client->irq);
}

static irqreturn_t mma8452_interrupt(int irq, void *dev_id)
{
	struct mma8452_data *mma = i2c_get_clientdata(mma8452_i2c_client);
	disable_irq_nosync(mma8452_i2c_client->irq);
	if( mma->is_suspend == 1 ||atomic_read(&mma->enabled) == 0)
		return IRQ_HANDLED;
	__gpio_ack_irq(mma->pdata->intr);	
	if (!work_pending(&mma->pen_event_work)) {
		queue_work(mma_wqueue, &mma->pen_event_work);
	}
	return IRQ_HANDLED;
}

static const struct file_operations mma8452_misc_fops = {
	.owner = THIS_MODULE,
	.open = mma8452_misc_open,
	.unlocked_ioctl = mma8452_misc_ioctl,
};              

static struct miscdevice mma8452_misc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name =  "g_sensor",
	.fops = &mma8452_misc_fops,
};

/*
 * I2C init/probing/exit functions
 */

static void mma8452_suspend(struct early_suspend *h);
static void mma8452_resume(struct early_suspend *h);

static int __devinit mma8452_probe(struct i2c_client *client,
				   const struct i2c_device_id *id)
{
	struct mma8452_data *mma;
	u8 buf[3] = {0,0,0};
	int result = -1;

	if(!i2c_check_functionality(client->adapter, 
				I2C_FUNC_SMBUS_BYTE|I2C_FUNC_SMBUS_BYTE_DATA)){
		dev_err(&client->dev, "client not i2c capable\n"); 
		result = -ENODEV;
	       	goto err0;
	}

	mma = kzalloc(sizeof(*mma),GFP_KERNEL);
	if (mma == NULL) {
		dev_err(&client->dev,"failed to allocate memory for module data\n");
		result = -ENOMEM;
		goto err0;
	}

	mutex_init(&mma->lock);	
	mutex_lock(&mma->lock);	
	mma8452_i2c_client = client;

	mma->pdata = kmalloc(sizeof(*mma->pdata),GFP_KERNEL);
	if(mma->pdata == NULL)
		goto err1;
	memcpy(mma->pdata,client->dev.platform_data,sizeof(*mma->pdata));

	mma8452_device_power_off(mma);
	udelay(100);
	mma8452_device_power_on(mma);
	udelay(100);
	atomic_set(&mma->enabled,1);

	i2c_set_clientdata(client,mma);
//	i2c_jz_setclk(client, 100*1000);
	printk(KERN_INFO "check mma8452 chip ID\n");
	mma8452_i2c_read_data(MMA8452_WHO_AM_I,buf,1);
	if (MMA8452_ID != buf[0]) {	//compare the address value 
		dev_err(&client->dev,"read chip ID 0x%x is not equal to 0x%x!\n", result,MMA8452_ID);
		printk("read chip ID failed\n");
		result = -EINVAL;
		goto err2;
	}
	

	/* Initialize the MMA8452 chip */
	result = mma8452_init_client(client);
	assert(result==0);

	INIT_WORK(&mma->pen_event_work, mma8452_work);
	mma_wqueue = create_singlethread_workqueue("mma8452");
	if(!mma_wqueue){
		result = -ESRCH;
		goto err2;
	}


	result = request_irq(client->irq, mma8452_interrupt, IRQF_DISABLED,
			"mma8452", mma);
	if (result < 0) {
		printk(" request irq is error\n");
		dev_err(&client->dev, "%s rquest irq failed\n", __func__);
		return result;
	}

	/*input poll device register */
	mma->input_dev = input_allocate_device();
	if (!mma->input_dev) {
		dev_err(&client->dev, "alloc poll device failed!\n");
		printk("alloc poll device failed!\n");
		result = -ENOMEM;
		goto err3;
	}

//	mma->input_dev->open = mma8452_input_open;
//	mma->input_dev->close = mma8452_input_close;
	input_set_drvdata(mma->input_dev,mma);
	set_bit(EV_ABS,mma->input_dev->evbit);
	input_set_abs_params(mma->input_dev, ABS_X, -8192, 8191, INPUT_FUZZ, INPUT_FLAT);
	input_set_abs_params(mma->input_dev, ABS_Y, -8192, 8191, INPUT_FUZZ, INPUT_FLAT);
	input_set_abs_params(mma->input_dev, ABS_Z, -8192, 8191, INPUT_FUZZ, INPUT_FLAT);
	mma->input_dev->name="g_sensor";
	result = input_register_device(mma->input_dev);
	if (result) {
		dev_err(&client->dev, "register poll device failed!\n");
		printk("register poll device failed!\n");
		goto err4;
	}

	mma8452_misc_data = mma;
	result = misc_register(&mma8452_misc_device);
	if (result) {
		dev_err(&client->dev, "register misc device failed!\n");
		goto err5;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
        mma->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
        mma->early_suspend.suspend = mma8452_suspend;
        mma->early_suspend.resume = mma8452_resume;
        register_early_suspend(&mma->early_suspend);
#endif

	mma8452_device_power_off(mma);
	atomic_set(&mma->enabled,0);
	mma->is_suspend = 0;
	disable_irq_nosync(mma8452_i2c_client->irq);
	mutex_unlock(&mma->lock);
	__gpio_as_irq_fall_edge(mma->pdata->intr);
	return 0;
err5:
	input_unregister_device(mma->input_dev);
err4:
	input_free_device(mma->input_dev);
err3:
	mma8452_device_power_off(mma);
err2:
	mutex_unlock(&mma->lock);
	kfree(mma->pdata);
err1:
	kfree(mma);	
err0:
	return result;
}

static int __devexit mma8452_remove(struct i2c_client *client)
{
	int result;
	struct mma8452_data *mma = i2c_get_clientdata(client);
	mma_status.ctl_reg1 = i2c_smbus_read_byte_data(client, MMA8452_CTRL_REG1);
	result = i2c_smbus_write_byte_data(client, MMA8452_CTRL_REG1,mma_status.ctl_reg1 & 0xFE);
	assert(result==0);
	input_unregister_device(mma->input_dev);
	misc_deregister(&mma8452_misc_device);

	return result;
}

static void mma8452_suspend(struct early_suspend *h)
{
	int result;
	struct mma8452_data *mma;
	mma = container_of(h, struct mma8452_data, early_suspend); 
	mma->is_suspend = 1;
	disable_irq_nosync(mma8452_i2c_client->irq);
    /*
	if(atomic_read(&mma->enabled) == 0){
		mma->enabled_save = 0;
	}else{
		mma->enabled_save = 1;
	}
    */
	mma8452_disable(mma);
	mma_status.ctl_reg1 = i2c_smbus_read_byte_data(mma8452_i2c_client, MMA8452_CTRL_REG1);
	result = i2c_smbus_write_byte_data(mma8452_i2c_client, MMA8452_CTRL_REG1,mma_status.ctl_reg1 & 0xFE);
	assert(result==0);
}

static void mma8452_resume(struct early_suspend *h)
{
	int result;
	struct mma8452_data *mma;
	mma = container_of(h, struct mma8452_data, early_suspend); 
	mma_status.ctl_reg1 = i2c_smbus_read_byte_data(mma8452_i2c_client, MMA8452_CTRL_REG1);
	result = i2c_smbus_write_byte_data(mma8452_i2c_client, MMA8452_CTRL_REG1,mma_status.ctl_reg1 | 0x01);
	assert(result==0);
	mma->is_suspend = 0;
    /*
     if(mma->enabled_save == 1){
		mma8452_enable(mma);
    }
    */
	mma8452_enable(mma);
  	enable_irq(mma8452_i2c_client->irq);
}

static const struct i2c_device_id mma8452_id[] = {
	{ MMA8452_DRV_NAME, 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, mma8452_id);

static struct i2c_driver mma8452_driver = {
	.driver = {
		.name	= MMA8452_DRV_NAME,
		.owner	= THIS_MODULE,
	},
/*
	.suspend = mma8452_suspend,
	.resume	= mma8452_resume,
*/
	.probe	= mma8452_probe,
	.remove	= __devexit_p(mma8452_remove),
	.id_table = mma8452_id,
};

static int __init mma8452_init(void)
{
	/* register driver */
	int res;

	res = i2c_add_driver(&mma8452_driver);
	if (res < 0){
		printk(KERN_INFO "add mma8452 i2c driver failed\n");
		return -ENODEV;
	}
	printk(KERN_INFO "add mma8452 i2c driver\n");

	return (res);
}

static void __exit mma8452_exit(void)
{
	printk(KERN_INFO "remove mma8452 i2c driver.\n");
	i2c_del_driver(&mma8452_driver);
}

module_init(mma8452_init);
module_exit(mma8452_exit);

MODULE_AUTHOR("bcjia <bcjia@ingenic.cn>");
MODULE_DESCRIPTION("MMA8452 3-Axis Orientation/Motion Detection Sensor driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.1");

