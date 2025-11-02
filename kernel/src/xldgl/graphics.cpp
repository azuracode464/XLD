#include "graphics.hpp"
#include <new> // Essencial para o "placement new"

// Para abs() sem a biblioteca padrão completa
static int abs(int n) {
    return (n < 0) ? -n : n;
}

// --- Definição da Fonte 8x16 ---
// Namespace anônimo para manter a fonte como um detalhe de implementação interno.
namespace {
    #define FONT8x16_IMPLEMENTATION
    #include "font.h" // Sua fonte abençoada! 😈
}

// --- Implementação do Namespace GFX ---
namespace GFX {

// --- Estratégia de Inicialização do Singleton para Kernel ---

// 1. Ponteiro para a instância global. Começa como nulo.
static GraphicsContext* g_global_renderer = nullptr;

// 2. Reservamos um espaço na memória (no segmento .bss) para o nosso objeto.
//    'alignas' garante que a memória terá o alinhamento correto para a classe,
//    evitando crashes em algumas arquiteturas.
alignas(GraphicsContext) static char g_renderer_storage[sizeof(GraphicsContext)];

// Implementação da função de inicialização global
void init(limine_framebuffer* fb) {
    // Só inicializa se ainda não tiver sido feito.
    if (g_global_renderer == nullptr) {
        // "Placement new": Constrói o objeto GraphicsContext diretamente
        // na memória que reservamos em 'g_renderer_storage'.
        // Isso nos dá o controle de onde o objeto vive, sem usar o heap (que não temos ainda).
        g_global_renderer = new (g_renderer_storage) GraphicsContext(fb);
    }
}

// Retorna a instância global.
GraphicsContext& get_global_renderer() {
    // Em um OS real, você poderia ter um 'panic' aqui se g_global_renderer for nullptr.
    // Isso significaria que o kernel tentou desenhar antes de inicializar o vídeo.
    return *g_global_renderer;
}


// --- Implementação dos Métodos da Classe GraphicsContext ---

GraphicsContext::GraphicsContext(limine_framebuffer* fb) {
    m_address = static_cast<uint32_t*>(fb->address);
    m_width = fb->width;
    m_height = fb->height;
    m_pitch = fb->pitch;
    m_penColor = Colors::White; // Cor padrão
}

void GraphicsContext::setPenColor(Color color) {
    m_penColor = color;
}

Color GraphicsContext::getPenColor() const {
    return m_penColor;
}

uint32_t GraphicsContext::getWidth() const {
    return m_width;
}

uint32_t GraphicsContext::getHeight() const {
    return m_height;
}

void GraphicsContext::clearScreen() {
    // Acesso direto à memória para máxima performance.
    // A divisão por 4 é porque o pitch é em bytes, e nosso ponteiro é de 32 bits (4 bytes).
    for (uint32_t y = 0; y < m_height; ++y) {
        for (uint32_t x = 0; x < m_width; ++x) {
            m_address[y * (m_pitch / 4) + x] = m_penColor;
        }
    }
}

void GraphicsContext::drawPixel(int x, int y) {
    // Checagem de limites para evitar desenhar fora da tela.
    if (x < 0 || static_cast<uint32_t>(x) >= m_width || y < 0 || static_cast<uint32_t>(y) >= m_height) {
        return;
    }
    m_address[y * (m_pitch / 4) + x] = m_penColor;
}

void GraphicsContext::drawPixel(int x, int y, Color color) {
    if (x < 0 || static_cast<uint32_t>(x) >= m_width || y < 0 || static_cast<uint32_t>(y) >= m_height) {
        return;
    }
    m_address[y * (m_pitch / 4) + x] = color;
}

void GraphicsContext::drawLine(int x0, int y0, int x1, int y1) {
    // Algoritmo de Linha de Bresenham
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    for (;;) {
        drawPixel(x0, y0);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void GraphicsContext::fillRect(int x, int y, int width, int height) {
    for (int i = y; i < y + height; ++i) {
        for (int j = x; j < x + width; ++j) {
            drawPixel(j, i);
        }
    }
}

void GraphicsContext::drawRect(int x, int y, int width, int height) {
    drawLine(x, y, x + width - 1, y);
    drawLine(x, y + height - 1, x + width - 1, y + height - 1);
    drawLine(x, y, x, y + height - 1);
    drawLine(x + width - 1, y, x + width - 1, y + height - 1);
}

void GraphicsContext::drawCircle(int centerX, int centerY, int radius) {
    // Algoritmo de Círculo de Bresenham (Midpoint)
    int x = radius, y = 0;
    int err = 0;

    while (x >= y) {
        drawPixel(centerX + x, centerY + y);
        drawPixel(centerX + y, centerY + x);
        drawPixel(centerX - y, centerY + x);
        drawPixel(centerX - x, centerY + y);
        drawPixel(centerX - x, centerY - y);
        drawPixel(centerX - y, centerY - x);
        drawPixel(centerX + y, centerY - x);
        drawPixel(centerX + x, centerY - y);

        if (err <= 0) {
            y += 1;
            err += 2 * y + 1;
        }
        if (err > 0) {
            x -= 1;
            err -= 2 * x + 1;
        }
    }
}

void GraphicsContext::fillCircle(int centerX, int centerY, int radius) {
    // Algoritmo simples, mas funcional. Pode ser otimizado depois.
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            if (x * x + y * y <= radius * radius) {
                drawPixel(centerX + x, centerY + y);
            }
        }
    }
}

void GraphicsContext::drawChar(int x, int y, char c) {
    // Pega o glifo do caractere na nossa fonte.
    const unsigned char* glyph = font8x16[static_cast<unsigned char>(c)];
    for (int i = 0; i < FONT_HEIGHT; ++i) {
        for (int j = 0; j < FONT_WIDTH; ++j) {
            // Verifica bit a bit se o pixel deve ser desenhado.
            if ((glyph[i] >> (7 - j)) & 1) {
                drawPixel(x + j, y + i);
            }
        }
    }
}

void GraphicsContext::drawString(int x, int y, const char* str) {
    for (int i = 0; str[i] != '\0'; ++i) {
        drawChar(x + (i * FONT_WIDTH), y, str[i]);
    }
}

} // namespace GFX

