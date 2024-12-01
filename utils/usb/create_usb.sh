# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    create_usb.sh                                      :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: vvaucoul <vvaucoul@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2024/10/24 11:45:36 by vvaucoul          #+#    #+#              #
#    Updated: 2024/10/24 14:04:35 by vvaucoul         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

#!/bin/bash

# create_usb.sh - Script to create a USB image for KronOS

if [ $# -ne 2 ]; then
	echo "Usage: $0 <usb_name> <usb_size>"
	exit 1
fi

USB_NAME=$1
USB_SIZE=$2

# Validate USB size format
if ! echo "$USB_SIZE" | grep -qE '^[0-9]+[KMG]$'; then
	echo "Error: USB size must be a number followed by K, M or G (e.g., 64K, 128M)"
	exit 1
fi

IMG_FILE="${USB_NAME}.img"

# Check if image already exists
if [ -f "$IMG_FILE" ]; then
	echo "Error: Image file '$IMG_FILE' already exists. Choose a different name or remove the existing file."
	exit 1
fi

# Create USB image
qemu-img create -f raw "$IMG_FILE" "$USB_SIZE" || {
	echo "Failed to create image with qemu-img"
	exit 1
}

# Format the image with FAT filesystem
mkfs.vfat "$IMG_FILE" || {
	echo "Failed to format image with mkfs.vfat"
	exit 1
}

echo "Successfully created USB image '$IMG_FILE' with size $USB_SIZE."
