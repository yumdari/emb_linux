#include <autoconf.h>
#include <stdio.h>
#include <unistd.h>
#include <asm/unistd.h>
#include <string.h>
//#include <stdbool.h>

int main(int argc, char* argv[]){
   unsigned long l;
   //printf("input value = ");
   //scanf("%ld",&l);
   
   if(argc == 2){
      if(argv[1][3]>='0'&& argv[1][3]<='9'){
         l= argv[1][3]-'0';

      }
      else if(argv[1][3] >= 'a'&& argv[1][3] <='z'){
         l=argv[1][3] -'a'+10;
      }
	  l = syscall(__NR_mysyscall, l);
   }
   else{

      int dir = 1; // 1 : + ,  0 : -
   
      l = 0x01;
   
      while(1){
         int idx;
         l = syscall(__NR_mysyscall,l);
         for(idx =0 ; idx < 4; idx ++){   
            if(((l >> idx) & 0x01) == 0x01){
               putchar('O');
            }
            else
               putchar('X');
   
            if(idx == 3) break;
            putchar(':');
         }
         usleep(100000);   
         putchar('\n');


         if(l == 0x00){
            if(dir == 1){
               l = 0x08;
               dir = 0;
            }
            else{
               l = 0x01;
               dir = 1;
            }
         }
         else{   
            if(dir == 1)
               l=l<<1;
         
            else 
               l=l>>1;
         }
      }
   }
   l = syscall(__NR_mysyscall,l);
   if(l <0){
      perror("syscall");
      return 1;
   }
   printf("mysyscall return value = %ld\n",l);
   return 0;
}
