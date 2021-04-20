all: vmlinux
	echo OK

clean:
	+make -C module-src clean
	rm vm-rootfs.ext4 dist.ext4 module.ko vm-logs.txt || true

superclean: clean
	+make -C linux clean
	rm linux-src.target linux.tar.xz rootfs.ext4 vmlinux

module.ko: src/dev.c module-src/Makefile
	+make -C src all 
	cp ./src/dev.ko ./module.ko

vmlinux: linux-src.target .config
	cp .config ./linux
	+make -C linux all
	cp ./linux/vmlinux .

linux.tar.xz:
	wget https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-5.11.11.tar.xz -O ./linux.tar.xz

linux-src.target: linux.tar.xz
	tar xf ./linux.tar.xz
	mv linux-5.11.11 linux
	(cd linux && git apply ../patch.diff)
	touch linux-src.target

rootfs.ext4:
	#wget https://s3.amazonaws.com/spec.ccfc.min/img/hello/fsfiles/hello-rootfs.ext4 -O rootfs.ext4
	wget https://storage.googleapis.com/temp-firecracker-data/hello-rootfs.ext4 -O rootfs.ext4
