// Trivial program for demonstrating ProcessTracer.exe

class Test
{
private:
  int *p;
public:
  Test() : p(0) {}

  int doit() { return *p; }
};


int main()
{
  int ret = Test().doit();
  return ret;
}