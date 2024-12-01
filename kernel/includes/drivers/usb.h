/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   usb.h                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vvaucoul <vvaucoul@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/24 11:37:58 by vvaucoul          #+#    #+#             */
/*   Updated: 2024/10/24 14:14:12 by vvaucoul         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef USB_DRIVER_H
#define USB_DRIVER_H

#include <stdint.h>

#define USB_MAX_DEVICES 128
#define USB_ENDPOINT_CONTROL 0
#define USB_ENDPOINT_BULK_IN 1
#define USB_ENDPOINT_BULK_OUT 2

typedef enum {
	USB_SPEED_LOW = 1,
	USB_SPEED_FULL,
	USB_SPEED_HIGH
} usb_speed_t;

// USB device descriptor
typedef struct {
	uint8_t address;
	uint8_t endpoint_control;
	uint8_t endpoint_bulk_in;
	uint8_t endpoint_bulk_out;
	usb_speed_t speed;
} usb_device_t;

// Function declarations for initializing and interacting with USB devices
void usb_init(void);
void usb_detect_and_install_devices(void);

// Function declarations for USB device control and bulk transfers
int usb_reset_device(uint8_t device_address);
int usb_control_transfer(uint8_t device_address, uint8_t request, uint16_t value, uint16_t index, uint8_t *data, uint16_t length);

// USB Mass Storage specific functions
int usb_mass_storage_write(uint8_t device_address, uint32_t lba, uint16_t num_blocks, uint8_t *data);
int usb_mass_storage_read(uint8_t device_address, uint32_t lba, uint16_t num_blocks, uint8_t *data);

// Raw USB device access functions
int usb_raw_write(uint8_t device_address, uint32_t lba, uint8_t *data);
int usb_raw_read(uint8_t device_address, uint32_t lba, uint8_t *data);

void usb_test_write_and_read(void);

#endif // USB_DRIVER_H
