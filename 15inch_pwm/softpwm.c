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


struct softpwm_platform_data {
	unsigned gpio_handler;
	script_gpio_set_t info;
	char pin_name[16];
	char pwm_name[64];

	unsigned char duty;

	ktime_t period;
	ktime_t pulse_on;
	ktime_t pulse_off;


	struct hrtimer hr_timer;

};

static struct softpwm_platform_data *p_backlight;
static struct platform_device *p_platform_device;

/* Get gpio pin value */
static int sunxi_gpio_get_value(struct softpwm_platform_data *gpio)
{
	int  ret;
	user_gpio_set_t gpio_info[1];

	ret = gpio_get_one_pin_status(gpio->gpio_handler,
				gpio_info, gpio->pin_name, 1);

	return gpio_info->data;
}

/* Set pin value (output mode) */
static void sunxi_gpio_set_value(struct softpwm_platform_data *gpio, int value)
{
	gpio_write_one_pin_value(gpio->gpio_handler,
					value, gpio->pin_name);
}

static int sunxi_direction_output(struct softpwm_platform_data *gpio, int value)
{
	int ret;

	ret =  gpio_set_one_pin_io_status(gpio->gpio_handler, 1, gpio->pin_name);

	if (!ret)
		ret = gpio_write_one_pin_value(gpio->gpio_handler,
					value, gpio->pin_name);

	return ret;
}


enum hrtimer_restart softpwm_hrtimer_callback(struct hrtimer *timer)
{
	unsigned char current_state;


	ktime_t now = ktime_get();
	current_state = sunxi_gpio_get_value(p_backlight);

	if(!current_state){
		sunxi_gpio_set_value(p_backlight, 1);
		hrtimer_forward(timer, now, p_backlight->pulse);
	}else{
		sunxi_gpio_set_value(p_backlight, 0);
		hrtimer_forward(timer, now, p_backlight->pulse);
	}

	return HRTIMER_RESTART;
}

/* Device device attributes */
//static DEVICE_ATTR(duty, S_IWUSR | S_IWGRP | S_IWOTH, NULL, set_duty_callback);

/* Probe the driver */
static int __devinit softpwm_probe(struct platform_device *pdev)
{
	struct timespec tp;

	printk(KERN_INFO "%s()\n", __FUNCTION__);

	hrtimer_get_res(CLOCK_MONOTONIC, &tp);

	/* Set period to 1kHz, 50% duty */
	p_backlight->period = ktime_set(0, 1000000);
	p_backlight->pulse = ktime_set(0, 500000);

	/* Init gpio as output */
	sunxi_direction_output(p_backlight, 0);


	hrtimer_init(&p_backlight->hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	p_backlight->hr_timer.function = &softpwm_hrtimer_callback;
	hrtimer_start(&p_backlight->hr_timer, p_backlight->pulse, HRTIMER_MODE_REL);

	return 0;
}


static int __devexit softpwm_remove(struct platform_device *pdev)
{
	printk(KERN_INFO "%s()\n", __FUNCTION__);

	hrtimer_cancel(&p_backlight->hr_timer);

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
	int err;
	int softpwm_used = 0;
	char key[20];

	printk(KERN_INFO "%s()\n", __FUNCTION__);

	/* Check if softpwm is used */
	err = script_parser_fetch("softpwm_para", "softpwm_used", &softpwm_used, sizeof(softpwm_used)/sizeof(int));
	if (!softpwm_used || err) {
		printk(KERN_INFO "%s: softpwm is not used in config\n", __FUNCTION__);
		return -EINVAL;
	}

	/* Allocate memory */
	p_backlight = kzalloc(sizeof(struct softpwm_platform_data), GFP_KERNEL);

	if (!p_backlight) {
		printk(KERN_INFO "%s: failed to kzalloc memory\n", __FUNCTION__);
		err = -ENOMEM;
		goto exit;
	}


	/* Set pwm pin_name */
	sprintf(p_backlight->pin_name, "pwm_pin");


	/* Read desired name from fex file */
	sprintf(p_backlight->pwm_name, "backlight_pwm");

	/* Read GPIO data */
	err = script_parser_fetch("softpwm_para", p_backlight->pin_name,
				(int *)&p_backlight->info,
				sizeof(script_gpio_set_t));

	if (err) {
		printk(KERN_INFO "%s failed to find %s\n", __FUNCTION__, key);
		goto exit;
	}

	/* reserve gpio for led */
	p_backlight->gpio_handler = gpio_request_ex("softpwm_para", p_backlight->pin_name);
	if (!p_backlight->gpio_handler) {
		printk(KERN_INFO "%s: cannot request %s, already used ?\n", __FUNCTION__, key);
		goto exit;
	}

	printk(KERN_INFO "%s: softpwm registered @ port:%d, num:%d\n", __FUNCTION__, p_backlight->info.port, p_backlight->info.port_num);


	p_platform_device = platform_device_alloc("softpwm", -1);
	if (!p_platform_device)
		goto exit;

	err = platform_device_add(p_platform_device);
	if (err)
		goto exit;

	return platform_driver_register(&softpwm_driver);

exit:
	if (err != -ENOMEM) {

		if (p_backlight->gpio_handler){
			gpio_release(p_backlight->gpio_handler, 1);
		}

		kfree(p_backlight);
		return err;
	}

	return err;
}


/* Exit function */
static void __exit softpwm_exit(void)
{

	printk(KERN_INFO "%s()\n", __FUNCTION__);

	platform_driver_unregister(&softpwm_driver);
	printk(KERN_INFO "%s: driver unregistered\n", __func__);

	platform_device_unregister(p_platform_device);
	printk(KERN_INFO "%s: device unregistered\n", __func__);

	/* Release gpios */
		if(p_backlight->gpio_handler){
			gpio_release(p_backlight->gpio_handler, 1);
			printk(KERN_INFO "%s: gpio %s released\n", __func__, p_backlight->info.gpio_name);
		}
}

module_init(softpwm_init);
module_exit(softpwm_exit);

MODULE_AUTHOR("Stefan Mavrodiev");
MODULE_LICENSE("GPL");
