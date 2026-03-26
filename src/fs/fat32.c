#include "fs/fat32.h"
#include "utils/malloc.h"
#include "VGA/videodriver.h"

fat32_bpb_t* main_bpb;
uint32_t data_region_start;

// 1. A FUNÇÃO DEVE FICAR AQUI (Antes da fat32_find_file_cluster)
int stellar_name_check(char* s1, char* s2) {
    for(int i = 0; i < 11; i++) {
        if(s1[i] != s2[i]) return 0;
    }
    return 1;
}

void fat32_init() {
    main_bpb = (fat32_bpb_t*) kmalloc(512);
    read_sectors(0, 1, main_bpb);

    if(main_bpb->boot_sig == 0x29 || main_bpb->boot_sig == 0x28) {
        data_region_start = main_bpb->reserved_sectors + 
                           (main_bpb->fat_count * main_bpb->fat_size_32);
    } else {
        print("FAT32: Error - Invalid Signature", 0x0C, 0, cursor_y++);
    }
}

void format_to_fat(char* input, char* output) {
    for(int i = 0; i < 11; i++) output[i] = ' ';

    int i = 0, j = 0;
    while(input[i] != '.' && input[i] != '\0' && j < 8) {
        char c = input[i++];
        if(c >= 'a' && c <= 'z') c -= 32;
        output[j++] = c;
    }

    while(input[i] != '\0' && input[i] != '.') i++;
    
    if(input[i] == '.') {
        i++;
        j = 8;
        while(input[i] != '\0' && j < 11) {
            char c = input[i++];
            if(c >= 'a' && c <= 'z') c -= 32;
            output[j++] = c;
        }
    }
}

uint32_t fat32_find_file_cluster(char* name) {
    fat32_dir_t* root_dir = (fat32_dir_t*) kmalloc(512);
    
    // Lê o diretório raiz
    read_sectors(data_region_start, 1, root_dir);

    for(int i = 0; i < 16; i++) {
        // Se o primeiro byte for 0, acabou a lista de arquivos
        if (root_dir[i].name[0] == 0) break;

        // Agora o compilador já conhece a função abaixo
        if (stellar_name_check(root_dir[i].name, name)) {
            return ((uint32_t)root_dir[i].cluster_high << 16) | root_dir[i].cluster_low;
        }
    }
    return 0; // Retorna 0 se não achar
}