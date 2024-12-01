# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    mount_usb.sh                                       :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: vvaucoul <vvaucoul@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2024/10/24 14:09:51 by vvaucoul          #+#    #+#              #
#    Updated: 2024/10/24 14:11:16 by vvaucoul         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

#!/bin/bash

# mount_usb.sh - Script to mount a USB image and display its contents

if [ $# -ne 1 ]; then
	echo "Usage: $0 <usb_image>"
	exit 1
fi

USB_IMAGE=$1

# Create a mount point
MOUNT_POINT="/mnt/myusb"
sudo mkdir -p $MOUNT_POINT

# Mount the USB image
sudo mount -o loop,offset=0 ${USB_IMAGE} $MOUNT_POINT

if [ $? -ne 0 ]; then
	echo "Failed to mount USB image."
	exit 1
fi

echo "USB image mounted at $MOUNT_POINT."

# Display the contents
echo "Contents of the USB image:"
ls -l $MOUNT_POINT

# Unmount after inspection
sudo umount $MOUNT_POINT
sudo rmdir $MOUNT_POINT

echo "USB image unmounted."
