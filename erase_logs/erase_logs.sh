#!/bin/bash

for log in $(find /var/log -type f);
do
	echo "Erasing log file: $log"
	> $log
done