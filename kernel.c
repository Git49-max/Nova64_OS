/*Basic kernel for Nova64 OS. Originally wrote by Saulo Henrique in Thursday, January 22nd, 2026.

Last update: Thursday, January 22nd, 2026, at 18:39 GMT-3 (Horário de Brasília)

kernel.c*/

#include videodriver.h
void main(){
  print("Nova64 is here!", 0x02, 0, 0);
  putc('>', 0x02, 0, 1);
  print("Sistem ready to further development", 0x02, 1, 1)
  while(1){
  }
}
