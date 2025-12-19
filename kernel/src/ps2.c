#include <stdint.h>
#include <stdbool.h>

extern void printk(const char *fmt, ...);

#define PS2_DATA_PORT 0x60
#define PS2_STATUS_PORT 0x64
#define PS2_COMMAND_PORT 0x64

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

/* Esperas utilitárias para o controlador PS/2 */
static inline void wait_input_empty(void) {
    while (inb(PS2_STATUS_PORT) & 0x02) { asm volatile("nop"); }
}

static inline void wait_output_full(void) {
    while (!(inb(PS2_STATUS_PORT) & 0x01)) { asm volatile("nop"); }
}

/* Mapas básicos de scancodes (set 1) para ASCII (simplificado) */
static const char scancode_to_ascii[] = {
    0,  0, '1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',0,
    'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\','z','x',
    'c','v','b','n','m',',','.','/',0,'*',0,' ',
};

static const char scancode_to_ascii_shift[] = {
    0, 0, '!','@','#','$','%','^','&','*','(',')','_','+','\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',0,
    'A','S','D','F','G','H','J','K','L',':','"','~',0,'|','Z','X',
    'C','V','B','N','M','<','>','?',0,'*',0,' ',
};

static bool shift_down = false;
static bool extended = false;

void ps2_init(void) {
    printk("[ps2] init start\n");

    wait_input_empty();
    outb(PS2_COMMAND_PORT, 0x20);         // read controller config
    wait_output_full();
    uint8_t config = inb(PS2_DATA_PORT);

    config |= 0x01;   // habilita IRQ1 (porta 1)
    config &= ~0x10;  // desabilita translation

    wait_input_empty();
    outb(PS2_COMMAND_PORT, 0x60);         // write controller config
    wait_input_empty();
    outb(PS2_DATA_PORT, config);

    /* Habilitar scanning no teclado (comando 0xF4) */
    wait_input_empty();
    outb(PS2_DATA_PORT, 0xF4);

    /* Leitura opcional de ACK (0xFA) se estiver disponível */
    if (inb(PS2_STATUS_PORT) & 0x01) {
        uint8_t maybe = inb(PS2_DATA_PORT);
        printk("[ps2] ack/byte: 0x%x\n", maybe);
    }

    printk("[ps2] init done (config=0x%x)\n", config);
}

void ps2_handler(void) {
    if (!(inb(PS2_STATUS_PORT) & 0x01)) {
        return;
    }

    uint8_t sc = inb(PS2_DATA_PORT);

    /* Debug: imprimir scancode cru */
    printk("[ps2] scancode: 0x%x\n", sc);

    if (sc == 0xE0) {
        extended = true;
        return;
    }

    bool is_break = (sc & 0x80) != 0;
    uint8_t code = sc & 0x7F;

    if (code == 0x2A || code == 0x36) {
        if (is_break) shift_down = false;
        else shift_down = true;
        extended = false;
        return;
    }

    if (extended) {
        extended = false;
        return;
    }

    if (is_break) {
        return;
    }

    const char *map = shift_down ? scancode_to_ascii_shift : scancode_to_ascii;
    if (code < sizeof(scancode_to_ascii)) {
        char c = map[code];
        if (c != 0) {
            printk("%c", c);
        } else {
            /* opcional: mostrar código não mapeado */
            printk("[ps2:unk 0x%x]", code);
        }
    }
}
