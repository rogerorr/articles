struct data
{
data();
data(int c);
int a;
int b;
int c;
int d;
data test(data d);
};

data foo(int a, int b)
{
  return a + b;
}

void bill()
{
  data dick(foo(100, 200));
}

data sophie(data d)
{
  return d;
}

data chris()
{
  return sophie(data(63));
}

void test()
{
  data d;
  data e;
  data f(d.test(e));
}
