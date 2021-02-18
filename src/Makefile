obj-m += dev.o
all:
	+make -C $(PWD)/linux-5.10.12 M=$(PWD)/src modules
clean:
	+make -C $(PWD)/linux-5.10.12 M=$(PWD)/src clean
sysall:
	+make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
sysclean:
	+make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
