#! /bin/bash

cp 'Original Disks/disk' diskimage

rm a.out 2> /dev/null

gcc *.c

./a.out
