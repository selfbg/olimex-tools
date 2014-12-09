#define module_license "GPL";
#define module_author "Stefan Mavrodiev";

struct duty_cycle {
	unsigned int period;
	unsigned int pulse;
};
struct softpwm_platform_data {
	unsigned gpio_handler;
	script_gpio_set_t info;
	char pin_name[16];
	char pwm_name[64];
	struct duty_cycle duty;
	ktime_t next_tick;

};
