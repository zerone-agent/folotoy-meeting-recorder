#include "meeting_scr1.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
int main(void) {
 uint8_t b[104]={0}; memcpy(b,"SCR1",4);b[4]=1;memset(b+20,255,4);memset(b+24,0xa5,48);b[96]=3;b[97]=0xf8;b[98]=0xff;b[99]=0xfe;b[100]=3;b[101]=0xf8;b[102]=0xff;b[103]=0xfe;
 for(size_t i=0;i<sizeof(b);i+=7){size_t n=sizeof(b)-i;if(n>7)n=7;if(!meeting_scr1_project(i,b+i,n,sizeof(b),2))return 1;}
 return fwrite(b,1,sizeof(b),stdout)==sizeof(b)?0:1;
}
