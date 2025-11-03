// Em kernel/src/drivers/fs/exfat.cpp

#include "exfat.hpp"
#include "drivers/ata/ata.hpp"
#include "terminal/terminal.hpp"
#include <cstddef>

// memcpy simples
static void* memcpy(void* dest, const void* src, size_t n) {
    for (size_t i = 0; i < n; i++) {
        ((char*)dest)[i] = ((char*)src)[i];
    }
    return dest;
}

namespace exFAT {

    // --- Variáveis Globais do Módulo ---
    static bool g_volume_initialized = false;
    static BootSector g_boot_sector;
    static uint32_t g_bytes_per_sector;
    static uint32_t g_sectors_per_cluster;
    static uint32_t g_first_data_sector;
    static uint32_t g_fat_lba;

    // Buffer estático para leitura de arquivos (1MB)
    static uint8_t g_file_buffer[1024 * 1024];
    static bool g_file_buffer_used = false;

    // --- Implementação das Funções ---

    bool initialize_volume() {
        if (g_volume_initialized) {
            return true;
        }

        uint8_t sector_buffer[512];
        if (!ATA::read_sectors(0, 1, sector_buffer)) {
            return false;
        }

        memcpy(&g_boot_sector, sector_buffer, sizeof(BootSector));

        // Validações
        if (g_boot_sector.boot_signature != 0xAA55) return false;
        const char* expected_name = "EXFAT   ";
        for(int i=0; i<8; i++) {
            if (g_boot_sector.file_system_name[i] != expected_name[i]) return false;
        }

        // Calcula valores
        g_bytes_per_sector = 1 << g_boot_sector.bytes_per_sector_shift;
        g_sectors_per_cluster = 1 << g_boot_sector.sectors_per_cluster_shift;
        g_first_data_sector = g_boot_sector.partition_offset + g_boot_sector.cluster_heap_offset;
        g_fat_lba = g_boot_sector.partition_offset + g_boot_sector.fat_offset;

        g_volume_initialized = true;
        return true;
    }

    uint32_t cluster_to_lba(uint32_t cluster) {
        if (!g_volume_initialized || cluster < 2) {
            return 0;
        }
        return g_first_data_sector + (cluster - 2) * g_sectors_per_cluster;
    }

    bool read_cluster(uint32_t cluster_num, uint8_t* buffer) {
        if (!g_volume_initialized) return false;

        uint32_t lba = cluster_to_lba(cluster_num);
        if (lba == 0) return false;

        return ATA::read_sectors(lba, g_sectors_per_cluster, buffer);
    }

    uint32_t get_next_cluster(uint32_t current_cluster) {
        if (!g_volume_initialized) return 0;

        // Cada entrada na FAT tem 4 bytes (32 bits)
        uint32_t fat_entry_offset = current_cluster * 4;
        uint32_t fat_sector = g_fat_lba + (fat_entry_offset / g_bytes_per_sector);
        uint32_t entry_in_sector_offset = fat_entry_offset % g_bytes_per_sector;

        uint8_t sector_buffer[512];
        if (!ATA::read_sectors(fat_sector, 1, sector_buffer)) {
            return 0xFFFFFFFF;
        }

        uint32_t next_cluster = *(uint32_t*)&sector_buffer[entry_in_sector_offset];
        return next_cluster;
    }

    uint32_t get_root_dir_cluster() {
        if (!g_volume_initialized) return 0;
        return g_boot_sector.root_dir_cluster;
    }

    uint32_t get_sectors_per_cluster() {
        if (!g_volume_initialized) return 0;
        return g_sectors_per_cluster;
    }

    // Função para converter número para string
    void uint32_to_string(uint32_t value, char* buffer) {
        char* ptr = buffer;
        if (value == 0) {
            *ptr++ = '0';
        } else {
            char temp[16];
            char* temp_ptr = temp + 15;
            *temp_ptr = '\0';
            
            while (value > 0) {
                *--temp_ptr = '0' + (value % 10);
                value /= 10;
            }
            
            while (*temp_ptr != '\0') {
                *ptr++ = *temp_ptr++;
            }
        }
        *ptr = '\0';
    }

    // Função para ler um arquivo completo
    uint8_t* read_file(const char* path, uint32_t& file_size) {
        if (!initialize_volume()) {
            Terminal::print("Erro: Falha ao inicializar volume exFAT\n");
            return nullptr;
        }

        // Verifica se o buffer já está em uso
        if (g_file_buffer_used) {
            Terminal::print("Erro: Buffer de arquivo ja esta em uso\n");
            return nullptr;
        }

        // Busca pelo arquivo no diretório raiz
        uint32_t current_cluster = get_root_dir_cluster();
        uint32_t sectors_per_cluster = get_sectors_per_cluster();
        uint8_t cluster_buffer[4096];

        // Pula o "/" inicial se existir
        const char* search_name = path;
        while (*search_name == '/') search_name++;

        Terminal::print("Procurando arquivo: ");
        Terminal::print(search_name);
        Terminal::print("\n");

        while (current_cluster < 0xFFFFFFF7) {
            if (!read_cluster(current_cluster, cluster_buffer)) {
                Terminal::print("Erro ao ler cluster do diretorio\n");
                return nullptr;
            }

            int entries_per_cluster = (512 * sectors_per_cluster) / 32;
            GenericDirectoryEntry* entry = (GenericDirectoryEntry*)cluster_buffer;

            for (int i = 0; i < entries_per_cluster; ++i, ++entry) {
                if (entry->type == EntryType::File) {
                    FileDirectoryEntry* file_entry = (FileDirectoryEntry*)entry;
                    StreamExtensionDirectoryEntry* stream_entry = (StreamExtensionDirectoryEntry*)(entry + 1);
                    
                    if (stream_entry->type != EntryType::StreamExtension) {
                        continue;
                    }

                    // Monta o nome do arquivo
                    char file_name[256];
                    int name_len = stream_entry->name_length;
                    int name_pos = 0;

                    for (int j = 0; j < file_entry->secondary_count - 1; ++j) {
                        FileNameDirectoryEntry* name_entry = (FileNameDirectoryEntry*)(entry + 2 + j);
                        if (name_entry->type != EntryType::FileName) break;

                        for (int k = 0; k < 15 && name_pos < name_len; ++k) {
                            // Conversão simples UTF-16 para ASCII
                            if (name_entry->file_name[k] < 128) {
                                file_name[name_pos++] = (char)name_entry->file_name[k];
                            } else {
                                file_name[name_pos++] = '?';
                            }
                        }
                    }
                    file_name[name_len] = '\0';

                    Terminal::print("Arquivo encontrado: ");
                    Terminal::print(file_name);
                    Terminal::print("\n");

                    // Compara com o nome procurado
                    bool match = true;
                    for (int c = 0; c < name_len; c++) {
                        if (file_name[c] != search_name[c]) {
                            match = false;
                            break;
                        }
                        if (search_name[c] == '\0') break;
                    }

                    if (match && search_name[name_len] == '\0') {
                        // Arquivo encontrado!
                        file_size = stream_entry->valid_data_length;
                        uint32_t first_cluster = stream_entry->first_cluster;
                        
                        // Verifica se o arquivo cabe no buffer
                        if (file_size > sizeof(g_file_buffer)) {
                            Terminal::print("Erro: Arquivo muito grande para o buffer\n");
                            return nullptr;
                        }

                        // Converte números para string para imprimir
                        char size_str[16];
                        char cluster_str[16];
                        uint32_to_string(file_size, size_str);
                        uint32_to_string(first_cluster, cluster_str);
                        
                        Terminal::print("Tamanho do arquivo: ");
                        Terminal::print(size_str);
                        Terminal::print(" bytes\n");
                        Terminal::print("Primeiro cluster: ");
                        Terminal::print(cluster_str);
                        Terminal::print("\n");

                        // Marca o buffer como usado
                        g_file_buffer_used = true;

                        // Lê o arquivo cluster por cluster
                        uint32_t current_file_cluster = first_cluster;
                        uint32_t bytes_read = 0;
                        uint32_t cluster_size = 512 * sectors_per_cluster;
                        
                        while (current_file_cluster < 0xFFFFFFF7 && bytes_read < file_size) {
                            if (!read_cluster(current_file_cluster, cluster_buffer)) {
                                Terminal::print("Erro ao ler cluster de dados\n");
                                g_file_buffer_used = false;
                                return nullptr;
                            }

                            uint32_t bytes_to_copy = file_size - bytes_read;
                            if (bytes_to_copy > cluster_size) {
                                bytes_to_copy = cluster_size;
                            }

                            memcpy(g_file_buffer + bytes_read, cluster_buffer, bytes_to_copy);
                            bytes_read += bytes_to_copy;
                            
                            current_file_cluster = get_next_cluster(current_file_cluster);
                        }

                        // Converte bytes lidos para string
                        char bytes_str[16];
                        uint32_to_string(bytes_read, bytes_str);
                        Terminal::print("Arquivo lido com sucesso: ");
                        Terminal::print(bytes_str);
                        Terminal::print(" bytes\n");
                        return g_file_buffer;
                    }

                    // Pula entradas secundárias já processadas
                    i += file_entry->secondary_count;
                    entry += file_entry->secondary_count;
                }
            }

            current_cluster = get_next_cluster(current_cluster);
        }

        Terminal::print("Arquivo nao encontrado\n");
        return nullptr;
    }

    // Função para liberar o buffer de arquivo
    void free_file_buffer() {
        g_file_buffer_used = false;
    }

} // namespace exFAT
