#!/bin/sh

make clean
make
qemu-system-x86_64 -fda ganaeos64.bin -monitor stdio -d pcall,int,cpu_reset -D log -s