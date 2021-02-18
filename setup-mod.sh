#!/usr/bin/env
set -euxo pipefail
insmod /module.ko
mknod pb c 249 0

# just example of interaction

exec 3<> ./pb
echo 'Cnjohn:a24:elinux@kernel.org:p88005553535' >&3
echo 'Cnvasya' >&3
echo 'Cnvasya' >&3 || true
echo 'Vjohn' >&3
cat <&3
echo 'Vvasya' >&3
cat <&3 
echo 'Dvasya' >&3
echo 'Vjohn' >&3
cat <&3
echo 'Vvasya' >&3
cat <&3
