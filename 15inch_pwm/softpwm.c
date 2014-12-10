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
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/fs.h>

#include <plat/sys_config.h>

#include <asm/div64.h>


struct softpwm_platform_data {
	unsigned gpio_handler;
	int id;

	script_gpio_set_t info;

	char pin_name[16];
	char device_name[64];

	unsigned duty;
	unsigned last_state;

	ktime_t period;
	ktime_t pulse_on;
	ktime_t pulse_off;
	struct hrtimer hr_timer;

	struct device *dev;

};

static DEFINE_MUTEX(sysfs_lock);

static struct softpwm_platform_data *p_backlight;
static struct softpwm_platform_data *p_dcr;


static int sunxi_gpio_is_valid(struct softpwm_platform_data *gpio)
{
	if(gpio->gpio_handler)
		return 1;

	return 0;
}

static int sunxi_direction_input(struct softpwm_platform_data *gpio)
{
	int ret;

	if(sunxi_gpio_is_valid(gpio)){
		ret = gpio_set_one_pin_io_status(gpio->gpio_handler, 0, gpio->pin_name);
	}else{
		printk(KERN_ERR "%s: requested gpio has no valid handler\n", __func__);
		return 1;
	}
	return ret;
}

/* Set pin value (output mode) */
static void sunxi_gpio_set_value(struct softpwm_platform_data *gpio, int value)
{

	if(sunxi_gpio_is_valid(gpio)){
		gpio_write_one_pin_value(gpio->gpio_handler, value, gpio->pin_name);
	}else{
		printk(KERN_ERR "%s: requested gpio has no valid handler\n", __func__);
	}
}

static int sunxi_direction_output(struct softpwm_platform_data *gpio, int value)
{
	int ret;

	if(!sunxi_gpio_is_valid(gpio)){
		printk(KERN_ERR "%s: requested gpio has no valid handler\n", __func__);
		return 1;
	}

	ret =  gpio_set_one_pin_io_status(gpio->gpio_handler, 1, gpio->pin_name);

	if (!ret)
		ret = gpio_write_one_pin_value(gpio->gpio_handler,
					value, gpio->pin_name);

	return ret;
}



static ssize_t duty_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t ret;
	mutex_lock(&sysfs_lock);
	ret = sprintf(buf, "%d\n", p_backlight->duty);
	mutex_unlock(&sysfs_lock);
	return ret;
}

static ssize_t duty_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	ssize_t ret;
	long duty;
	u64 v;

	mutex_lock(&sysfs_lock);

	ret = strict_strtol(buf, 0, &duty);
	if(ret)
		return -EINVAL;

	if(duty < 0 || duty > 100)
		return -EINVAL;

	p_backlight->duty = duty;
	v = p_backlight->period.tv64;
	do_div(v, 100);

	p_backlight->pulse_on.tv64 = v*duty;
	p_backlight->pulse_off.tv64 = v*(100-duty);

	mutex_unlock(&sysfs_lock);

	return count;

}

static DEVICE_ATTR(duty, 0666, duty_show, duty_store);




enum hrtimer_restart softpwm_hrtimer_callback(struct hrtimer *timer)
{
	unsigned char current_state;
	struct softpwm_platform_data *dev;
	ktime_t now = ktime_get();

	dev = container_of(timer, struct softpwm_platform_data, hr_timer);

	if(dev->id == 0){
		if(!dev->duty){
			sunxi_gpio_set_value(dev, 0);
			hrtimer_forward(timer, now, p_backlight->pulse_off);
		}else if(dev->duty == 100){
			sunxi_gpio_set_value(dev, 1);
			hrtimer_forward(timer, now, p_backlight->pulse_on);
		}else{
			current_state = dev->last_state;
			if(current_state){
				sunxi_gpio_set_value(dev, 0);
				dev->last_state = 0;
				hrtimer_forward(timer, now, p_backlight->pulse_off);
			}else{
				sunxi_gpio_set_value(dev, 1);
				dev->last_state = 1;
				hrtimer_forward(timer, now, p_backlight->pulse_on);
			}
		}
	}

	return HRTIMER_RESTART;
}


static struct attribute *softpwm_sysfs_entries[] = {
		&dev_attr_duty.attr,
		NULL
};

static struct attribute_group softpwm_attribute_group = {
		.name = NULL,
		.attrs = softpwm_sysfs_entries,
};

static struct class softpwm_class = {
		.name = 	"softpwm",
		.owner =	THIS_MODULE,
};



/* Initialization of the driver */
static ssize_t __init softpwm_init(void)
{
	int err;
	int softpwm_used = 0;

	printk(KERN_INFO "%s()\n", __func__);


	/* Check if softpwm is used */
	err = script_parser_fetch("softpwm_para", "softpwm_used", &softpwm_used, sizeof(softpwm_used)/sizeof(int));
	if (!softpwm_used || err) {
		printk(KERN_INFO "%s: softpwm is not used in config\n", __func__);
		return -EINVAL;
	}

	/* Allocate memory */
	p_backlight = kzalloc(sizeof(struct softpwm_platform_data), GFP_KERNEL);
	p_dcr = kzalloc(sizeof(struct softpwm_platform_data), GFP_KERNEL);

	if (!p_backlight || !p_dcr) {
		printk(KERN_INFO "%s: failed to kzalloc memory\n", __func__);
		err = -ENOMEM;
		goto exit0;
	}


	/* Set pwm pin_name */
	sprintf(p_backlight->pin_name, "pwm_pin");
	sprintf(p_dcr->pin_name, "dcr_pin");

	sprintf(p_backlight->device_name, "backlight");
	sprintf(p_dcr->device_name, "dcr");

	/* Read GPIO data */
	err = script_parser_fetch("softpwm_para", p_backlight->pin_name,
			(int *)&p_backlight->info,
			sizeof(script_gpio_set_t));

	if (err) {
		printk(KERN_INFO "%s failed to find %s\n", __func__, p_backlight->pin_name);
		goto exit0;
	}
	err = script_parser_fetch("softpwm_para", p_dcr->pin_name,
			(int *)&p_dcr->info,
			sizeof(script_gpio_set_t));
	if (err) {
		printk(KERN_INFO "%s failed to find %s\n", __func__, p_dcr->pin_name);
		goto exit0;
	}


	/* Request gpio */
	p_backlight->gpio_handler = gpio_request_ex("softpwm_para", p_backlight->pin_name);
	if (!p_backlight->gpio_handler) {
		printk(KERN_INFO "%s: cannot request %s, already used ?\n", __func__, p_backlight->pin_name);
		goto exit0;
	}
	p_dcr->gpio_handler = gpio_request_ex("softpwm_para", p_dcr->pin_name);
	if (!p_dcr->gpio_handler) {
		printk(KERN_INFO "%s: cannot request %s, already used ?\n", __func__, p_dcr->pin_name);
		goto exit0;
	}

	printk(KERN_INFO "%s: backlight registered @ port:%d, num:%d\n", __func__, p_backlight->info.port, p_backlight->info.port_num);
	printk(KERN_INFO "%s: dcr registered @ port:%d, num:%d\n", __func__, p_dcr->info.port, p_dcr->info.port_num);


	err = class_register(&softpwm_class);
	if(err < 0){
		printk(KERN_INFO "%s: unable to register class\n", __func__);
		goto exit0;
	}


	/* Make sysfs devices */
	p_backlight->dev = device_create(&softpwm_class, &platform_bus, MKDEV(0, 0), p_backlight, p_backlight->device_name);
	if(IS_ERR(p_backlight->dev)) {
		printk(KERN_INFO "%s: device_create failed\n", __func__);
		err = PTR_ERR(p_backlight->dev);
		goto exit1;
	}
	p_dcr->dev = device_create(&softpwm_class, &platform_bus, MKDEV(0, 0), p_dcr, p_dcr->device_name);
	if(IS_ERR(p_backlight->dev)) {
		printk(KERN_INFO "%s: device_create failed\n", __func__);
		err = PTR_ERR(p_dcr->dev);
		goto exit1;
	}

	err = sysfs_create_group(&p_backlight->dev->kobj, &softpwm_attribute_group);
	if(err < 0){
		printk(KERN_INFO "%s: failed to create sysfs device attributes\n", __func__);
		goto exit2;
	}
	err = sysfs_create_group(&p_dcr->dev->kobj, &softpwm_attribute_group);
	if(err < 0){
		printk(KERN_INFO "%s: failed to create sysfs device attributes\n", __func__);
		goto exit2;
	}

	p_backlight->id = 0;
	p_dcr->id = 1;

	/* Set period to 1kHz, 50% duty */
	p_backlight->period = ktime_set(0, 1000000);
	p_backlight->pulse_on = ktime_set(0, 500000);
	p_backlight->pulse_off = ktime_set(0, 500000);


	/* Init gpio as output */
	sunxi_direction_output(p_backlight, 0);
	p_backlight->last_state = 0;
	sunxi_direction_output(p_dcr, 0);
	p_dcr->last_state = 0;


	hrtimer_init(&p_backlight->hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hrtimer_init(&p_dcr->hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);

	p_backlight->hr_timer.function = &softpwm_hrtimer_callback;
	p_dcr->hr_timer.function = &softpwm_hrtimer_callback;

	hrtimer_start(&p_backlight->hr_timer, p_backlight->pulse_off, HRTIMER_MODE_REL);


	return 0;

exit2:
	sysfs_remove_group(&p_backlight->dev->kobj, &softpwm_attribute_group);
	sysfs_remove_group(&p_dcr->dev->kobj, &softpwm_attribute_group);

exit1:
	/* Unregister devices */
	if(p_backlight->dev)
		device_unregister(p_backlight->dev);
	if(p_dcr->dev)
		device_unregister(p_dcr->dev);

	/* Unregister class */
	class_unregister(&softpwm_class);


exit0:
	if (err != -ENOMEM) {

		if(p_backlight->gpio_handler){
			gpio_release(p_backlight->gpio_handler, 1);
		}

		if(p_dcr->gpio_handler){
			gpio_release(p_dcr->gpio_handler, 1);
		}
	}
	kfree(p_dcr);
	kfree(p_backlight);
	return err;
}


/* Exit function */
static void __exit softpwm_exit(void)
{

	printk(KERN_INFO "%s()\n", __func__);

	hrtimer_cancel(&p_backlight->hr_timer);
	hrtimer_cancel(&p_dcr->hr_timer);

	/* Make gpios as inputs */
	sunxi_direction_input(p_backlight);
	sunxi_direction_input(p_dcr);

	/* Release gpios */
	if(p_backlight->gpio_handler){
		gpio_release(p_backlight->gpio_handler, 1);
		printk(KERN_INFO "%s: gpio %s released\n", __func__, p_backlight->info.gpio_name);
	}
	if(p_dcr->gpio_handler){
		gpio_release(p_dcr->gpio_handler, 1);
		printk(KERN_INFO "%s: gpio %s released\n", __func__, p_dcr->info.gpio_name);
	}

	sysfs_remove_group(&p_backlight->dev->kobj, &softpwm_attribute_group);
	sysfs_remove_group(&p_dcr->dev->kobj, &softpwm_attribute_group);

	if(p_backlight->dev)
		device_unregister(p_backlight->dev);
	if(p_dcr->dev)
		device_unregister(p_dcr->dev);

	class_unregister(&softpwm_class);




}

module_init(softpwm_init);
module_exit(softpwm_exit);

MODULE_AUTHOR("Stefan Mavrodiev");
MODULE_LICENSE("GPL");
