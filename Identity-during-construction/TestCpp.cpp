#include <iostream>
#include <typeinfo>

int value()
{
static int seed;
  std::cout << "value()\n";
  return ++seed;
}

class Base
{
private:
  int i = value();
public:
  Base()
  {
    std::cout << "Base ctor\n";
    std::cout << "Type=" << typeid(*this).name() << "\n";
    std::cout << "Base::i=" << i << "\n";
    init();
  }

  virtual void init()
  {
    std::cout << "Base::init\n";
    std::cout << "Base::i=" << i << "\n";
  }
};

class Derived: public Base
{
private:
  int j = value();
public:
  Derived()
  {
    std::cout << "Derived ctor\n";
    std::cout << "Type=" << typeid(*this).name() << "\n";
    std::cout << "Derived::j=" << j << "\n";
  }

  void init()
  {
    std::cout << "Derived::init\n";
    std::cout << "Base::j=" << j << "\n";
  }
};


int main()
{
  Derived d;
}
