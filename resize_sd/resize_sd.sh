#!/bin/bash

fdisk_first() {
                p1_start=`fdisk -l /dev/mmcblk0 | grep mmcblk0p1 | awk '{print $2}'`
                echo "Found the start point of mmcblk0p1: $p2_start"
                fdisk /dev/mmcblk0 << __EOF__ >> /dev/null
d
1
n
p
1
$p1_start

p
w
__EOF__

                sync
                touch /root/.resize
                echo "Ok, Partition resized, please reboot now"
                echo "Once the reboot is completed please run this script again"
}

resize_fs() {
        echo "Activating the new size"
        resize2fs /dev/mmcblk0p1 >> /dev/null
        echo "Done!"                                                                                                                                                                            
        echo "Enjoy your new space!"                                                                                                                                                            
        rm -rf /root/.resize                                                                                                                                                                    
}


if [ -f /root/.resize ]; then
        resize_fs
else
        fdisk_first
fi
