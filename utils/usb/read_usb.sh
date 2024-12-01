# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    read_usb.sh                                        :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: vvaucoul <vvaucoul@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2024/10/24 14:15:28 by vvaucoul          #+#    #+#              #
#    Updated: 2024/10/24 14:15:42 by vvaucoul         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

#!/bin/bash

# read_usb.sh - Script to read data from a USB image

if [ $# -ne 2 ]; then
	echo "Usage: $0 <usb_image> <lba>"
	exit 1
fi

USB_IMAGE=$1
LBA=$2

# Validate LBA
if ! echo "$LBA" | grep -qE '^[0-9]+$'; then
	echo "Error: LBA must be a positive integer."
	exit 1
fi

OFFSET=$((LBA * 512))

# Read 512 bytes from the specified LBA
dd if="$USB_IMAGE" of=read_block.bin bs=512 count=1 skip="$LBA" 2>/dev/null

if [ $? -ne 0 ]; then
	echo "Failed to read from USB image."
	exit 1
fi

# Display the content
echo "Content at LBA $LBA:"
strings read_block.bin | grep "Hello USB World!"
