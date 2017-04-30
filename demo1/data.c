#include <stdio.h>
#include <unistd.h>

int main(void){
    
  int val = 0;
  
  int d = 1;
  
  while(1){
    if(abs(val) > 10){
      d *= -1;
    }
    
    val += d;
    printf("%i\n", val);
    sleep(1);
  }
  
  return 0;
}