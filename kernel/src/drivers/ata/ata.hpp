// Em kernel/src/drivers/ata/ata.hpp

#ifndef DRIVER_ATA_HPP
#define DRIVER_ATA_HPP

#include <cstdint> // Para uint8_t, uint16_t, etc.
#include <cstddef> // Para size_t

namespace ATA {

    // Inicializa o driver. Por enquanto, não faz muito, mas é bom ter.
    void init();

    // Lê um ou mais setores do disco.
    // Parâmetros:
    //   - lba: O "endereço" do setor no disco (Logical Block Address). É um número que vai de 0 até o fim do disco.
    //   - count: Quantos setores a gente quer ler de uma vez.
    //   - buffer: Um lugar na memória (um "carrinho de mão") pra gente colocar os dados que leu.
    // Retorna:
    //   - true se deu tudo certo, false se deu algum B.O. 😱
    bool read_sectors(uint32_t lba, uint8_t count, uint8_t* buffer);

    // Escreve um ou mais setores no disco.
    // É a mesma ideia do read, mas ao contrário! A gente pega os dados do buffer e joga no disco.
    // Retorna:
    //   - true se a escrita foi um sucesso, false se falhou.
    bool write_sectors(uint32_t lba, uint8_t count, const uint8_t* buffer);

} // namespace ATA

#endif // DRIVER_ATA_HPP

