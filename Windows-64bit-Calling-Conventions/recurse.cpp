#include <stdio.h>

void foo(int i, char ch)
{
  double k;
  k = i + ch;
  printf("&k: %p\n", &k);;
  if (ch > 0) foo(i, ch-1);
}

int main()
{
  foo(120, '\4');
}