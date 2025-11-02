#include "clear.hpp"
#include "xldgl/graphics.hpp" // Para poder limpar a tela
#include "terminal.hpp" // Para poder interagir com o terminal (ex: resetar cursor)

namespace Command::Clear {

int execute(int argc, char* argv[]) {
    // O comando 'clear' é simples, ele ignora os argumentos (argc, argv).
    
    // Pega o renderizador global
    auto& renderer = GFX::get_global_renderer();
    
    // Define a cor de fundo do terminal e limpa a tela
    renderer.setPenColor(GFX::Colors::Black); // Ou a cor de fundo que a gente definir para o terminal
    renderer.clearScreen();
    
    // Reseta a posição do cursor do terminal para o canto superior esquerdo
    Terminal::reset_cursor();
    
    return 0; // Retorna 0 para indicar sucesso!
}

} // namespace Command::Clear
