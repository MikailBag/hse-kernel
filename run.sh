#!/usr/bin/env bash
set -euxo pipefail
sudo rm /run/firecracker.socket || true
cp rootfs.ext4 vm-rootfs.ext4

mkdir -p ./mnt
#sudo mount vm-rootfs.ext4 ./mnt
#sudo cp ./module.ko ./mnt/module.ko
#sudo cp ./setup-mod.sh ./mnt/init.sh
#sudo umount --lazy ./mnt
touch vm-logs.txt
sudo firecracker --config-file ./firecracker-config.json --level Debug --log-path ./vm-logs.txt
