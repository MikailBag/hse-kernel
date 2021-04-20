obj-m += dev.o
all:
	+make -C $(PWD)/linux M=$(PWD)/module-src modules
clean:
	+make -C $(PWD)/linux M=$(PWD)/module-src clean
sysall:
	+make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
sysclean:
	+make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
