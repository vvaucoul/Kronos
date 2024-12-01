/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   usb.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vvaucoul <vvaucoul@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/24 11:38:09 by vvaucoul          #+#    #+#             */
/*   Updated: 2024/10/27 10:07:02 by vvaucoul         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <drivers/usb.h>
#include <string.h>
#include <system/io.h>
#include <system/serial.h>

#define UHCI_BASE_ADDRESS 0xE0 // Updated base address for UHCI (example)

// Define offsets based on UHCI specification
#define UHCI_COMMAND_REGISTER 0x00
#define UHCI_RESET_REGISTER 0x04
#define UHCI_STATUS_REGISTER 0x06
#define UHCI_REQUEST_REGISTER 0x08
#define UHCI_VALUE_REGISTER 0x0A
#define UHCI_INDEX_REGISTER 0x0C
#define UHCI_LENGTH_REGISTER 0x0E
#define UHCI_DATA_REGISTER 0x10
#define UHCI_TRANSFER_STATUS_REGISTER 0x12

#define USB_CLASS_MASS_STORAGE 0x08
#define USB_SUBCLASS_SCSI 0x06
#define USB_PROTOCOL_BULK_ONLY 0x50

static usb_device_t usb_devices[USB_MAX_DEVICES] = {0};

// Function Prototypes
static int enable_uhci_controller(void);
static int initialize_usb_port(uint8_t port_number);
static int usb_msc_init(uint8_t device_address);
static int send_scsi_command(uint8_t device_address, uint8_t *cdb, uint8_t cdb_length, uint8_t *data, uint16_t data_length, uint8_t direction);

// Simple SCSI WRITE command structure
typedef struct {
	uint8_t opcode; // Operation code
	uint8_t flags;
	uint32_t lba; // Logical Block Addressing
	uint8_t reserved;
	uint16_t transfer_length; // Number of blocks
	uint8_t control;
} scsi_write10_t;

// Simple SCSI READ command structure
typedef struct {
	uint8_t opcode; // Operation code
	uint8_t flags;
	uint32_t lba; // Logical Block Addressing
	uint8_t reserved;
	uint16_t transfer_length; // Number of blocks
	uint8_t control;
} scsi_read10_t;

/**
 * @brief Initializes the USB subsystem.
 *
 * This function sets up the USB subsystem by initializing the USB devices array
 * to zero and enabling the UHCI controller. Further initialization steps can be
 * added as needed.
 */
void usb_init(void) {
	// qemu_printf("Initializing USB subsystem...\n");
	memset(usb_devices, 0, sizeof(usb_devices));
	if (enable_uhci_controller() != 0) {
		// qemu_printf("Failed to enable UHCI controller\n");
		return;
	}
	usb_detect_and_install_devices();
}

/**
 * @brief Enables the UHCI controller.
 *
 * This function enables the UHCI controller by writing to the appropriate register.
 *
 * @return 0 on success, -1 on failure.
 */
static int enable_uhci_controller(void) {
	// qemu_printf("Enabling UHCI Controller at I/O base 0x%X...\n", UHCI_BASE_ADDRESS);

	// Set the Run/Stop bit (bit 1) to 1 to enable the controller
	outb(UHCI_BASE_ADDRESS + UHCI_COMMAND_REGISTER, 0x02);
	// qemu_printf("Sent enable command to UHCI Command Register.\n");

	// Implement a delay or poll a status bit to ensure the controller is enabled
	// For simplicity, we'll check the Run/Stop bit
	uint8_t cmd = inb(UHCI_BASE_ADDRESS + UHCI_COMMAND_REGISTER);
	// qemu_printf("Read UHCI Command Register: 0x%X\n", cmd);
	if ((cmd & 0x02) == 0) { // Check if Run/Stop bit is set
		// qemu_printf("Run/Stop bit not set. UHCI Controller failed to enable.\n");
		return -1;
	}

	// qemu_printf("UHCI Controller successfully enabled at I/O base 0x%X\n", UHCI_BASE_ADDRESS);
	return 0;
}

/**
 * @brief Checks if a USB device is present at the given address.
 *
 * This function checks if a USB device is present at the specified address.
 *
 * @param device_address The address of the USB device to check.
 * @return 1 if a device is present, 0 otherwise.
 */
int usb_device_present(uint8_t device_address) {
	if (device_address == 0) {
		// Address 0 is reserved
		return 0;
	}

	// Placeholder: Implement proper device detection and descriptor parsing
	// For testing, assume device at address 1 is a Mass Storage device
	return (device_address == 1) ? 1 : 0;
}

/**
 * @brief Detects and installs USB devices.
 *
 * This function iterates over possible USB device addresses, attempting to reset
 * and initialize each device.
 */
void usb_detect_and_install_devices(void) {
	// qemu_printf("Detecting and installing USB devices...\n");
	for (uint8_t i = 1; i < USB_MAX_DEVICES; ++i) { // Start from 1
		if (usb_device_present(i)) {
			// qemu_printf("Device present at address %d\n", i);
			if (usb_reset_device(i) == 0) {
				usb_devices[i].address = i;
				usb_devices[i].speed = USB_SPEED_FULL;					// Assuming full speed for simplicity
				usb_devices[i].endpoint_control = USB_ENDPOINT_CONTROL; // Set the default endpoint

				// Initialize Mass Storage device
				if (usb_msc_init(i) == 0) {
					// qemu_printf("USB Mass Storage Device Detected and Installed: Address %d\n", i);
				} else {
					// qemu_printf("Failed to initialize USB Mass Storage Device at address %d\n", i);
				}
			} else {
				// qemu_printf("Failed to reset USB device at address %d\n", i);
			}
		} else {
			// qemu_printf("No device present at address %d\n", i);
		}
	}
}

/**
 * @brief Initializes a USB Mass Storage device.
 *
 * This function sets up endpoints and performs necessary initialization for a Mass Storage device.
 *
 * @param device_address The address of the USB Mass Storage device.
 * @return 0 on success, -1 on failure.
 */
static int usb_msc_init(uint8_t device_address) {
	// qemu_printf("Initializing USB Mass Storage device at address %d...\n", device_address);

	// Placeholder: Discover endpoints and assign bulk_in and bulk_out
	// In a real implementation, you would parse the device descriptors to find Bulk IN and Bulk OUT endpoints

	// For testing, assign arbitrary endpoint numbers
	usb_devices[device_address].endpoint_bulk_in = 1;  // Example endpoint number
	usb_devices[device_address].endpoint_bulk_out = 2; // Example endpoint number

	// Further initialization can be done here (e.g., sending SCSI INQUIRY)

	return 0; // Assume success for testing
}

/**
 * @brief Resets a USB device.
 *
 * This function sends a reset signal to the specified USB device.
 *
 * @param device_address The address of the USB device to reset. Must be less than USB_MAX_DEVICES.
 * @return 0 on success, -1 if the device_address is invalid or reset fails.
 */
int usb_reset_device(uint8_t device_address) {
	if (device_address >= USB_MAX_DEVICES || device_address == 0) {
		// qemu_printf("Invalid device address: %d\n", device_address);
		return -1;
	}

	// qemu_printf("Resetting USB device at address %d...\n", device_address);

	// Example: Send a reset command to the device
	// Actual implementation requires proper USB transaction setup
	outb(UHCI_BASE_ADDRESS + UHCI_RESET_REGISTER, device_address);
	// qemu_printf("Sent reset command to UHCI Reset Register.\n");

	// Implement a proper delay or poll status
	for (volatile int i = 0; i < 1000; i++)
		;

	uint8_t status = inb(UHCI_BASE_ADDRESS + UHCI_STATUS_REGISTER);
	// qemu_printf("Read UHCI Status Register: 0x%X\n", status);
	if ((status & 0x01) == 0) {
		// qemu_printf("Device at address %d failed to respond\n", device_address);
		return -1;
	}

	// qemu_printf("USB device at address %d successfully reset.\n", device_address);
	return 0;
}

/**
 * @brief Sends a SCSI command to a USB Mass Storage device.
 *
 * This function sends a SCSI command descriptor block (CDB) to the device and handles data transfer.
 *
 * @param device_address The address of the USB device.
 * @param cdb Pointer to the command descriptor block.
 * @param cdb_length Length of the CDB.
 * @param data Pointer to data buffer (for data phase).
 * @param data_length Length of data.
 * @param direction Direction of data transfer: 0 for OUT, 1 for IN.
 * @return 0 on success, -1 on failure.
 */
static int send_scsi_command(uint8_t device_address, uint8_t *cdb, uint8_t cdb_length, uint8_t *data, uint16_t data_length, uint8_t direction) {
	if (device_address >= USB_MAX_DEVICES || device_address == 0) {
		// qemu_printf("Invalid device address: %d\n", device_address);
		return -1;
	}

	// Placeholder: Implement sending the CDB via Bulk OUT and handling data transfer
	// This requires setting up USB transfer descriptors, handling data buffers, etc.
	// For simplicity, we'll simulate success

	// qemu_printf("Sending SCSI command to device %d...\n", device_address);
	// Simulate sending command
	for (uint8_t i = 0; i < cdb_length; i++) {
		outb(UHCI_BASE_ADDRESS + UHCI_DATA_REGISTER + i, cdb[i]);
	}
	// qemu_printf("SCSI command sent.\n");

	// Simulate data transfer delay
	for (volatile int i = 0; i < 1000; i++)
		;

	// Simulate successful transfer
	// qemu_printf("SCSI command executed successfully.\n");
	return 0;
}

/**
 * @brief Performs a USB Mass Storage write operation.
 *
 * This function writes data to the specified Logical Block Address (LBA).
 *
 * @param device_address The address of the USB device.
 * @param lba Logical Block Address to write to.
 * @param num_blocks Number of 512-byte blocks to write.
 * @param data Pointer to data buffer.
 * @return 0 on success, -1 on failure.
 */
int usb_mass_storage_write(uint8_t device_address, uint32_t lba, uint16_t num_blocks, uint8_t *data) {
	scsi_write10_t write_cmd;
	memset(&write_cmd, 0, sizeof(write_cmd));

	write_cmd.opcode = 0x2A; // WRITE(10) SCSI Command
	write_cmd.lba = lba;
	write_cmd.transfer_length = num_blocks;

	// qemu_printf("Preparing WRITE(10) command to LBA %u for %u blocks.\n", lba, num_blocks);
	return send_scsi_command(device_address, (uint8_t *)&write_cmd, sizeof(write_cmd), data, num_blocks * 512, 0);
}

/**
 * @brief Performs a USB Mass Storage read operation.
 *
 * This function reads data from the specified Logical Block Address (LBA).
 *
 * @param device_address The address of the USB device.
 * @param lba Logical Block Address to read from.
 * @param num_blocks Number of 512-byte blocks to read.
 * @param data Pointer to data buffer.
 * @return 0 on success, -1 on failure.
 */
int usb_mass_storage_read(uint8_t device_address, uint32_t lba, uint16_t num_blocks, uint8_t *data) {
	scsi_read10_t read_cmd;
	memset(&read_cmd, 0, sizeof(read_cmd));

	read_cmd.opcode = 0x28; // READ(10) SCSI Command
	read_cmd.lba = lba;
	read_cmd.transfer_length = num_blocks;

	// qemu_printf("Preparing READ(10) command from LBA %u for %u blocks.\n", lba, num_blocks);
	return send_scsi_command(device_address, (uint8_t *)&read_cmd, sizeof(read_cmd), data, num_blocks * 512, 1);
}

/**
 * @brief Writes raw data to a specified LBA on the USB device.
 *
 * @param device_address The address of the USB device.
 * @param lba Logical Block Address to write to.
 * @param data Pointer to the data buffer (must be 512 bytes).
 * @return 0 on success, -1 on failure.
 */
int usb_raw_write(uint8_t device_address, uint32_t lba, uint8_t *data) {
	if (device_address >= USB_MAX_DEVICES || device_address == 0) {
		// qemu_printf("Invalid device address: %d\n", device_address);
		return -1;
	}

	// Ensure data is 512 bytes (1 block)
	// In a real implementation, you'd handle multiple blocks and error checking
	if (!data) {
		// qemu_printf("Data buffer is NULL.\n");
		return -1;
	}

	// Prepare SCSI WRITE(10) command
	scsi_write10_t write_cmd;
	memset(&write_cmd, 0, sizeof(write_cmd));

	write_cmd.opcode = 0x2A; // WRITE(10) SCSI Command
	write_cmd.lba = lba;
	write_cmd.transfer_length = 1; // Number of blocks

	// qemu_printf("Writing raw data to USB device at LBA %u...\n", lba);
	if (send_scsi_command(device_address, (uint8_t *)&write_cmd, sizeof(write_cmd), data, 512, 0) == 0) {
		// qemu_printf("Successfully wrote to USB device at LBA %u.\n", lba);
		return 0;
	} else {
		// qemu_printf("Failed to write to USB device at LBA %u.\n", lba);
		return -1;
	}
}

/**
 * @brief Reads raw data from a specified LBA on the USB device.
 *
 * @param device_address The address of the USB device.
 * @param lba Logical Block Address to read from.
 * @param data Pointer to the data buffer (must be 512 bytes).
 * @return 0 on success, -1 on failure.
 */
int usb_raw_read(uint8_t device_address, uint32_t lba, uint8_t *data) {
	if (device_address >= USB_MAX_DEVICES || device_address == 0) {
		// qemu_printf("Invalid device address: %d\n", device_address);
		return -1;
	}

	// Ensure data is 512 bytes (1 block)
	if (!data) {
		// qemu_printf("Data buffer is NULL.\n");
		return -1;
	}

	// Prepare SCSI READ(10) command
	scsi_read10_t read_cmd;
	memset(&read_cmd, 0, sizeof(read_cmd));

	read_cmd.opcode = 0x28; // READ(10) SCSI Command
	read_cmd.lba = lba;
	read_cmd.transfer_length = 1; // Number of blocks

	// qemu_printf("Reading raw data from USB device at LBA %u...\n", lba);
	if (send_scsi_command(device_address, (uint8_t *)&read_cmd, sizeof(read_cmd), data, 512, 1) == 0) {
		// qemu_printf("Successfully read from USB device at LBA %u.\n", lba);
		return 0;
	} else {
		// qemu_printf("Failed to read from USB device at LBA %u.\n", lba);
		return -1;
	}
}

/**
 * @brief Test function to write and read "Hello USB World!" to/from the USB device.
 */
void usb_test_write_and_read(void) {
	uint8_t write_data[512];
	uint8_t read_data[512];
	memset(write_data, 0, sizeof(write_data));
	memset(read_data, 0, sizeof(read_data));

	const char *message = "Hello USB World!";
	strncpy((char *)write_data, message, sizeof(write_data) - 1);

	uint32_t lba = 1; // Choosing LBA 1 to avoid overwriting the boot sector

	// Write to USB device
	if (usb_raw_write(1, lba, write_data) == 0) {
		// qemu_printf("Write operation successful.\n");
	} else {
		// qemu_printf("Write operation failed.\n");
		return;
	}

	// Read back from USB device
	if (usb_raw_read(1, lba, read_data) == 0) {
		// qemu_printf("Read operation successful.\n");
	} else {
		// qemu_printf("Read operation failed.\n");
		return;
	}

	// Verify the data
	if (strncmp((char *)write_data, (char *)read_data, strlen(message)) == 0) {
		// qemu_printf("Data verification successful: %s\n", read_data);
	} else {
		// qemu_printf("Data verification failed.\n");
	}
}