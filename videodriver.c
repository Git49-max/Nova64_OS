/* Video driver for Nova64 OS (Text mode). Originally wrote by Saulo Henrique in Thursday, January 22nd, 2026.

Last update: Thursday, January 22nd, 2026, at 16:02 GMT-3 (Horário de Brasília)

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
while(str[i] != /0){
if(x + i >= 80){
putc(str[i], color, x + i, y + i);
} else putc(str[i], color, x + i, y);
i++;
}

