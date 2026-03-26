#include "io.h"
#include "utils/types.h"

#define ATA_PRIMARY_DATA         0x1F0
#define ATA_PRIMARY_ERR          0x1F1
#define ATA_PRIMARY_SECCOUNT     0x1F2
#define ATA_PRIMARY_LBA_LOW      0x1F3
#define ATA_PRIMARY_LBA_MID      0x1F4
#define ATA_PRIMARY_LBA_HIGH     0x1F5
#define ATA_PRIMARY_DRIVE_SEL    0x1F6
#define ATA_PRIMARY_COMM_STAT    0x1F7

void ata_wait_bsy() {
    while(inb(ATA_PRIMARY_COMM_STAT) & 0x80);
}

void ata_wait_drq() {
    while(!(inb(ATA_PRIMARY_COMM_STAT) & 0x08));
}

void read_sectors(uint32_t lba, uint8_t count, void* buffer) {
    ata_wait_bsy();
    
    // 0xE0 = Master Drive + LBA mode
    outb(ATA_PRIMARY_DRIVE_SEL, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_PRIMARY_SECCOUNT, count);
    outb(ATA_PRIMARY_LBA_LOW, (uint8_t)lba);
    outb(ATA_PRIMARY_LBA_MID, (uint8_t)(lba >> 8));
    outb(ATA_PRIMARY_LBA_HIGH, (uint8_t)(lba >> 16));
    outb(ATA_PRIMARY_COMM_STAT, 0x20); // Comando de leitura

    uint16_t* ptr = (uint16_t*)buffer;
    for (int i = 0; i < count; i++) {
        ata_wait_bsy();
        ata_wait_drq();
        for (int j = 0; j < 256; j++) {
            *ptr++ = inw(ATA_PRIMARY_DATA);
        }
    }
}