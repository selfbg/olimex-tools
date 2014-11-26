#!/bin/sh

BACKTITLE="OlinuXino screen configurator"

temp_dir="/tmp/screen"
mmc_dir="/mnt/mmc"
sunxi_tools_dir="/opt/sunxi-tools"
bin_file="script.bin"
fex_file="script.fex"

tempfile1=/tmp/dialog_1_$$
tempfile2=/tmp/dialog_2_$$
tempfile3=/tmp/dialog_3_$$
tempfile4=/tmp/dialog_4_$$

# Define variable names.
# It's important that they match the ones in the fex file.
# If you have old version of the configuration file they might mismatct, so edit this section.
LCD_X="lcd_x"

# Define some functions

# Find line number for a give word
#
# Parameters:
# $1	<Word> to be searched
#
# Return:
# <line> -> If word is found
# <null> -> There is no such word
#
# Note: Only the first coincidence is returned
function find_word()
{
	echo=$(grep -nr -m 1 "$1" $temp_dir/$fex_file | awk '{print$1}' FS=":")
}

# Find parameter and set its value
# Parameters:
#	$1	<parameter> to search for
#	$2	new value
#
# Note: if a parameter is present in multiple places only the first is replaced.
function change_parameter()
{	
	# Find line number
	local line=$(find_word $1)
	
	# Check if parameter is null
	if [ -z $line ];
	then
		dialog --infobox "Cannot find $1 variable!" 0 0
		sleep 2
		exit
	fi
	
	# Replace parameter
	sed -i $line's/.*/'$1' = '$2'/' $temp_dir/$fex_file
}


# Display confirmation dialog
# Parameters:
#	$1	<Dialog> to be displayed
# Return:
#	0 -> "YES" is pressed
#	1 -> "NO" is pressed 
function display_confirm()
{
	dialog --title "Confirmation" --backtitle $BACKTITLE --yesno $1 0 0
    case $? in
    	0)
    		echo 0
		;;
    	1|255)
    		echo 1
		;;
    esac
}

# Insert a line after matching word
# Parameters:
# $1	<Word>	to be searched
# $2 	<Parameter> to be inserted
# $3	<Value>
function insert_after()
{
	# Find word
	local line=$(find_word $1)
	
	# Chech if it's unset
	if[ -z $line ];
	then
		dialog --infobox "Cannot find $1 variable!" 0 0
		sleep 2
		exit
	else
		sed -i $line'a\'$2' = '$3'' $temp_dir/$fex_file
	fi
}



set_screen_none() {
	if [ $(display_confirm "Disable screen ?") -eq 1 ];
	then
	   	change_parameter "screen0_output_type" "0"
	fi
}

set_screen_hdmi() {
    
    dialog --backtitle $BACKTITLE \
    --menu "Select screen mode:" 12 55 8 \
    0	"480i"		 \
    1	"576i"		 \
    2	"480p"		 \
    3	"576p"		 \
    4	"720p50"	 \
    5	"720p60"	 \
    6	"1080i50"	 \
    7	"1080i60"	 \
    8	"1080p24"	 \
    9	"1080p50"	 \
    10	"1080p60"	 \
    11	"pal"		 \
    14	"ntsc"		2> $tempfile3
  
	retv=$?
	choice=$(cat $tempfile3)

	# Check if ESC of CANCLE are pressed
	if [ $retv -eq 1 -o $retv -eq 255 ];
	then
		clear
		exit
	fi

	case $choice in
		0)
			if [ $(display_confirm "Set HDMI to 480i ?") -eq 1 ];
			then
			   	change_parameter "screen0_output_type" "3"
    			change_parameter "screen0_output_mode" $choice
			fi
    	;;
		1)
			if [ $(display_confirm "Set HDMI to 576i ?") -eq 1 ];
			then
			   	change_parameter "screen0_output_type" "3"
    			change_parameter "screen0_output_mode" $choice
			fi
    	;;
		2)
			if [ $(display_confirm "Set HDMI to 480p ?") -eq 1 ];
			then
			   	change_parameter "screen0_output_type" "3"
    			change_parameter "screen0_output_mode" $choice
			fi
    	;;
		3)
			if [ $(display_confirm "Set HDMI to 576p ?") -eq 1 ];
			then
			   	change_parameter "screen0_output_type" "3"
    			change_parameter "screen0_output_mode" $choice
			fi
    	;;
		4)
			if [ $(display_confirm "Set HDMI to 720p50 ?") -eq 1 ];
			then
			   	change_parameter "screen0_output_type" "3"
    			change_parameter "screen0_output_mode" $choice
			fi
    	;;
		5)
			if [ $(display_confirm "Set HDMI to 720p60 ?") -eq 1 ];
			then
			   	change_parameter "screen0_output_type" "3"
    			change_parameter "screen0_output_mode" $choice
			fi
    	;;  
		6)
			if [ $(display_confirm "Set HDMI to 1080i50 ?") -eq 1 ];
			then
			   	change_parameter "screen0_output_type" "3"
    			change_parameter "screen0_output_mode" $choice
			fi
    	;;  
		7)
			if [ $(display_confirm "Set HDMI to 1080i60 ?") -eq 1 ];
			then
			   	change_parameter "screen0_output_type" "3"
    			change_parameter "screen0_output_mode" $choice
			fi
    	;;  
		8)
			if [ $(display_confirm "Set HDMI to 1080p24 ?") -eq 1 ];
			then
			   	change_parameter "screen0_output_type" "3"
    			change_parameter "screen0_output_mode" $choice
			fi
    	;;  
		9)
			if [ $(display_confirm "Set HDMI to 1080p50 ?") -eq 1 ];
			then
			   	change_parameter "screen0_output_type" "3"
    			change_parameter "screen0_output_mode" $choice
			fi
    	;;  
		10)
			if [ $(display_confirm "Set HDMI to 1080p60 ?") -eq 1 ];
			then
			   	change_parameter "screen0_output_type" "3"
    			change_parameter "screen0_output_mode" $choice
			fi
    	;;  
		11)
			if [ $(display_confirm "Set HDMI to pal ?") -eq 1 ];
			then
			   	change_parameter "screen0_output_type" "3"
    			change_parameter "screen0_output_mode" $choice
			fi
    	;;  
		14)
			if [ $(display_confirm "Set HDMI to ntsc ?") -eq 1 ];
			then
			   	change_parameter "screen0_output_type" "3"
    			change_parameter "screen0_output_mode" $choice
			fi
    	;;  
	esac
    
   
}

set_screen_vga() {
    
    dialog --backtitle $BACKTITLE \
    --menu "Select screen mode:" 0 0 0 \
    0	"1680x1050"	 \
    1	"1440x900"	 \
    2	"1360x768"	 \
    3	"1280x1024"	 \
    4	"1024x768"	 \
    5	"800x600"	 \
    6	"640x480"	 \
    10	"1920x1080"	 \
    11	"1280x720"	 2> $tempfile4
  
	retv=$?
	choice=$(cat $tempfile4)

	# Check if ESC of CANCLE are pressed
	if [ $retv -eq 1 -o $retv -eq 255 ];
	then
		clear
		exit
	fi

	#sed -i $line's/.*/screen0_output_type = 4/' $temp_dir/$fex_file
	case $choice in
		0)
			if [ $(display_confirm "Set VGA to 1680x1050 ?") -eq 1 ];
			then
			   	change_parameter "screen0_output_type" "4"
    			change_parameter "screen0_output_mode" $choice
			fi
    	;;
    	1)
			if [ $(display_confirm "Set VGA to 1440x900 ?") -eq 1 ];
			then
			   	change_parameter "screen0_output_type" "4"
    			change_parameter "screen0_output_mode" $choice
			fi
    	;;
    	2)
			if [ $(display_confirm "Set VGA to 1360x768 ?") -eq 1 ];
			then
			   	change_parameter "screen0_output_type" "4"
    			change_parameter "screen0_output_mode" $choice
			fi
    	;;
    	3)
			if [ $(display_confirm "Set VGA to 1280x1024 ?") -eq 1 ];
			then
			   	change_parameter "screen0_output_type" "4"
    			change_parameter "screen0_output_mode" $choice
			fi
    	;;
    	4)
			if [ $(display_confirm "Set VGA to 1024x768 ?") -eq 1 ];
			then
			   	change_parameter "screen0_output_type" "4"
    			change_parameter "screen0_output_mode" $choice
			fi
    	;;
    	5)
			if [ $(display_confirm "Set VGA to 800x600 ?") -eq 1 ];
			then
			   	change_parameter "screen0_output_type" "4"
    			change_parameter "screen0_output_mode" $choice
			fi
    	;;
    	6)
			if [ $(display_confirm "Set VGA to 640x480 ?") -eq 1 ];
			then
			   	change_parameter "screen0_output_type" "4"
    			change_parameter "screen0_output_mode" $choice
			fi
    	;;
    	10)
			if [ $(display_confirm "Set VGA to 1920x1080 ?") -eq 1 ];
			then
			   	change_parameter "screen0_output_type" "4"
    			change_parameter "screen0_output_mode" $choice
			fi
    	;;
    	11)
			if [ $(display_confirm "Set VGA to 1280x720 ?") -eq 1 ];
			then
			   	change_parameter "screen0_output_type" "4"
    			change_parameter "screen0_output_mode" $choice
			fi
    	;;  
	esac
    
   
}

set_screen_lcd() {

	dialog --backtitle $BACKTITLE \
    --menu "Select screen mode:" 0 0 0 \
    "4.3"	"480x272"	 \
    "7.0"	"800x480"	 \
    "10.3"	"1024x600"	 \
    "15.6"	"1366x768"	 \
    "15.6-FHD"	"1920x1080" 2> $tempfile2
  
	retv=$?
	choice=$(cat $tempfile2)
  
	# Check if ESC of CANCLE are pressed
	if [ $retv -eq 1 -o $retv -eq 255 ];
	then
    	clear
    	exit
    fi
  
	case $choice in
    "4.3")
	    x=480
	    y=272
	    freq=9
	    hbp=40
	    ht=525
	    vbp=8
	    vt=576
	    vspw=5
	    hspw=30
	    #extra options
	    lcd_if=0
	    lcd_lvds_bitwidth=0
	    lcd_lvds_ch=0
	    lcd_frm=1
	    lcd_io_cfg0=0
	    lcd_bl_en_used=1
	    fb0_scaler_mode_enable=0
	    fb0_width=0
	    fb0_height=0
    ;;
    "7.0")
	    x=800
	    y=480
	    freq=33
	    hbp=46
	    ht=1055
	    vbp=23
	    vt=1050
	    vspw=1
	    hspw=30
	    #extra options
	    lcd_if=0
	    lcd_lvds_bitwidth=0
	    lcd_lvds_ch=0
	    lcd_frm=1
	    lcd_io_cfg0=0
	    lcd_bl_en_used=1
	    fb0_scaler_mode_enable=0
	    fb0_width=0
	    fb0_height=0
    ;;
    "10.3")
	    x=1024
	    y=600
	    freq=45
	    hbp=160
	    ht=1200
	    vbp=23
	    vt=1250
	    vspw=2
	    hspw=10
	    #extra options
	    lcd_if=0
	    lcd_lvds_bitwidth=0
	    lcd_lvds_ch=0
	    lcd_frm=1
	    lcd_io_cfg0=0
	    lcd_bl_en_used=1
	    fb0_scaler_mode_enable=0
	    fb0_width=0
	    fb0_height=0
    ;;
	"15.6")
	    x=1366
	    y=768
	    freq=70
	    hbp=54
	    ht=1440
	    vbp=23
	    vt=1616
	    vspw=0
	    hspw=0
	    #extra options
	    lcd_if=3
	    lcd_lvds_bitwidth=1
	    lcd_lvds_ch=0
	    lcd_frm=1
	    lcd_io_cfg0="0x04000000"
	    lcd_bl_en_used=0
	    fb0_scaler_mode_enable=1
	    fb0_width=1366
	    fb0_height=768
    ;;
    "15.6-FHD")
	    x=1920
	    y=1080
	    freq=76
	    hbp=100
	    ht=2226
	    vbp=23
	    vt=2284
	    vspw=0
	    hspw=0
	    #extra options
	    lcd_if=3
	    lcd_lvds_bitwidth=1
	    lcd_lvds_ch=1
	    lcd_frm=1
	    lcd_io_cfg0="67108864"
	    lcd_bl_en_used=0
	    fb0_scaler_mode_enable=1
	    fb0_width=1920
	    fb0_height=1080
    ;;
    esac
    
    if [ $(display_confirm "Set LCD to '$x'x'$y' ?") -eq 1 ];
	then
	   	change_parameter "screen0_output_type" "1"
	   	change_parameter "lcd_x" $x
	   	change_parameter "lcd_y" $y
	   	change_parameter "lcd_dclk_freq" $freq
	   	change_parameter "lcd_hbp" $hbp
	   	change_parameter "lcd_ht" $ht
	   	change_parameter "lcd_vbp" $vbp
	   	change_parameter "lcd_vt" $vt
	   	change_parameter "lcd_vspw" $vspw
	   	change_parameter "lcd_hspw" $hspw
	   	change_parameter "lcd_if" $lcd_if
	   	change_parameter "lcd_lvds_bitwidth" $lcd_lvds_bitwidth
	   	change_parameter "lcd_io_cfg0" $lcd_io_cfg0
	   	change_parameter "lcd_bl_en_used" $lcd_bl_en_used
	   	change_parameter "fb0_scaler_mode_enable" $fb0_scaler_mode_enable
	   	change_parameter "fb0_width" $fb0_width
	   	change_parameter "fb0_height" $fb0_height
	   	change_parameter "lcd_lvds_ch" $lcd_lvds_ch
	   	change_parameter "lcd_frm" $lcd_frm
	   	change_parameter "screen0_output_type" "1"
	   	change_parameter "screen0_output_type" "1"
	fi

	if [ "$choice" = "15.6" ] || [ "$choice" = "15.6-FHD" ];
	then
		if [ -z $(find_word "pll3") ];
		then
			insert_after "clock" "pll3" "297"
		else
			change_parameter "pll3" "297"
		fi	
	fi
    
}

MAIN() {
	dialog --backtitle $BACKTITLE \
	--menu "Select output type:" 12 55 8 \
	"None" "Disable all screens" \
	"LCD" "Set configuration for LCD" \
	"HDMI" "Set configuration for HDMI" \
	"VGA" "Set configuration for VGA" 2> $tempfile1
  
	retv=$?
	choice=$(cat $tempfile1)

	# Check if ESC of CANCLE are pressed
	if [ $retv -eq 1 -o $retv -eq 255 ];
	then
		clear
		exit
	fi

	# Check selected option
	case $choice in
		"None")
			SET_NONE
		;;
	    "LCD")
	    	SET_LCD
	    ;;
	    "HDMI")
	    	SET_HDMI
	    ;;
	    "VGA")
	    SET_VGA
	    ;;
	esac
	trap "rm -f $tempfile1" 0 1 2 5 15

	umount /dev/mmcblk0p1 > /dev/null 2>&1
	#Export PATH
	#Check for /opt/sunxi-tools
if [ ! -d $sunxi_tools_dir ];
then
  dialog --infobox "No sunxi-tools found!" 3 26
  sleep 2
  clear
  exit
fi
export PATH=$PATH:$sunxi_tools_dir

# Check for /mnt/mmc
if [ ! -d $mmc_dir ];
then
  mkdir -p $mmc_dir
fi

# Check for /mnt/mmc
if [ ! -d $temp_dir ];
then
  mkdir -p $temp_dir
fi

mount /dev/mmcblk0p1 $mmc_dir
sleep 1
if [ $? -ne 0 ];
then
  dialog --infobox "Failed to mount sd-card" 0 0
  sleep 2
  exit
fi
  

# Copy current script
cp $mmc_dir/$bin_file $temp_dir/



# bin2fex
bin2fex $temp_dir/$bin_file > $temp_dir/$fex_file


# Remove binfile
rm $temp_dir/$bin_file

MAIN

# Adding timestamp
TIME=$(date)
sed -i "\$a;LAST EDITED: $TIME" $temp_dir/$fex_file



# make the bin file
fex2bin $temp_dir/$fex_file > $temp_dir/$bin_file


# Copy current script
cp -vf $temp_dir/$bin_file $mmc_dir


# Unmount
umount /dev/mmcblk0p1


rm -rf $temp_dir

dialog --title "Reboot" --yesno "Reboot the board? " 0 0
case $? in
  0) 		reboot;;
  1|255) 	clear;;
esac  
