/* Video driver for Nova64 OS (Text mode). Originally wrote by Saulo Henrique in Thursday, January 22nd, 2026.

Last update: Thursday, January 22nd, 2026, at 21:06 GMT-3 (Horário de Brasília)

videodriver.c

*/

#define vidAdr 0xB8000
void putc(char letter, unsigned char color, int x, int y){
volatile unsigned char *vram = (unsigned char*) vidAdr;
int offset = (y * 80 + x) * 2;
vram[offset] = letter;
vram[offset + 1] = color;
}
/* putc for a single caractere, now print for a string */
void print(char *str, unsigned char color, int x, int y){
int i = 0;
while(str[i] != '\0'){
if(x + i >= 80){
putc(str[i], color, x + i, y + i);
} else putc(str[i], color, x + i, y);
i++;
}
}
void clear_screen() {
    volatile unsigned char *vram = (unsigned char*) vidAdr;
    for (int i = 0; i < 80 * 25 * 2; i += 2) {
        vram[i] = ' ';      
        vram[i+1] = 0x07;   
    }
}

