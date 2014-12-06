obj-m += softpwm.o

all:
	make -C /compile/A20/linux-sunxi/ M=$(PWD) modules

clean:
	make -C /compile/A20/linux-sunxi/ M=$(PWD) clean