/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   PS2_8042.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vvaucoul <vvaucoul@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/24 11:14:27 by vvaucoul          #+#    #+#             */
/*   Updated: 2024/10/24 11:27:44 by vvaucoul         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef PS2_8042_H
#define PS2_8042_H

#include <stdint.h>

// Ports du contrôleur PS/2
#define PS2_DATA_PORT 0x60
#define PS2_STATUS_PORT 0x64
#define PS2_CMD_PORT 0x64

// Bits du registre d'état
#define PS2_OUTPUT_BUFFER_FULL 0x01
#define PS2_INPUT_BUFFER_FULL 0x02
#define PS2_SYSTEM_FLAG 0x04
#define PS2_COMMAND_DATA 0x08
#define PS2_MOUSE_DATA 0x20

// Commandes du contrôleur PS/2
#define PS2_CMD_READ_CONFIG 0x20
#define PS2_CMD_WRITE_CONFIG 0x60
#define PS2_CMD_DISABLE_MOUSE 0xA7
#define PS2_CMD_ENABLE_MOUSE 0xA8
#define PS2_CMD_TEST_MOUSE 0xA9
#define PS2_CMD_TEST_CONTROLLER 0xAA
#define PS2_CMD_SELF_TEST 0xAA
#define PS2_CMD_DISABLE_KEYBOARD 0xAD
#define PS2_CMD_ENABLE_KEYBOARD 0xAE

// Réponses du contrôleur
#define PS2_RESPONSE_ACK 0xFA
#define PS2_RESPONSE_RESEND 0xFE
#define PS2_RESPONSE_ERROR 0xFC

// Type de gestionnaire de scancode
typedef void (*ps2_handler_t)(uint8_t scancode);

// Structure pour stocker l'état du contrôleur PS/2
typedef struct {
	uint8_t config;
} ps2_controller_t;

// Initialisation du pilote PS/2
int ps2_init();

// Envoi d'une commande au contrôleur PS/2
int ps2_send_command(uint8_t cmd);

// Lecture des données du port de données PS/2
int ps2_read_data(uint8_t *data);

// Gestionnaire d'interruption PS/2
void ps2_interrupt_handler();

// Enregistrement d'un gestionnaire de scancode
int ps2_register_handler(ps2_handler_t handler);

#endif /* !PS2_8042_H */