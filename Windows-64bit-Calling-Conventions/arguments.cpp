#include <stdio.h>

typedef void testfn(int a, int b, int c, int d, int e);

void test(int a, int b, int c, int d, int e)
{
  printf("sum: %i\n", a + b + c + d + e);
}

int main()
{
  testfn * volatile testp = &test;
  testp(1111, 2222, 3333, 4444, 5555);
}