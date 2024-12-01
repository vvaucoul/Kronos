/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   keyboard.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vvaucoul <vvaucoul@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/06/22 13:56:07 by vvaucoul          #+#    #+#             */
/*   Updated: 2024/10/24 11:34:49 by vvaucoul         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <drivers/keyboard.h>
#include <shell/ksh_termcaps.h>
#include <system/irq.h>

#include <ctype.h>
#include <macros.h>

static bool kbd_uppercase = false;
static bool kbd_ctrl = false;
static bool kbd_alt = false;
static bool kbd_capslock = false;
static kbd_lang_t kbd_language = KEYBOARD_LAYOUT_EN;

/* en-US Keyboard Layout */
static const uint8_t kbdus[128] = {
	0, 27, '1', '2', '3', '4', '5', '6', '7', '8',
	'9', '0', '-', '=', '\b',
	'\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0,
	'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', KEY_LEFTSHIFT,
	'\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', KEY_RIGHTSHIFT,
	'*', 0, ' ', KEY_CAPSLOCK, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12, '-',
	0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static const uint8_t kbdus_shift[128] = {
	0, 27, '!', '@', '#', '$', '%', '^', '&', '*',
	'(', ')', '_', '+', '\b',
	'\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0,
	'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', KEY_LEFTSHIFT,
	'|', 'Z', 'X', 'C', 'V', 'B', 'N', '<', '>', '?', KEY_RIGHTSHIFT,
	'*', 0, ' ', KEY_CAPSLOCK, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12, '-',
	0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/* fr-FR Keyboard Layout */
static const uint8_t kbdfr[128] = {
	0, 27, '&', 0xE9, '"', '\'', '(', '-', 0xE8, '_',
	0xE7, 0xE0, ')', '=', '\b',
	'\t', 'a', 'z', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '^', '$', '\n', 0,
	'q', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', 'm', 0xF9, '*', KEY_LEFTSHIFT,
	'<', 'w', 'x', 'c', 'v', 'b', 'n', ',', ';', ':', '!', KEY_RIGHTSHIFT,
	0, 0, ' ', KEY_CAPSLOCK, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12, '-',
	0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static const uint8_t kbdfr_shift[128] = {
	0, 27, '1', '2', '3', '4', '5', '6', '7', '8',
	'9', '0', (uint8_t)0xA8, '+', '\b',
	'\t', 'A', 'Z', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', (uint8_t)0xA8, '*', '\n', 0,
	'Q', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', 'M', '%', (uint8_t)0xB5, KEY_LEFTSHIFT,
	'>', 'W', 'X', 'C', 'V', 'B', 'N', '?', '.', '/', (uint8_t)0xA7, KEY_RIGHTSHIFT,
	'*', 0, ' ', KEY_CAPSLOCK, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12, '-',
	0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static const uint8_t *get_keyboard_codes(void) {
	if (kbd_uppercase) {
		switch (kbd_language) {
			case KEYBOARD_LAYOUT_FR:
				return kbdfr_shift;
			default:
				return kbdus_shift;
		}
	} else {
		switch (kbd_language) {
			case KEYBOARD_LAYOUT_FR:
				return kbdfr;
			default:
				return kbdus;
		}
	}
}

// ! ||--------------------------------------------------------------------------------||
// ! ||                                 Keyboard Buffer                                ||
// ! ||--------------------------------------------------------------------------------||

static char kbd_buffer[KBD_BUFFER_SIZE];
static int kbd_buffer_head = 0;
static int kbd_buffer_tail = 0;

/**
 * @brief Insert a character into the keyboard buffer.
 *
 * This function is responsible for adding a character to the keyboard buffer.
 *
 * @param c The character to be added to the buffer.
 */
static void kbd_buffer_insert(char c) {
	kbd_buffer[kbd_buffer_head] = c;
	kbd_buffer_head = (kbd_buffer_head + 1) % KBD_BUFFER_SIZE;
	if (kbd_buffer_head == kbd_buffer_tail) {
		// Buffer overflow, advance the tail
		kbd_buffer_tail = (kbd_buffer_tail + 1) % KBD_BUFFER_SIZE;
	}
}

/**
 * Retrieves a character from the keyboard buffer.
 *
 * @return The character retrieved from the keyboard buffer.
 */
static char kbd_buffer_get(void) {
	if (kbd_buffer_head == kbd_buffer_tail) {
		return 0; // Buffer is empty
	}
	char c = kbd_buffer[kbd_buffer_tail];
	kbd_buffer_tail = (kbd_buffer_tail + 1) % KBD_BUFFER_SIZE;
	return c;
}

// ! ||--------------------------------------------------------------------------------||
// ! ||                                 Keyboard Layout                                ||
// ! ||--------------------------------------------------------------------------------||

/**
 * Sets the keyboard layout to the specified language.
 *
 * @param lang The language to set the keyboard layout to.
 */
void keyboard_set_layout(kbd_lang_t lang) {
	kbd_language = lang;
	printk("\t   Keyboard layout set to "_GREEN
		   "%s"_END
		   "\n",
		   lang == KEYBOARD_LAYOUT_EN ? "EN" : "FR");
}

/**
 * @brief Retrieves the status of the uppercase mode of the keyboard.
 *
 * This function returns the current status of the uppercase mode of the keyboard.
 *
 * @return The status of the uppercase mode of the keyboard.
 */
int kbd_uppercase_status(void) {
	return kbd_uppercase;
}

/**
 * @brief Retrieves the control status of the keyboard.
 *
 * This function returns the control status of the keyboard.
 *
 * @return The control status of the keyboard.
 */
int kbd_ctrl_status(void) {
	return kbd_ctrl;
}

/**
 * @brief Retrieves the status of the Alt key on the keyboard.
 *
 * This function returns the status of the Alt key on the keyboard.
 *
 * @return The status of the Alt key.
 */
int kbd_alt_status(void) {
	return kbd_alt;
}

// ! ||--------------------------------------------------------------------------------||
// ! ||                                scan code handler                               ||
// ! ||--------------------------------------------------------------------------------||

/**
 * Handles the given scancode.
 *
 * @param scancode The scancode to be handled.
 * @return True if the scancode was successfully handled, false otherwise.
 */
static bool scancode_handler(uint8_t scancode, int pressed) {
	if (pressed) {
		switch (scancode) {
			case KEY_LEFTSHIFT: kbd_uppercase = true; return true;
			case KEY_RIGHTSHIFT: kbd_uppercase = true; return true;
			case KEY_LEFTCTRL: kbd_ctrl = true; return true;
			case KEY_RIGHTCTRL: kbd_ctrl = true; return true;
			case KEY_LEFTALT: kbd_alt = true; return true;
			case KEY_RIGHTALT: kbd_alt = true; return true;
			case KEY_CAPSLOCK: kbd_capslock = !kbd_capslock; return true; // Toggle caps lock
			default: break;
		}
	} else {
		switch (scancode) {
			case KEY_LEFTSHIFT: kbd_uppercase = false; return true;
			case KEY_RIGHTSHIFT: kbd_uppercase = false; return true;
			case KEY_LEFTCTRL: kbd_ctrl = false; return true;
			case KEY_RIGHTCTRL: kbd_ctrl = false; return true;
			case KEY_LEFTALT: kbd_alt = false; return true;
			case KEY_RIGHTALT: kbd_alt = false; return true;
			default: break;
		}
	}
	return false;
}

/**
 * @brief Retrieves a character from the keyboard input buffer.
 *
 * This function reads a character from the keyboard input buffer and returns it.
 *
 * @return The character read from the keyboard input buffer.
 */
int getchar(void) {
	char c;

	while ((c = kbd_buffer_get()) == 0) {
		__asm__ volatile("hlt");
	}
	return (int)c;
}

/**
 * @brief Handles keyboard interrupts.
 *
 * This function is the interrupt handler for keyboard interrupts. It takes a pointer to a struct regs
 * as a parameter, which contains the register values at the time of the interrupt. The function is responsible
 * for handling the keyboard interrupt and performing any necessary actions.
 *
 * @param r A pointer to a struct regs containing the register values at the time of the interrupt.
 */
void keyboard_handler(__unused__ struct regs *r) {
	uint8_t scancode = inb(KBD_DATA_PORT);

	/* Check if the key was released */
	if (scancode & 0x80) {
		uint8_t keycode = scancode & 0x7F;

		/* Handle the key release */
		scancode_handler(keycode, false);
	}
	/* Check if the key was pressed */
	else {
		/**
		 * Check if the scancode is a special key (e.g. shift, ctrl, alt, etc.)
		 * We don't want to add these to the buffer, so we handle them separately.
		 */
		if (!scancode_handler(scancode, true)) {

			/* Get the key from the scancode */
			char key = get_keyboard_codes()[scancode];
			if (key) {
				if (kbd_capslock && isalpha(key)) {
					key = toupper(key);
				}
				kbd_buffer_insert(kbd_uppercase ? toupper(key) : key);
			}
		}
	}
	pic8259_send_eoi(IRQ_KBD);
}

/**
 * Installs the keyboard driver.
 */
void keyboard_install(void) {
	irq_install_handler(IRQ_KBD, keyboard_handler);
}