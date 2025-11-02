#include "uname.hpp"
#include "terminal/terminal.hpp" // Para usar Terminal::print

namespace Command::Uname {

int execute(int argc, char* argv[]) {
    // Ignora os argumentos
    Terminal::print("xldOS version 0.3 - Arquiteto Edition\n");
    return 0; // Sucesso
}

} // namespace Command::Uname
