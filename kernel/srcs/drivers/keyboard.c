/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   keyboard.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vvaucoul <vvaucoul@student.42.Fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/06/22 13:56:07 by vvaucoul          #+#    #+#             */
/*   Updated: 2022/06/27 13:04:48 by vvaucoul         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/drivers/keyboard.h"

#include "../../includes/shell/kshell.h"
#include "../../includes/shell/kshell_termcaps.h"

unsigned char kbdus[128] =
    {
        0, 27, '1', '2', '3', '4', '5', '6', '7', '8',    /* 9 */
        '9', '0', '-', '=', '\b',                         /* Backspace */
        '\t',                                             /* Tab */
        'q', 'w', 'e', 'r',                               /* 19 */
        't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',     /* Enter key */
        0,                                                /* 29   - Control */
        'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', /* 39 */
        '\'', '`', 0,                                     /* Left shift */
        '\\', 'z', 'x', 'c', 'v', 'b', 'n',               /* 49 */
        'm', ',', '.', '/', 0,                            /* Right shift */
        '*',
        0,   /* Alt */
        ' ', /* Space bar */
        0,   /* Caps lock */
        0,   /* 59 - F1 key ... > */
        0, 0, 0, 0, 0, 0, 0, 0,
        0, /* < ... F10 */
        0, /* 69 - Num lock*/
        0, /* Scroll Lock */
        0, /* Home key */
        0, /* Up Arrow */
        0, /* Page Up */
        '-',
        0, /* Left Arrow */
        0,
        0, /* Right Arrow */
        '+',
        0, /* 79 - End key*/
        0, /* Down Arrow */
        0, /* Page Down */
        0, /* Insert Key */
        0, /* Delete Key */
        0, 0, 0,
        0, /* F11 Key */
        0, /* F12 Key */
        0, /* All other keys are undefined */
};

static bool scancode_handler(unsigned char scancode)
{
    switch (scancode)
    {
        /* TERMINAL MODE */
        // case KEYBOARD_KEY_ESCAPE:
        //     reboot();
        //     return (true);
        // case KEYBOARD_KEY_BACK:
        //     terminal_back_once();
        //     return (true);
        // case KEYBOARD_KEY_ARROW_LEFT:
        // case KEYBOARD_KEY_ARROW_RIGHT:
        //     return (true);
        // case KEYBOARD_KEY_ARROW_DOWN:
        //     terminal_cursor_up();
        //     return (true);
        // case KEYBOARD_KEY_ARROW_TOP:
        //     terminal_cursor_down();
        //     return (true);

    /* KSHELL MODE */
    case KEYBOARD_KEY_ESCAPE:
        poweroff();
        return (true);
    case KEYBOARD_KEY_BACK:
        kshell_del_one();
        return (true);
    case KEYBOARD_KEY_ARROW_LEFT:
        kshell_move_cursor_left();
        return (true);
    case KEYBOARD_KEY_ARROW_RIGHT:
        kshell_move_cursor_right();
        return (true);
    case KEYBOARD_KEY_ARROW_DOWN:
        kshell_move_cursor_down();
        return (true);
    case KEYBOARD_KEY_ARROW_TOP:
        kshell_move_cursor_up();
        return (true);
    case KEYBOARD_KEY_ENTER:
        kshell_new_line();
        return (true);
    case KEYBOARD_KEY_SUPPR:
        kshell_suppr_char();
        return (true);
    case KEYBOARD_SCREEN_F1:
        reboot();
        return (true);
    default:
        break;
    }
    return (false);
}

void keyboard_handler(struct regs *r)
{
    (void)r;
    unsigned char scancode;

    /* Read from the keyboard's data buffer */
    scancode = inportb(0x60);

    /* If the top bit of the byte we read from the keyboard is
     *  set, that means that a key has just been released */
    if (scancode & 0x80)
    {
        /* You can use this one to see if the user released the
         *  shift, alt, or control keys... */
    }
    else
    {
        /* Here, a key was just pressed. Please note that if you
         *  hold a key down, you will get repeated key press
         *  interrupts. */

        /* Just to show you how this works, we simply translate
         *  the keyboard scancode into an ASCII value, and then
         *  display it to the screen. You can get creative and
         *  use some flags to see if a shift is pressed and use a
         *  different layout, or you can add another 128 entries
         *  to the above layout to correspond to 'shift' being
         *  held. If shift is held using the larger lookup table,
         *  you would add 128 to the scancode when you look for it */
        if ((scancode_handler(scancode)) == false)
        {
            kshell_write_char(kbdus[scancode]);
        }
    }
}

/* Installs the keyboard handler into IRQ1 */
void keyboard_install()
{
    irq_install_handler(1, keyboard_handler);
}
