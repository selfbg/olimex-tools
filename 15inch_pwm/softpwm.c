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

static struct softpwm_platform_data *p_softpwm_platform_data;
static struct platform_device *p_platform_device;



/* Probe the driver */
static int __devinit softpwm_probe(struct platform_device *pdev)
{
	printk(KERN_DEBUG "%s()\n", __FUNCTION__);
	return 0;
}


static int __devexit softpwm_remove(struct platform_device *pdev)
{
	printk(KERN_DEBUG "%s()\n", __FUNCTION__);


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
	for(i = 0; i < pwm_num; i++){
//		if()
	}

}

module_init(softpwm_init);
module_exit(softpwm_exit);

MODULE_AUTHOR("Stefan Mavrodiev <stefan.mavrodiev@gmail.com>");
MODULE_DESCRIPTION("Soft pwm driver for 15.6inch LCDs on OLinuXino boards");
MODULE_LICENSE("GPL");
