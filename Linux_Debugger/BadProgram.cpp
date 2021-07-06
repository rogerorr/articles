// Trivial program for demonstrating ProcessTracer.exe
#include <stdio.h>

int main(int argc, char **argv)
{
  int idx;
  for (idx = 1; idx < argc; ++idx)
  {
    printf("** opening %s\n", argv[idx]);
    FILE *fp = fopen(argv[idx], "r");
    fclose(fp);
  }
}
