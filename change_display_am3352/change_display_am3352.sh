#!/bin/bash

# Check for root
if [ "$(whoami)" != "root" ]; then
    echo "Run this script as root." >&2
    exit 1
fi

# Define some files
TARGET="/boot/am335x-olimex-som.dtb"
LCD_4="/boot/dtbs/am335x-olimex-som_lcd_4.dtb"
LCD_7="/boot/dtbs/am335x-olimex-som_lcd_7.dtb"
LCD_10="/boot/dtbs/am335x-olimex-som_lcd_10.dtb"

VGA_800x600="/boot/dtbs/am335x-olimex-som_vga_800x600.dtb"
VGA_1368x768="/boot/dtbs/am335x-olimex-som_vga_1368x768.dtb"

#Usage info
show_help() {
    cat << EOF
Usage: ${0##*/} [-h] [-l MODE] [-v MODE]
Change the default dtb file to use a diferent resolution. This will
OVERWRITE the current one! If you have made some changes to the
device tree, DO NOT USE this script!

-h        display this help and exit
-l MODE   set resolution for LCD
-v MODE   set resolution for VGA

MODE can be 480x272, 800x480 and 1024x600 for LCD.
For VGA - 1368x768 and 800x600.
EOF
}

confirm() {
    read -p "This action will overwrite you current device-tree. Are you sure? " -r
    if [[ $REPLY =~ ^[Yy]$ ]]
        then
        echo "1"
    else
        echo "0"
    fi
}

set_lcd() {
    if [ "$(confirm)" == "0" ];
        then
            echo
            echo "Abort."
            exit 1
        fi

    echo
    case "$1" in
        "480x272")
            if [ ! -e "$LCD_4" ]; then
                echo "\"$LCD_4\" doesn't exist!" >&2
                exit 1
            else
                cp $LCD_4 $TARGET
            fi
        ;;
        "800x480")
            if [ ! -e "$LCD_7" ]; then
                echo "\"$LCD_7\" doesn't exist!" >&2
                exit 1
            else
                cp $LCD_7 $TARGET
            fi
        ;;
        "1024x600")
            if [ ! -e "$LCD_10" ]; then
                echo "\"$LCD_10\" doesn't exist!" >&2
                exit 1
            else
                cp $LCD_10 $TARGET
            fi
        ;;
        *)
            echo "Invalid MODE! Check -h for information" >&2
            exit 1
        ;;
    esac
    echo "Done."
    echo "Reboot the board for changes to take effect."
}


set_vga() {
    if [ "$(confirm)" == "0" ];
        then
            echo
            echo "Abort."
            exit 1
        fi

    echo
    case "$1" in
        "800x600")
            if [ ! -e "$VGA_800x600" ]; then
                echo "\"$VGA_800x600\" doesn't exist!" >&2
                exit 1
            else
                cp $VGA_800x600 $TARGET
            fi
        ;;
        "1368x768")
            if [ ! -e "$VGA_1368x768" ]; then
                echo "\"$VGA_1368x768\" doesn't exist!" >&2
                exit 1
            else
                cp $VGA_1368x768 $TARGET
            fi
        ;;
        *)
            echo "Invalid MODE! Check -h for information" >&2
            exit 1
        ;;
    esac
    echo "Done."
    echo "Reboot the board for changes to take effect."
}


while getopts "hv:l:" opt;
do
    case "$opt" in
        h)
        show_help
        exit 0
        ;;
        l)
        set_lcd "$OPTARG"
        exit 0
        ;;
        v)
        set_vga "$OPTARG"
        exit 0
        ;;
        '?')
        show_help >&2
        exit 1;
        ;;
    esac
done
