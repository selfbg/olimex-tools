#include "softpwm_gpio.h"

/* Check if gpio num requested and valid */
static int sunxi_gpio_is_valid(unsigned gpio)
{
	if (gpio >= pwm_num)
		return 0;

	if (p_softpwm_platform_data[gpio].gpio_handler)
		return 1;

	return 0;
}

/* Get gpio pin value */
int sunxi_gpio_get_value(unsigned gpio)
{
	int  ret;
	user_gpio_set_t gpio_info[1];

	if (gpio >= pwm_num)
		return -1;

	if(!sunxi_gpio_is_valid(gpio)){
		printk(KERN_DEBUG "%s: gpio num %d does not have valid handler\n", __func__, gpio);
	}

	ret = gpio_get_one_pin_status(p_softpwm_platform_data[gpio].gpio_handler,
				gpio_info, p_softpwm_platform_data[gpio].pin_name, 1);

	return gpio_info->data;
}

/* Set pin value (output mode) */
void sunxi_gpio_set_value(unsigned gpio, int value)
{
	int ret ;

	if (gpio >= pwm_num)
		return;

	if(!sunxi_gpio_is_valid(gpio)){
		printk(KERN_DEBUG "%s: gpio num %d does not have valid handler\n", __func__, gpio);
	}

	ret = gpio_write_one_pin_value(p_softpwm_platform_data[gpio].gpio_handler,
					value, p_softpwm_platform_data[gpio].pin_name);

	return;
}

static int sunxi_direction_output(unsigned gpio, int value)
{
	int ret;

	if (gpio >= pwm_num)
		return -1;

	if(!sunxi_gpio_is_valid(gpio)){
		printk(KERN_DEBUG "%s: gpio num %d does not have valid handler\n", __func__, gpio);
	}

	ret =  gpio_set_one_pin_io_status(p_softpwm_platform_data[gpio].gpio_handler, 1,
			p_softpwm_platform_data[gpio].pin_name);
	if (!ret)
		ret = gpio_write_one_pin_value(p_softpwm_platform_data[gpio].gpio_handler,
					value, p_softpwm_platform_data[gpio].pin_name);

	return ret;
}
