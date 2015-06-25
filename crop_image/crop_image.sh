#!/bin/bash

set -e

# Some global variables
CARD_MOUNT_PATH="./mnt/sd"
IMG_MOUNT_PATH="./mnt/img"
TEMP_PATH="./tmp"


PARTITIONS=0
PARTITION_DATA=()
PARTITION_START=()
PARTITION_END=()

# Variables set by optarg
DEVICE=
VERBOSE=
OUTPUT=

# Some colors
YELLOW="\e[33m"
DEFAULT="\e[39m"

function usage {
    cat << EOF
usage: $0 [OPTIONS] DEVICE

Crop image, and make minimal image

OPTIONS:
    -h  Show this message
    -v  Verbose
    -o  Output file
EOF
}

function detect_partitions {

    while read i
    do
        if [[ "$i" == "$DEVICE"* ]]; then
            let PARTITIONS=PARTITIONS+1
            PARTITION_DATA+=("$i")
        fi
    done < <(fdisk $DEVICE -l)
}

function get_partition_start {
    echo ${PARTITION_DATA[$1]} | awk '{print $2}'
}

function get_partition_end {
    echo ${PARTITION_DATA[$1]} | awk '{print $3}'
}

function create_image {
    redirect_cmd dd if=/dev/zero of="$TEMP_PATH/$1" bs=512 count="$2"
    redirect_cmd mkfs.ext3 "$TEMP_PATH/$1"
}

function redirect_cmd {
    if [ -z $VERBOSE ]; then
        "$@" > /dev/null 2>&1
    else
        "$@"
    fi
}

function parse_arguments {
    while getopts ":o:vh" opt "$@";
    do
        case $opt in
            "o")
            OUTPUT=$OPTARG
            ;;
            "v")
            VERBOSE=1
            ;;
            "h")
            usage
            exit 0
            ;;
            \?)
            echo "Invalid option: -$OPTARG" >&2
            exit 1
            ;;
            :)
            echo "Option -$OPTARG requires an argument." >&2
        esac
    done

    DEVICE=${@:$OPTIND:1}
    if [ -z $DEVICE ]; then
        echo "Missing argument DEVICE"
        exit 1
    fi
}

function print_color {
    echo -e -n "$YELLOW$1$DEFAULT"
}

# Parse command line argumetns
parse_arguments "$@"

# Script must be run as root
print_color "Checking for root..."
if [ $(/usr/bin/id -u) -ne 0 ]; then
    print_color "fail\n"
    echo "Run script as root"
    exit 1
else
    print_color "ok\n"
fi

# Check if device exists
print_color "Device exists..."
if [ ! -e "$DEVICE" ]; then
    print_color "no\n"
    exit 1
else
    print_color "yes\n"
fi

# Check for mount points
print_color "Checking for sdcard mounting point..."
if [ ! -d $CARD_MOUNT_PATH ]; then
    print_color "no\n"
    print_color "Creating mounting point..."
    mkdir -p $CARD_MOUNT_PATH
    print_color "done\n"
else
    print_color "ok\n"
fi

print_color "Checking for image mounting point..."
if [ ! -d $IMG_MOUNT_PATH ]; then
    print_color "no\n"
    print_color "Creating mounting point..."
    mkdir -p $IMG_MOUNT_PATH
    print_color "done\n"
else
    print_color "ok\n"
fi

print_color "Checking for temp directory..."
if [ ! -d $TEMP_PATH ]; then
    print_color "no\n"
    print_color "Creating temp directory..."
    mkdir -p $TEMP_PATH
    print_color "done\n"
else
    print_color "ok\n"
fi

print_color "Detecting partitions..."
detect_partitions
print_color "$PARTITIONS\n"
if [ $PARTITIONS -eq 0 ]; then
    exit 1
fi

for i in `seq 1 $PARTITIONS`;
do
    print_color "${PARTITION_DATA[$(($i-1))]}\n"
    PARTITION_START+=($(get_partition_start $(($i-1))))
    PARTITION_END+=($(get_partition_end $(($i-1))))
done


# Create boot partititon
if [ $PARTITIONS -gt 1 ]; then
    print_color "Creating boot partititon...\n"
    create_image "sd1.img" "$((${PARTITION_END[0]}-${PARTITION_START[0]}))"

    # Mount source and destination
    mount -o loop "$TEMP_PATH/sd1.img" $IMG_MOUNT_PATH
    mount $DEVICE"1" $CARD_MOUNT_PATH

    # Copy all files
    print_color "Copying files...\n"
    cp -rf $CARD_MOUNT_PATH/* "$IMG_MOUNT_PATH/"

    umount $CARD_MOUNT_PATH
    umount $IMG_MOUNT_PATH
fi

# Create rootfs image
print_color "Creating rootfs partition...\n"
mount "$DEVICE$PARTITIONS" $CARD_MOUNT_PATH

# Add 20MB free
create_image "sd$PARTITIONS.img" $(($(du -hs -B 512 $CARD_MOUNT_PATH | awk '{print $1}')+$((40960*5))))
mount -o loop "$TEMP_PATH/sd$PARTITIONS.img" $IMG_MOUNT_PATH

# Copy all data
print_color "Copying files...\n"
cp -rf $CARD_MOUNT_PATH/* $IMG_MOUNT_PATH

umount $CARD_MOUNT_PATH
umount $IMG_MOUNT_PATH

if [ -z $OUTPUT ]; then
    OUTPUT="sd.img"
fi

print_color "Creating final image..."
create_image "$OUTPUT" ${PARTITION_START[0]}
print_color "ok\n"
print_color "Copying bootloader..."
redirect_cmd dd if="$DEVICE" of="$OUTPUT" bs=512 count=${PARTITION_START[0]}
print_color "ok\n"

for i in `seq 1 $PARTITIONS`;
do

    print_color "Appending partition $i..."
    redirect_cmd dd if="$TEMP_PATH/sd$i.img" of="$OUTPUT" bs=512 seek=${PARTITION_START[$(($i-1))]} conv=notrunc
    print_color "ok\n"
done

if [ $PARTITIONS -gt 1 ]; then
#Format final image
fdisk "$OUTPUT" << __EOF__
d
2
d
n


${PARTITION_START[0]}
${PARTITION_END[0]}
n


${PARTITION_START[1]}

w
__EOF__
else
fdisk "$OUTPUT" << __EOF__
d
n


${PARTITION_START[0]}

w
__EOF__
fi


# Cleanup
print_color "Cleaning..."
rm -rf ./mnt
rm -rf ./tmp
print_color "ok\n"
