#!/bin/bash

# Script that resize partition to fill the sd card.
#
# Parameters:
# <device> - for example /dev/mmcblk0
# <partition_number> - the number of the partition to resize, for example "1"
#
# Important!
# The partition number should be the last. So if you have 2 partitions
# and you want to resize the first one, you can't!


if [ -z $1 ] || [ -z $2 ];
then
	echo "Error: not enough arguments!"
	echo "Usage: ./resize_sd <device> <partition>"
	exit
fi

# Check if the device is block
if [ ! -b $1 ];
then
	echo "Device $1 does not exist!"
	exit
fi

# Find partition name
partition_name=$1p$2
echo "Partition name: $partition_name"

if [ ! -b $partition_name ];
then
	echo "Partition $partition_name does not exist!"
	exit
fi

# Check if init script exist
if [ -e /etc/init.d/resize_sd ];
then
	echo "Script /etc/init.d/resize_sd exists. Reboot the board or remove the script."
	exit
fi

# Find the start point
partition_start=`fdisk -l $1 | grep $partition_name | awk '{print $2}'`
if [ -z $partition_start ];
then
	echo "Failed to find the start of partition $partition_name !"
	exit
else
	echo "Found the start point of $partition_name: $partition_start"
fi

# Format the sd card
fdisk $1 << __EOF__ >> /dev/null
d
$2
n
p
$2
$partition_start

p
w
__EOF__

# Now add start script for the actual resize
cat << __EOF__ > /etc/init.d/resize_sd &&
#!/bin/sh
### BEGIN INIT INFO
# Provides:resize_sd
# Required-Start: $local_fs $syslog
# Required-Stop: $local_fs $syslog
# Default-Start: 2 3 4 5
# Default-Stop: 0 1 6
# Short-Description: Resize sdcard
# Description: Resize filesystem to fill sdcard
### END INIT INFO

. /lib/lsb/init-functions

case "\$1" in
	start)
		log_daemon_msg "Starting resize sdcard" &&
		resize2fs $partition_name &&
		rm /etc/init.d/resize_sd &&
		update-rc.d resize_sd remove &&
		log_end_msg \$?
		;;
		
	*)
		echo "Usage: \$0 start" >&2
		exit 1
		;;
esac

exit 0
__EOF__

chmod +x /etc/init.d/resize_sd &&
update-rc.d resize_sd defaults &&
echo "Root filesystem will be resized upon the next reboot"
reboot
