#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <plat/sys_config.h>

struct softpwm_platform_data {
	unsigned gpio_handler;
	script_gpio_set_t info;
	char pin_name[16];
	char pwm_name[64];
};

struct softpwm_priv {
	int num_pwm;

};

/* Number of pwm in fex file */
static int pwm_num;
static struct softpwm_platform_data *ppwm;
static struct platform_device *pdev;




static struct timer_list softpwm_timer;
static struct class *softpwm_class;
static struct device *softpwm_device_object;





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
static void softpwm_timer_handler(unsigned long arg)
{
	printk(KERN_DEBUG "%s\n", __FUNCTION__);
	mod_timer(&softpwm_timer, jiffies + msecs_to_jiffies(1000));
}

static DEVICE_ATTR(duty, S_IWUSR | S_IWGRP | S_IWOTH, NULL, set_duty_callback);


/* Probe the driver */
static int __devinit softpwm_probe(struct platform_device *pdev)
{
	printk(KERN_DEBUG "%s()\n", __FUNCTION__);

	setup_timer(&softpwm_timer, softpwm_timer_handler, 0);
	mod_timer(&softpwm_timer, jiffies + msecs_to_jiffies(500));

//	softPWM_class = class_create(THIS_MODULE, "softpwm");
//	if(IS_ERR(softPWM_class)){
//		return PTR_ERR(softPWM_class);
//	}
//
//	softPWM_device_object = device_create(softPWM_class, NULL, 0, NULL, "softpwm");
//	if(IS_ERR(softPWM_device_object)){
//		return PTR_ERR(softPWM_device_object);
//	}




	return 0;
}


static int __devexit softpwm_remove(struct platform_device *pdev)
{
	struct softpwm_platform_data *pdata = pdev->dev.platform_data;

	printk(KERN_DEBUG "%s()\n", __FUNCTION__);

	gpio_release(pdata->gpio_handler, 0);

//	device_remove_file(softpwm_device_object, &dev_attr_duty);
//	device_destroy(softpwm_class, 0);
//	class_destroy(softpwm_class);

//	del_timer(&softpwm_timer);

//	kfree(pdev->dev.platform_data);
	platform_device_unregister(pdev);

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
	ppwm = kzalloc(sizeof(struct softpwm_platform_data) * pwm_num,
				GFP_KERNEL);

	if (!ppwm) {
		printk(KERN_DEBUG "%s: failed to kzalloc memory\n", __FUNCTION__);
		err = -ENOMEM;
		goto exit;
	}

	pwm_i = ppwm;

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

	pdev = platform_device_register_simple("softpwm", 0, NULL, 0);
	pdev->dev.platform_data = kzalloc(sizeof(struct softpwm_platform_data), GFP_KERNEL);
	return platform_driver_register(&softpwm_driver);

exit:
	if (err != -ENOMEM) {

		for (i = 0; i < pwm_num; i++) {
			if (ppwm[i].gpio_handler)
				gpio_release(ppwm[i].gpio_handler, 1);
		}

		kfree(ppwm);
		return err;
	}

	return err;
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
