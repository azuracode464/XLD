#include "help.hpp"
#include "terminal/terminal.hpp"

namespace Command::Help {

int execute(int argc, char* argv[]) {
    Terminal::print("Comandos disponiveis no xldOS:\n");
    Terminal::print("  clear  - Limpa a tela do terminal.\n");
    Terminal::print("  echo   - Imprime um texto na tela.\n");
    Terminal::print("  help   - Mostra esta mensagem de ajuda.\n");
    Terminal::print("  reboot - Reinicia o computador.\n");
    Terminal::print("  uname  - Mostra a versao do sistema.\n");
    return 0;
}

} // namespace Command::Help
