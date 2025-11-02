#include "echo.hpp"
#include "terminal/terminal.hpp"

namespace Command::Echo {

int execute(int argc, char* argv[]) {
    // O loop começa em 1 para ignorar o nome do próprio comando ("echo")
    for (int i = 1; i < argc; i++) {
        Terminal::print(argv[i]);
        if (i < argc - 1) {
            Terminal::print(" "); // Adiciona um espaço entre os argumentos
        }
    }
    Terminal::print("\n");
    return 0;
}

} // namespace Command::Echo
