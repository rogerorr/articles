extern void check();
extern void bar(double d);

void foo(int i, char ch)
{
  double k;
  k = i + ch;
  bar(k);
  check();
}

