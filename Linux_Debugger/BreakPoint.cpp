#include <stdio.h>
int main()
{
  puts("Before");
  __asm( "int $0x3\n" );
  puts("After");
}

