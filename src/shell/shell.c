#include "keyboard/kbdriver.h"
#include "VGA/videodriver.h"
#include "utils/string.h"
#include "RTC/rtcdriver.h"
#include "kernel/kernel.h"
#include "utils/io.h"
#include "utils/pmm.h" // Para acessar free_pages 
#include "utils/malloc.h"
#include "fs/fat32.h"
#include "stellar/stellar.h"

extern int cursor_x;
extern int cursor_y;
extern int shell_active;
extern int config_status;
extern uint64_t free_pages;

extern uint64_t stellar_stack[8192];
extern uint32_t data_region_start;
extern fat32_bpb_t* main_bpb;

void run_stellar_from_disk() {
    uint8_t* code_buf = (uint8_t*) kmalloc(512);
    
    // FAT32 armazena em maiúsculas: "CALCULO EXE" (7 letras + 1 espaço + 3 ext)
    uint32_t cluster = fat32_find_file_cluster("CALCULO EXE");
    
    // SE O CLUSTER FOR 0, O ARQUIVO NÃO FOI ACHADO.
    // Se tentar ler o cluster 0, o driver ATA vai travar o Nova64!
    if (cluster < 2) {
        print("Erro: CALCULO.EXE nao encontrado no disco!", 0x0C, 0, cursor_y++);
        return; 
    }

    // Cálculo do setor físico (LBA)
    uint32_t lba = data_region_start + ((cluster - 2) * main_bpb->sectors_per_cluster);
    
    read_sectors(lba, 1, code_buf);

    StellarVM vm;
    stellar_init(&vm, code_buf, (double*)stellar_stack);
    stellar_run(&vm);
}
int h, m, s;

void display_neofetch() {
    cursor_y++;
    print("  _   _                  ", 0x0B, 0, cursor_y++);
    print(" | \\ | |               ", 0x0B, 0, cursor_y++);
    print(" |  \\| | _____   ____  ", 0x0B, 0, cursor_y++);
    print(" | . ` |/ _ \\ \\ / / _`| ", 0x0B, 0, cursor_y++);
    print(" | |\\  | (_) \\ V / (_|| ", 0x0B, 0, cursor_y++);
    print(" |_| \\_|\\___/ \\_/ \\__, ", 0x0B, 0, cursor_y++);
    
    cursor_y++;
    
    print(" OS: ", 0x0B, 0, cursor_y); 
    print("Nova64 OS Aurora v0.1.0S", 0x07, 5, cursor_y++); 
    
    print(" KERNEL: ", 0x0B, 0, cursor_y);
    print("x86_64 Monolithic", 0x07, 9, cursor_y++);
    
    print(" SHELL: ", 0x0B, 0, cursor_y);
    print("NovaBash (shell32)", 0x07, 8, cursor_y++);
    
    // Memória Real baseada no PMM
    print(" MEMORY: ", 0x0B, 0, cursor_y);
    uint64_t free_kb = free_pages * 4;
    print_int(128, 0x07, 9, cursor_y);
    print(" MB / Free: ", 0x07, 13, cursor_y);
    print_int(free_kb / 1024, 0x07, 25, cursor_y);
    print(" MB", 0x07, 28, cursor_y++);
}

void print_formatted_time(int hh, int mm, int ss, int x, int y) {
    if(hh < 10) { putc('0', 0xF0, x++, y); }
    print_int(hh, 0xF0, x, y);
    x += (hh < 10) ? 1 : 2;
    putc(':', 0xF0, x++, y);
    if(mm < 10) { putc('0', 0xF0, x++, y); }
    print_int(mm, 0xF0, x, y);
    x += (mm < 10) ? 1 : 2;
    putc(':', 0xF0, x++, y);
    if(ss < 10) { putc('0', 0xF0, x++, y); }
    print_int(ss, 0xF0, x, y);
}

void bashinit(){
    clear_screen();
    print("Nova64 Bash", 0x0B, 0, 0);
    cursor_y = 1;
    putc('>', 0x0F, 0, cursor_y);
    putc('$', 0x0F, 0, cursor_y);
    print(user.data, 0x0F, 2, cursor_y);
    putc('>', 0x0F, user.length + 2, cursor_y); 
    cursor_x = user.length + 3;
}

void exeCommand(String key_buffer){
    cursor_y++; 
    cursor_x = 0;

    if(strcmp(key_buffer.data, "time") == 0){
        get_time(&h, &m, &s);
        print(time_zone.data, 0x02, 0, cursor_y);
        print_formatted_time(h, m, s, 14, cursor_y);
    } 
    else if(strcmp(key_buffer.data, "cls") == 0){
        clear_screen();
        bashinit();
    }
    else if(strcmp(key_buffer.data, "free") == 0){ // NOVO COMANDO 
        print("Memory Free: ", 0x0A, 0, cursor_y);
        print_int((free_pages * 4) / 1024, 0x07, 13, cursor_y);
        print(" MB", 0x07, 16, cursor_y);
    }
    else if(strcmp(key_buffer.data, "exit") == 0){
        shell_active = 0;
        clear_screen();
        print("Nova64 OS", 0x0B, 0, 0);
        if(config_status != 3 ){
            print("> [1] Start Configuration", 0x0F, 0, 1);
        }
        putc('>', 0x02, 0, 2);
        cursor_x = 2;
        cursor_y = 2;
    } else if(strcmp(key_buffer.data, "neofetch") == 0){
        display_neofetch();
    } else if(strcmp(key_buffer.data, "power --restart") == 0){
        reboot();
    } else if(strcmp(key_buffer.data, "power --off") == 0){
        shutdown();
    } else if(strcmp(key_buffer.data, "stellar") == 0){
        run_stellar_from_disk();
    }

    else {
        print("COMMAND NOT FOUND!", 0x0C, 0, cursor_y);
    }
}