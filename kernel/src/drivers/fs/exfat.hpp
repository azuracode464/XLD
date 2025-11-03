// Em kernel/src/drivers/fs/exfat.hpp

#ifndef FS_EXFAT_HPP
#define FS_EXFAT_HPP

#include <cstdint>

namespace exFAT {

    // --- O SETOR DE BOOT ---
    struct __attribute__((packed)) BootSector {
        uint8_t  jump_boot[3];
        char     file_system_name[8];
        uint8_t  must_be_zero[53];
        uint64_t partition_offset;
        uint64_t volume_length;
        uint32_t fat_offset;
        uint32_t fat_length;
        uint32_t cluster_heap_offset;
        uint32_t cluster_count;
        uint32_t root_dir_cluster;
        uint32_t volume_serial_number;
        uint16_t file_system_revision;
        uint16_t volume_flags;
        uint8_t  bytes_per_sector_shift;
        uint8_t  sectors_per_cluster_shift;
        uint8_t  num_fats;
        uint8_t  drive_select;
        uint8_t  percent_in_use;
        uint8_t  reserved[7];
        uint8_t  boot_code[390];
        uint16_t boot_signature;
    };

    // --- AS ENTRADAS DE DIRETÓRIO ---
    enum class EntryType : uint8_t {
        AllocationBitmap = 0x81,
        UpcaseTable      = 0x82,
        VolumeLabel      = 0x83,
        File             = 0x85,
        StreamExtension  = 0xC0,
        FileName         = 0xC1,
    };

    struct __attribute__((packed)) GenericDirectoryEntry {
        EntryType type;
        uint8_t   data[31];
    };

    struct __attribute__((packed)) FileDirectoryEntry {
        EntryType type;
        uint8_t   secondary_count;
        uint16_t  set_checksum;
        uint16_t  file_attributes;
        uint16_t  reserved1;
        uint32_t  created_timestamp;
        uint32_t  last_modified_timestamp;
        uint32_t  last_accessed_timestamp;
        uint8_t   created_10ms_increment;
        uint8_t   last_modified_10ms_increment;
        uint8_t   created_utc_offset;
        uint8_t   last_modified_utc_offset;
        uint8_t   last_accessed_utc_offset;
        uint8_t   reserved2[7];
    };

    struct __attribute__((packed)) StreamExtensionDirectoryEntry {
        EntryType type;
        uint8_t   general_flags;
        uint8_t   reserved1;
        uint8_t   name_length;
        uint16_t  name_hash;
        uint16_t  reserved2;
        uint64_t  valid_data_length;
        uint32_t  reserved3;
        uint32_t  first_cluster;
        uint64_t  data_length;
    };

    struct __attribute__((packed)) FileNameDirectoryEntry {
        EntryType type;
        uint8_t   general_flags;
        uint16_t  file_name[15];
    };

    // --- FUNÇÕES PÚBLICAS ---
    bool initialize_volume();
    uint32_t cluster_to_lba(uint32_t cluster);
    bool read_cluster(uint32_t cluster_num, uint8_t* buffer);
    uint32_t get_next_cluster(uint32_t current_cluster);
    uint32_t get_root_dir_cluster();
    uint32_t get_sectors_per_cluster();
    uint8_t* read_file(const char* path, uint32_t& file_size);
    void free_file_buffer(); // ← ADICIONE ESTA LINHA

} // namespace exFAT

#endif // FS_EXFAT_HPP
