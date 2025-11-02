#ifndef TERMINAL_HPP
#define TERMINAL_HPP

namespace Terminal {

// Inicializa o terminal (limpa a tela, mostra mensagem de boas-vindas)
void init();

// A função principal que nunca retorna. O coração do nosso terminal.
void run();

// --- Funções de utilidade para os comandos ---

// Imprime uma string na posição atual do cursor
void print(const char* str);

// Reseta a posição do cursor para o canto superior esquerdo (0, 0)
// O comando 'clear' vai usar isso!
void reset_cursor();

} // namespace Terminal

#endif // TERMINAL_HPP

