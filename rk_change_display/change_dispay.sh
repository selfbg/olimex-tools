#!/bin/bash

# Script that change the output resolution and device.
# Since this is hardcoded in the kernel, the kernel overwrite the old one.

sd_boot="/dev/mmcblk0"
nand_boot="/dev/block/mtd/by-name/boot"
kern_dir="/etc/olimex/kernel"


echo "Warning! This script will overwrite your kernel with default one."
read -p "Continue? [yes/no]: " answer
case $answer in
	[yY] | [yY][eE][Ss] )
	echo ""		
	;;
	[nN] | [nN][oO] )
	echo "Abort"
	exit 1
	;;
	*)
	echo "Invalid input"
	exit 1
	;;
esac

#Check if kernel directory exists
if [ ! -d "$kern_dir" ];
then
	echo "Kernel directory does not exist!"
	exit 1
fi

echo "From where you boot?"
echo "[0] SD-card"
echo "[1] NAND flash"
read -p "Enter [0/1]: " media
case $media in
	0)
	echo "Selected: SD-card"
	boot=$sd_boot
	;;
	1)
	echo "Selected: NAND flash"
	boot=$nand_boot
	;;
	*)
	echo "Invalid input"
	exit 1
	;;
esac

echo ""
echo "Select new configuration:"
echo "[0] HDMI 480p"
echo "[1] HDMI 720p"
echo "[2] HDMI 1080p"
echo "[3] LCD 4.3 inch"
echo "[4] LCD 7 inch"
echo "[5] LCD 10 inch"
read -p "Select [0..5]: " out

case $out in
	0)
	echo "Selected: HDMI 480p"
	kernel="$kern_dir/kernel_hdmi_480p.img"
	;;
	1)
	echo "Selected: HDMI 720p"
	kernel="$kern_dir/kernel_hdmi_720p.img"
	;;
	2)
	echo "Selected: HDMI 1080p"
	kernel="$kern_dir/kernel_hdmi_1080p.img"
	;;
	3)
	echo "Selected: LCD 4.3 inch"
	kernel="$kern_dir/kernel_lcd_43.img"
	;;
	4)
	echo "Selected: LCD 7 inch"
	kernel="$kern_dir/kernel_lcd_7.img"
	;;
	5)
	echo "Selected: LCD 10 inch"
	kernel="$kern_dir/kernel_lcd_10.img"
	;;
esac

if [ $media == 0 ];
then
command="dd if=$kernel of=$boot conv=sync,fsync seek=16384"
elif [ $media == 1 ];
then
command="dd if=$kernel of=$boot conv=sync,fsync bs=16384"
fi
$command

echo ""
read -p "Reboot? [yes/no]: " answer
case $answer in
	[yY] | [yY][eE][Ss] )
	reboot	
	;;
	[nN] | [nN][oO] )
	exit 0
	;;
	*)
	echo "Invalid input"
	exit 1
	;;
esac