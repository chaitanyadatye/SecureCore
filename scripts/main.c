

#include<stdio.h>                                                                                
int main()
{
  float f, f1, f2;
  int i, temp;
  f1 = 2.2;
  f2 = 3;
  temp = 3;
  for (i = 0; i < 7;i++) {
     if (temp) {
        f1 = calc_command_cx(f1, f2);
     }
     else  {
        f2 = calc_command_cx(f1,f2);
     }
     temp -= 2;
  }
  f = calc_command_cx(f1, f2);
 printf("%f", f);
                                                                                
}

