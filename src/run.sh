#!/bin/bash

exec python pow.py 25 300 \
qemu-system-x86_64 \
    -enable-kvm \
    -kernel vmlinuz-5.3.0-19-generic \
    -initrd initramfs.cpio.gz \
    -nographic \
    -monitor /dev/null \
    -append "console=ttyS0 quiet"
