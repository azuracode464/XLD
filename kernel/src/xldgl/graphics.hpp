#ifndef GRAPHICS_HPP
#define GRAPHICS_HPP

#include <stdint.h>
#include <stddef.h>

// O bootloader Limine nos passa uma estrutura C.
// Precisamos dizer ao compilador C++ para não fazer "name mangling" nela.
extern "C" {
    #include <limine.h>
}

// Namespace para toda a nossa biblioteca gráfica
namespace GFX {

// Dimensões da nossa fonte bitmap
constexpr int FONT_WIDTH = 8;
constexpr int FONT_HEIGHT = 16;

// Tipo para representar cores no formato 0xAARRGGBB
using Color = uint32_t;

// --- Cores Pré-definidas ---
namespace Colors {
    constexpr Color Black       = 0x00000000;
    constexpr Color White       = 0x00FFFFFF;
    constexpr Color Red         = 0x00FF0000;
    constexpr Color Green       = 0x0000FF00;
    constexpr Color Blue        = 0x000000FF;
    constexpr Color Yellow      = 0x00FFFF00;
    constexpr Color Cyan        = 0x0000FFFF;
    constexpr Color Magenta     = 0x00FF00FF;
    constexpr Color DarkGray    = 0x00555555;
    constexpr Color LightGray   = 0x00AAAAAA;
}

// A classe principal que representa nosso contexto de desenho.
// Ela detém o controle sobre o framebuffer e o estado do desenho.
class GraphicsContext {
public:
    // Construtor: inicializa o contexto com as informações do framebuffer
    GraphicsContext(limine_framebuffer* fb);

    // Proibimos a cópia para evitar ter múltiplos contextos para o mesmo framebuffer
    GraphicsContext(const GraphicsContext&) = delete;
    GraphicsContext& operator=(const GraphicsContext&) = delete;

    // --- Funções de Configuração ---
    void setPenColor(Color color);
    Color getPenColor() const;
    uint32_t getWidth() const;
    uint32_t getHeight() const;

    // --- Funções de Desenho Primitivo ---
    void clearScreen();
    void drawPixel(int x, int y);
    void drawPixel(int x, int y, Color color); // Sobrecarga para desenhar com uma cor específica

    // --- Funções de Desenho de Formas ---
    void drawLine(int x0, int y0, int x1, int y1);
    void drawRect(int x, int y, int width, int height);
    void fillRect(int x, int y, int width, int height);
    void drawCircle(int centerX, int centerY, int radius);
    void fillCircle(int centerX, int centerY, int radius);

    // --- Funções de Desenho de Texto ---
    void drawChar(int x, int y, char c);
    void drawString(int x, int y, const char* str);

private:
    uint32_t* m_address;    // Endereço do framebuffer (m_ para "member")
    uint32_t  m_width;
    uint32_t  m_height;
    uint32_t  m_pitch;      // Pitch em bytes
    Color     m_penColor;   // Cor atual da caneta
};

// Função para obter uma instância global do nosso contexto gráfico.
// Isso garante que ele seja inicializado apenas uma vez.
GraphicsContext& get_global_renderer();

// Função de inicialização global, a ser chamada uma vez no kernel.
void init(limine_framebuffer* fb);

} // namespace GFX

#endif // GRAPHICS_HPP

