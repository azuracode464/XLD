#include "terminal.hpp"
#include "drivers/keyboard/keyboard.hpp"
#include "xldgl/graphics.hpp"
#include <cstddef> // Para std::size_t

// --- INCLUSÃO DOS NOSSOS PLUGINS DE COMANDO ---
// O terminal agora só precisa saber que eles existem.
#include "commands/command.hpp"
#include "commands/clear/clear.hpp"
#include "commands/help/help.hpp"
#include "commands/uname/uname.hpp"
#include "commands/echo/echo.hpp"
#include "commands/reboot/reboot.hpp"

// --- Variáveis e Estruturas Internas do Terminal ---
namespace {
    // Estrutura para registrar um comando
    struct RegisteredCommand {
        const char* name;
        CommandExecutor execute;
    };

    // A LISTA MESTRA DE COMANDOS DO xldOS!
    // Para adicionar um comando, basta criar os arquivos e adicionar uma linha aqui.
    RegisteredCommand g_commands[] = {
        {"clear",  Command::Clear::execute},
        {"help",   Command::Help::execute},
        {"uname",  Command::Uname::execute},
        {"echo",   Command::Echo::execute},
        {"reboot", Command::Reboot::execute}
    };
    const std::size_t g_num_commands = sizeof(g_commands) / sizeof(RegisteredCommand);

    // Buffer para a linha de comando atual
    char g_command_buffer[256];
    std::size_t g_buffer_index = 0;

    // Posição do cursor na tela
    int g_cursor_x = 0;
    int g_cursor_y = 0;

    GFX::GraphicsContext* g_renderer;

    // --- Funções Auxiliares Internas ---

    void put_char(char c) {
        if (c == '\n') {
            g_cursor_y += GFX::FONT_HEIGHT;
            g_cursor_x = 0;
        } else {
            char str[2] = {c, '\0'};
            g_renderer->drawString(g_cursor_x, g_cursor_y, str);
            g_cursor_x += GFX::FONT_WIDTH;
        }
        // TODO: Implementar a rolagem da tela (scrolling) quando o cursor_y passar da altura da tela.
    }

    // strcmp simples, já que não temos a biblioteca C padrão completa.
    bool strcmp(const char* a, const char* b) {
        while (*a != '\0' && *b != '\0') {
            if (*a++ != *b++) {
                return false;
            }
        }
        return *a == *b;
    }

    // Função que analisa a linha de comando e chama o comando certo
    void process_command(char* buffer) {
        char* argv[32]; // Suporta até 32 argumentos
        int argc = 0;

        char* current_arg = buffer;
        while (*current_arg != '\0' && argc < 31) {
            while (*current_arg == ' ') *current_arg++ = '\0'; // Remove espaços e separa
            if (*current_arg == '\0') break;

            argv[argc++] = current_arg;

            while (*current_arg != ' ' && *current_arg != '\0') current_arg++;
        }
        argv[argc] = nullptr;

        if (argc == 0) return;

        // Procura o comando (argv[0]) na nossa lista
        for (std::size_t i = 0; i < g_num_commands; ++i) {
            if (strcmp(argv[0], g_commands[i].name)) {
                // ACHOU! Executa o comando com os argumentos corretos!
                g_commands[i].execute(argc, argv);
                return;
            }
        }

        Terminal::print("Comando nao encontrado: ");
        Terminal::print(argv[0]);
    }
}

// --- Implementação das Funções Públicas do Terminal ---
namespace Terminal {

    void init() {
        g_renderer = &GFX::get_global_renderer();
        g_renderer->setPenColor(GFX::Colors::Black);
        g_renderer->clearScreen();
        reset_cursor();

        g_renderer->setPenColor(GFX::Colors::White);
        print("Bem-vindo ao xldOS v0.3 - Arquiteto Edition!\n");
        print("xldOS> ");
    }

    void print(const char* str) {
        for (int i = 0; str[i] != '\0'; ++i) {
            put_char(str[i]);
        }
    }

    void reset_cursor() {
        g_cursor_x = 0;
        g_cursor_y = 0;
    }

    void run() {
        for (;;) {
            Keyboard::KeyEvent key = Keyboard::wait_for_key();

            if (key.special == Keyboard::SpecialKey::Enter) {
                put_char('\n');
                g_command_buffer[g_buffer_index] = '\0';
                
                if (g_buffer_index > 0) {
                    process_command(g_command_buffer);
                }

                print("\nxldOS> ");
                g_buffer_index = 0;
            }
            else if (key.special == Keyboard::SpecialKey::Backspace) {
                if (g_buffer_index > 0) {
                    g_buffer_index--;
                    g_cursor_x -= GFX::FONT_WIDTH;
                    g_renderer->setPenColor(GFX::Colors::Black);
                    g_renderer->fillRect(g_cursor_x, g_cursor_y, GFX::FONT_WIDTH, GFX::FONT_HEIGHT);
                    g_renderer->setPenColor(GFX::Colors::White);
                }
            }
            else if (key.character != 0) {
                if (g_buffer_index < (sizeof(g_command_buffer) - 1)) {
                    put_char(key.character);
                    g_command_buffer[g_buffer_index] = key.character;
                    g_buffer_index++;
                }
            }
        }
    }

} // namespace Terminal

