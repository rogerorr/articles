#include <stdio.h>

extern void foo(int i, char ch);

void bar(double d)
{
}

void check()
{
  static char *p;
  char ch(0);
  if (p)
  {
    printf("Delta: %i\n", (p - &ch));
  }
  p = &ch;
}

int main()
{
  check();
  foo(12, 'x');
}
