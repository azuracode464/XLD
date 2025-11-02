// Em kernel/src/drivers/keyboard.hpp

#ifndef KEYBOARD_HPP
#define KEYBOARD_HPP

namespace Keyboard {

// Enum para teclas especiais que não são caracteres imprimíveis
enum class SpecialKey {
    None,
    Enter,
    Backspace,
    ArrowUp,
    ArrowDown,
    ArrowLeft,
    ArrowRight,
    // ... podemos adicionar mais depois
};

// Estrutura para representar uma tecla pressionada
struct KeyEvent {
    char character;      // Se for 'a', 'b', '1', etc., fica aqui. Se não, fica 0.
    SpecialKey special;  // Se for Enter, Backspace, etc., fica aqui.
};

// Inicializa o driver de teclado (se necessário)
void init();

// Espera por uma tecla e a retorna.
// Esta função vai ficar "presa" (bloqueada) até o usuário digitar algo.
KeyEvent wait_for_key();

} // namespace Keyboard

#endif // KEYBOARD_HPP

