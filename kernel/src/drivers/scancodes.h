// kernel/src/drivers/scancodes.h

#ifndef SCANCODES_H
#define SCANCODES_H

#include <stdint.h>

#define KEY_RELEASE_OFFSET 0x80 // Scancodes de liberação são o scancode de pressão + 0x80

// Caracteres não imprimíveis
#define KEY_LSHIFT 0x2A
#define KEY_RSHIFT 0x36
#define KEY_ENTER  0x1C
#define KEY_BKSP   0x0E
#define KEY_SPACE  0x39
#define KEY_CAPS   0x3A
#define KEY_LCTRL  0x1D // <<< ESTA É A LINHA QUE FALTAVA
// Tabela de mapeamento de Scancode Set 1 para ASCII.
// O índice do array é o próprio scancode.
const uint8_t scancode_to_ascii[] = {
    0,    27,  '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8',  '9',  '0',  '-',  '=', '\b', // 0x00 - 0x0E
  '\t',  'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',  'o',  'p',  '[',  ']', '\n',   // 0x0F - 0x1C
     0,  'a',  's',  'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';', '\'',  '`',    0,   // 0x1D - 0x2A
  '\\',  'z',  'x',  'c',  'v',  'b',  'n',  'm',  ',',  '.',  '/',    0,   '*',    0,   // 0x2B - 0x38
   ' ',    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,   // 0x39 - 0x46
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,   // 0x47 - 0x53
    0,    0,    0,    0                                                             // 0x54 - 0x57
};

// Tabela para quando a tecla Shift está pressionada.
const uint8_t scancode_to_ascii_shift[] = {
    0,    27,  '!',  '@',  '#',  '$',  '%',  '^',  '&',  '*',  '(',  ')',  '_',  '+', '\b',
  '\t',  'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',  'O',  'P',  '{',  '}', '\n',
     0,  'A',  'S',  'D',  'F',  'G',  'H',  'J',  'K',  'L',  ':',  '"',  '~',    0,
  '|',  'Z',  'X',  'C',  'V',  'B',  'N',  'M',  '<',  '>',  '?',    0,   '*',    0,
   ' ',    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0
};

#endif // SCANCODES_H

