#!/bin/bash
for i in {1..1000}
 do
    echo "dmesg"
    dmesg -c >> dmesg.txt
 sleep 20
done
