// Em kernel/src/apps/terminal/commands/command.hpp

#ifndef COMMAND_HPP
#define COMMAND_HPP

// Todo comando deve ter uma função 'execute' que recebe os argumentos
// e retorna 0 para sucesso ou um código de erro.
// 'argc' é a contagem de argumentos, 'argv' são os argumentos.
// Ex: "echo ola mundo" -> argc=3, argv={"echo", "ola", "mundo"}
using CommandExecutor = int (*)(int argc, char* argv[]);

#endif // COMMAND_HPP

