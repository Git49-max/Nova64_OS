/* Video driver for Nova64 OS (Text mode). Originally wrote by Saulo Henrique in Thursday, January 22nd, 2026.

Last update: Friday, January 23rd, 2026, at 14:39 GMT-3 (Horário de Brasília)

videodriver.c

*/

#define vidAdr 0xB8000

void itoa(int n, char *str){
int i = 0;
if (n == 0){
str[i++] = '0';
str[i] = '\0';}

while(n > 0){
    str[i++] = (n % 10) + '0';
    n = n/10;
}
str[i] = '\0';
int start = 0;
int end = i - 1;
while(start < end){
char temp = str[start];
str[start] = str[end];
str[end] = temp;
start++;
end--;

void putc(char letter, unsigned char color, int x, int y, bool ascii, char *bufferstr){
volatile unsigned char *vram = (unsigned char*) vidAdr;
int offset = (y * 80 + x) * 2;
if (ascii == true){
 vram[offset] = itoa(letter, bufferstr);
 vram[offset + 1] = color;
} else {
vram[offset] = letter;
vram[offset + 1] = color;
}
}
/* putc for a single caractere, now print for a string */
void print(char *str, unsigned char color, int x, int y, bool ascii, char *bufferstr){
int i = 0;
while(str[i] != '\0'){
if(x + i >= 80 && ascii == true){
putc(str[i], color, x + i, y + i, true, bufferstr);
} else if (x + i <= 80 && ascii == true){putc(str[i], color, x + i, y, true, bufferstr) ;}
    else if(x + i >= 80 && ascii = false){putc(str[i], color, x + i, y + i, false, bufferstr);}
      else putc(str[i], color, x + i, y, false, bufferstr;
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

