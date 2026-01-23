/*Basic kernel for Nova64 OS. Originally wrote by Saulo Henrique in Thursday, January 22nd, 2026.

Last update: Friday, January 23rd, 2026, at 09:04 GMT-3 (Horário de Brasília)

kernel.c*/

#include "videodriver.h"
void main(){
  clear_screen();
  print("Nova64 is here!", 0x02, 0, 0);
  putc('>', 0x02, 0, 1);
  print("System ready to further development", 0x02, 1, 1);
  while(1){
  }
}
