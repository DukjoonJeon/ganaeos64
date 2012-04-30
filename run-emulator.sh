#!/bin/sh

qemu-system-x86_64 -fda ganaeos64.bin -monitor stdio -m 128 -gdb tcp::12345,ipv4 -S