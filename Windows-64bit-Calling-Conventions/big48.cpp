struct S
{
  char ch1, ch2, ch3, ch4, ch5, ch6;
};

void fred(S s);

int main()
{
  S s = {0};
  fred(s);
}