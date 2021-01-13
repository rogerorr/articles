/*
NAME
    HeapCorruption
*/

#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv)
{
  int len = argc > 1 ? atoi(argv[1]) : 0;
  
  char *buffer1 = (char*)calloc(100, 1);
  char *buffer2 = (char*)calloc(100, 1);
  
  puts("Fill buffer 1");
  memset(buffer1, '@', len);
  
  puts("Print buffer 2");
  puts(buffer2);

  puts("Free buffer 2");
  free(buffer2);

  puts("Free buffer 1");
  free(buffer1);

  puts("So long, and thanks for all the fish");
}
