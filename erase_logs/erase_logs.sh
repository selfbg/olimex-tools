#!/bin/bash

# Script that erase all logs in /var/log/
#

for log in $(find /mnt/sd/var/log -type f);
do
	echo "Erasing log file: $log"
	> $log
done