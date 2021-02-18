all: vmlinux rootfs.ext4 module.ko
	echo OK

clean:
	+make -C src clean
	rm vm-rootfs.ext4 dist.ext4 module.ko vm-logs.txt || true

superclean: clean
	+make -C linux-5.10.12 clean
	rm linux-src.target linux.tar.xz rootfs.ext4 vmlinux

module.ko: src/dev.c src/Makefile
	+make -C src all 
	cp ./src/dev.ko ./module.ko

vmlinux: linux-src.target .config
	cp .config ./linux-5.10.12
	+make -C linux-5.10.12 all
	cp ./linux-5.10.12/vmlinux .

linux.tar.xz:
	wget https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-5.10.12.tar.xz -O ./linux.tar.xz

linux-src.target: linux.tar.xz
	tar xvf ./linux.tar.xz 
	touch linux-src.target

rootfs.ext4:
	wget https://s3.amazonaws.com/spec.ccfc.min/img/hello/fsfiles/hello-rootfs.ext4 -O rootfs.ext4
