/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   PS2_8042.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vvaucoul <vvaucoul@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/24 11:14:29 by vvaucoul          #+#    #+#             */
/*   Updated: 2024/10/24 11:34:54 by vvaucoul         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <drivers/ps2/PS2_8042.h>
#include <system/io.h>
#include <system/irq.h>

#define MAX_PS2_HANDLERS 10

// Structure du contrôleur PS/2
static ps2_controller_t ps2_controller;

void ps2_interrupt_handler();

// Tableau de gestionnaires enregistrés
static ps2_handler_t ps2_handlers[MAX_PS2_HANDLERS];
static int ps2_handler_count = 0;

// Fonction d'attente jusqu'à ce que le buffer d'entrée soit vide
static void ps2_wait_input_buffer_empty() {
	while (inb(PS2_STATUS_PORT) & PS2_INPUT_BUFFER_FULL)
		;
}

// Fonction d'attente jusqu'à ce que le buffer de sortie soit plein
static void ps2_wait_output_buffer_full() {
	while (!(inb(PS2_STATUS_PORT) & PS2_OUTPUT_BUFFER_FULL))
		;
}

// Initialisation du contrôleur PS/2
int ps2_init() {
	// Effectuer un auto-test du contrôleur
	ps2_send_command(PS2_CMD_SELF_TEST);
	uint8_t response = 0;
	if (ps2_read_data(&response) != 0 || response != 0x55) {
		return -1;
	}

	// Activer le clavier
	ps2_send_command(PS2_CMD_ENABLE_KEYBOARD);

	// Enregistrer le gestionnaire d'interruption (IRQ1 pour le clavier)
	// irq_install_handler(1, ps2_interrupt_handler);

	return 0;
}

// Envoi d'une commande au contrôleur PS/2
int ps2_send_command(uint8_t cmd) {
	ps2_wait_input_buffer_empty();
	outb(PS2_CMD_PORT, cmd);
	return 0;
}

// Lecture des données du port de données PS/2
int ps2_read_data(uint8_t *data) {
	if (inb(PS2_STATUS_PORT) & PS2_OUTPUT_BUFFER_FULL) {
		*data = inb(PS2_DATA_PORT);
		return 0;
	}
	return -1;
}

// Enregistrement d'un gestionnaire de scancode
int ps2_register_handler(ps2_handler_t handler) {
	if (ps2_handler_count >= MAX_PS2_HANDLERS) {
		return -1; // Trop de gestionnaires enregistrés
	}
	ps2_handlers[ps2_handler_count++] = handler;
	return 0;
}

// Gestionnaire d'interruption PS/2 (IRQ1 pour le clavier)
void ps2_interrupt_handler() {
	uint8_t scancode;
	if (ps2_read_data(&scancode) == 0) {
		// Appeler tous les gestionnaires enregistrés
		for (int i = 0; i < ps2_handler_count; i++) {
			if (ps2_handlers[i]) {
				ps2_handlers[i](scancode);
			}
		}
	}

	// Envoyer l'EOI (End of Interrupt) au PIC
	pic8259_send_eoi(1);
}