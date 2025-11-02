#ifndef COMMAND_CLEAR_HPP
#define COMMAND_CLEAR_HPP

#include "../command.hpp" // Inclui a definição padrão de um comando

namespace Command::Clear {

// A declaração da nossa função 'execute'.
// O terminal vai chamar isso quando o usuário digitar "clear".
int execute(int argc, char* argv[]);

} // namespace Command::Clear

#endif // COMMAND_CLEAR_HPP

