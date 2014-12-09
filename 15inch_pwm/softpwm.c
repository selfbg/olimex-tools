#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>

#include <plat/sys_config.h>

#include "softpwm.h"
#include "softpwm_gpio.h"



/* Lock protects against softpwm_remove() being called while
 * sysfs files are active.
 */
static DEFINE_MUTEX(sysfs_lock);

/* Number of pwm in fex file */
static int pwm_num;

static struct softpwm_platform_data *p_softpwm_platform_data;
static struct platform_device *p_platform_device;

static struct class *p_softpwm_class;
static struct device *p_softpwm_device;

static struct hrtimer hr_timer;



static void pwm_timer_handler(unsigned long foo)
{
	int current_value = sunxi_gpio_get_value(0);
	if(!current_value){
		sunxi_gpio_set_value(0, 1);
		mod_timer(&p_softpwm_platform_data[0].timer,
				jiffies + usecs_to_jiffies(p_softpwm_platform_data[0].duty.time_on));
	}else{
		sunxi_gpio_set_value(0, 0);
		mod_timer(&p_softpwm_platform_data[0].timer,
				jiffies + usecs_to_jiffies(p_softpwm_platform_data[0].duty.time_off));
	}
}

enum hrtimer_restart softpwm_hrtimer_callback(struct hrtimer *timer){
	ktime_t now = ktime_get();
	ktime_t next_tick = ktimer_set(0,0);

	now = ktime_get();
}

static int set_duty_callback(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	long duty = 0;
	unsigned int period;
	unsigned int time_on, time_off;

	if(kstrtol(buf, 10, &duty) < 0)
		return -EINVAL;
	if(duty < 0 || duty > 100)
		return -EINVAL;

	printk(KERN_DEBUG "%s: setting duty to %d\n", __func__, (int)duty);

	period = p_softpwm_platform_data->duty.period;
	time_on = (period/100) * duty;
	time_off = period - time_on;

	printk(KERN_DEBUG "%s: time_on %d\n", __func__, time_on);
	printk(KERN_DEBUG "%s: time_off %d\n", __func__, time_off);

	p_softpwm_platform_data->duty.time_on = time_on;
	p_softpwm_platform_data->duty.time_off = time_off;

	return count;


}
/* Device device attributes */
static DEVICE_ATTR(duty, S_IWUSR | S_IWGRP | S_IWOTH, NULL, set_duty_callback);

/* Probe the driver */
static int __devinit softpwm_probe(struct platform_device *pdev)
{
	int i;
	int ret;

	printk(KERN_DEBUG "%s()\n", __FUNCTION__);
	sunxi_direction_output(0, 0);


	/* Creating class */
	printk(KERN_DEBUG "%s: creating new class\n", __func__);
	p_softpwm_class = class_create(THIS_MODULE, "softpwm");

	/* Allocate memory for devices */
	p_softpwm_device = kzalloc(sizeof(struct device) * pwm_num, GFP_KERNEL);

	/* Initially period is 1000kHz */
	for(i = 0; i < pwm_num; i++){
		p_softpwm_platform_data[i].duty.period = 1000;
		p_softpwm_platform_data[i].duty.time_on = 1000;
		p_softpwm_platform_data[i].duty.time_off = 0;

		printk(KERN_DEBUG "%s: creating new device %s\n", __func__, p_softpwm_platform_data[i].pwm_name);
		p_softpwm_device = device_create(p_softpwm_class,
				NULL,
				0,
				NULL,
				p_softpwm_platform_data[i].pwm_name);

		BUG_ON(IS_ERR(p_softpwm_device));

		ret = device_create_file(p_softpwm_device, &dev_attr_duty);
		BUG_ON(ret < 0);

		break;
	}

	printk(KERN_DEBUG "%s initializating time\n", __func__);

	hrtimer_get_res(CLOCK_MONOTONIC, &tp);
	printk(KERN_DEBUG "%s: clock resolution is %ldns\n", __func__, tp.tv_nsec);

	hrtimer_init(&hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hr_timer.function = &softpwm_hrtimer_callback;

	return 0;
}


static int __devexit softpwm_remove(struct platform_device *pdev)
{
	printk(KERN_DEBUG "%s()\n", __FUNCTION__);

	hrtimer_cancel(&hr_timer);

	/* Destroy devices */
	device_remove_file(p_softpwm_device, &dev_attr_duty);
	device_destroy(p_softpwm_class, 0);
	class_destroy(p_softpwm_class);

	kfree(p_platform_device->dev.platform_data);


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
	int i;
	int err;
	int softpwm_used = 0;
	struct softpwm_platform_data *pwm_i;
	char key[20];

	printk(KERN_DEBUG "%s()\n", __FUNCTION__);

	/* Check if softpwm is used */
	err = script_parser_fetch("softpwm_para", "softpwm_used", &softpwm_used, sizeof(softpwm_used)/sizeof(int));
	if (!softpwm_used || err) {
		printk(KERN_DEBUG "%s: softpwm is not used in config\n", __FUNCTION__);
		return -EINVAL;
	}


	/* Read number of pwm */
	err = script_parser_fetch("softpwm_para", "softpwm_num", &pwm_num, sizeof(pwm_num)/sizeof(int));
	if (!pwm_num || err) {
		printk(KERN_DEBUG "%s: cannot set %d number of pwms\n", __FUNCTION__, pwm_num);
		return -EINVAL;
	}

	/* Allocate memory */
	p_softpwm_platform_data = kzalloc(sizeof(struct softpwm_platform_data) * pwm_num,
				GFP_KERNEL);

	if (!p_softpwm_platform_data) {
		printk(KERN_DEBUG "%s: failed to kzalloc memory\n", __FUNCTION__);
		err = -ENOMEM;
		goto exit;
	}

	pwm_i = p_softpwm_platform_data;

	/* Request all needed pins */
	for(i = 0; i < pwm_num; i++){

		/* Set pwm pin_name */
		sprintf(pwm_i->pin_name, "softpwm_pin_%d", i+1);


		/* Read desired name from fex file */
		sprintf(key, "softpwm_name_%d", i+1);

		err = script_parser_fetch("softpwm_para", key,
					  (int *)pwm_i->pwm_name,
					  sizeof(pwm_i->pwm_name)/sizeof(int));
		if (err) {
			printk(KERN_DEBUG "%s: failed to find %s\n", __FUNCTION__, key);
			goto exit;
		}

		/* Read GPIO data */
		sprintf(key, "softpwm_pin_%d", i + 1);
		err = script_parser_fetch("softpwm_para", key,
					(int *)&pwm_i->info,
					sizeof(script_gpio_set_t));

		if (err) {
			printk(KERN_DEBUG "%s failed to find %s\n", __FUNCTION__, key);
			break;
		}

		/* reserve gpio for led */
		pwm_i->gpio_handler = gpio_request_ex("softpwm_para", key);
		if (!pwm_i->gpio_handler) {
			printk(KERN_DEBUG "%s: cannot request %s, already used ?\n", __FUNCTION__, key);
			break;
		}

		printk(KERN_DEBUG "%s: softpwm registered @ port:%d, num:%d\n", __FUNCTION__, pwm_i->info.port, pwm_i->info.port_num);

		pwm_i++;
	}

	p_platform_device = platform_device_alloc("softpwm", -1);
	if (!p_platform_device)
		goto exit;

	err = platform_device_add(p_platform_device);
	if (err)
		goto exit;

	return platform_driver_register(&softpwm_driver);

exit:
	if (err != -ENOMEM) {

		for (i = 0; i < pwm_num; i++) {
			if (p_softpwm_platform_data[i].gpio_handler)
				gpio_release(p_softpwm_platform_data[i].gpio_handler, 1);
		}

		kfree(p_softpwm_platform_data);
		return err;
	}

	return err;
}


/* Exit function */
static void __exit softpwm_exit(void)
{
	int i;

	printk(KERN_DEBUG "%s()\n", __FUNCTION__);

	platform_driver_unregister(&softpwm_driver);
	printk(KERN_DEBUG "%s: driver unregistered\n", __func__);

	platform_device_unregister(p_platform_device);
	printk(KERN_DEBUG "%s: device unregistered\n", __func__);

	/* Release gpios */
	for(i = 0; i < pwm_num; i++){
		if(p_softpwm_platform_data[i].gpio_handler){
			gpio_release(p_softpwm_platform_data[i].gpio_handler, 1);
			printk(KERN_DEBUG "%s: gpio %s released\n", __func__, p_softpwm_platform_data[i].info.gpio_name);
		}
	}
}

module_init(softpwm_init);
module_exit(softpwm_exit);

MODULE_AUTHOR(module_author);
MODULE_LICENSE(module_license);
