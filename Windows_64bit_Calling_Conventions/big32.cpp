struct S
{
  char ch1, ch2, ch3, ch4;
};

void fred(S s);

int main()
{
  S s = {0};
  fred(s);
}