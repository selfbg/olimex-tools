#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <plat/sys_config.h>


#define GPIO_INPUT 0
#define GPIO_OUTPUT 1

struct softpwm_platform_data {
	unsigned gpio_handler;
	script_gpio_set_t info;
};


static struct timer_list softPWM_timer;
static struct class *softPWM_class;
static struct device *softPWM_device;

static ssize_t set_duty_callback(struct device* dev, struct device_attribute* attr, const char* buf, size_t count)
{
	long duty = 0;
	if(kstrtol(buf, 10, &duty) < 0){
		return -EINVAL;
	}
	if(duty > 100 || duty < 0){
		return -EINVAL;
	}
	printk(KERN_DEBUG "%s: set duty to %d\n", __FUNCTION__, (unsigned char)duty);

	return count;
}
static void SoftPWMTimerHandler(unsigned long arg)
{
	mod_timer(&softPWM_timer, jiffies + usecs_to_jiffies(1000));
}

//static int make_gpio_output(void)
//{
//	int ret;
//	struct sunxi_gpio_chip *sgpio = to_sunxi_gpio(chip);
//	ret =  gpio_set_one_pin_io_status(sgpio->data[gpio].gpio_handler,
//					  GPIO_OUTPUT,
//					  sgpio->data[gpio].pin_name);
//	return ret;
//
//}
//
//static int make_gpio_input(void)
//{
//	int ret;
//	struct sunxi_gpio_chip *sgpio = to_sunxi_gpio(chip);
//	ret =  gpio_set_one_pin_io_status(sgpio->data[gpio].gpio_handler,
//					  GPIO_INPUT,
//					  sgpio->data[gpio].pin_name);
//	return ret;
//}
//

//static ssize_t set_period_callback(struct device* dev, struct device_attribute* attr, const char* buf, size_t count)
//{
//	unsigned int period = 0;
//	if (kstrtol(buf, 10, &period) < 0)
//		return -EINVAL;
//
//	return count;
//}

static DEVICE_ATTR(duty, S_IWUSR | S_IWGRP | S_IWOTH, NULL, set_duty_callback);


/* Probe the driver */
static int __devinit softpwm_probe(struct platform_device *pdev)
{

	int err = 0;
	int ret = 0;
	int softpwm_used = 0;
	struct softpwm_platform_data *pdata = pdev->dev.platform_data;

	printk(KERN_DEBUG "%s()\n", __FUNCTION__);

	if(!pdata) {
		printk(KERN_DEBUG "%s: Invalid platform_data!\n", __FUNCTION__);
		return -ENXIO;
	}

	err = script_parser_fetch("softpwm_para", "softpwm_used", &softpwm_used, sizeof(softpwm_used)/sizeof(int));
	if (!softpwm_used || err) {
		printk(KERN_DEBUG "%s: softpwm is not used in config\n", __FUNCTION__);
		return -EINVAL;
	}

	err = script_parser_fetch("softpwm_para", "softpwm_pin", (int *) &pdata->info, sizeof(script_gpio_set_t));
	if(err){
		printk(KERN_DEBUG "%s: cannot access gpio handler, already used?\n", __FUNCTION__);
		return -EBUSY;
	}

	pdata->gpio_handler = gpio_request_ex("softpwm_para", "softpwm_pin");

	printk(KERN_DEBUG "%s: softpwm registered @ port:%d, num:%d", __FUNCTION__, pdata->info.port, pdata->info.port_num);


	setup_timer(&softPWM_timer, SoftPWMTimerHandler, 0);
	mod_timer(&softPWM_timer, jiffies + msecs_to_jiffies(500));

	softPWM_class = class_create(THIS_MODULE, "soft_pwm");
	if(IS_ERR(softPWM_class)){
		return PTR_ERR(softPWM_class);
	}

	softPWM_device = device_create(softPWM_class, NULL, 0, NULL, "soft_pwm");
	if(IS_ERR(softPWM_device)){
		return PTR_ERR(softPWM_device);
	}

	ret = device_create_file(softPWM_device, &dev_attr_duty);


	return 0;
}


static int __devexit softpwm_remove(struct platform_device *pdev)
{
	struct softpwm_platform_data *pdata = pdev->dev.platform_data;

	printk(KERN_DEBUG "%s()\n", __FUNCTION__);

	gpio_release(pdata->gpio_handler, 0);

	device_remove_file(softPWM_device, &dev_attr_duty);
	device_destroy(softPWM_class, 0);
	class_destroy(softPWM_class);

	del_timer(&softPWM_timer);

	return 0;

}


static struct platform_driver softpwm_driver = {
	.probe		= softpwm_probe,
	.remove		= __devexit_p(softpwm_remove),
	.driver		= {
		.name	= "softpwm",
		.owner	= THIS_MODULE,
	},
};

/* Initialization of the driver */
static ssize_t __init softpwm_init(void)
{

	printk(KERN_DEBUG "%s()\n", __FUNCTION__);

	return platform_driver_register(&softpwm_driver);
}


/* Exit function */
static void __exit softpwm_exit(void)
{
	printk(KERN_DEBUG "%s()\n", __FUNCTION__);

	platform_driver_unregister(&softpwm_driver);

}

module_init(softpwm_init);
module_exit(softpwm_exit);

MODULE_AUTHOR("Stefan Mavrodiev <stefan.mavrodiev@gmail.com>");
MODULE_DESCRIPTION("Soft pwm driver for 15.6inch LCDs on OLinuXino boards");
MODULE_LICENSE("GPL");
