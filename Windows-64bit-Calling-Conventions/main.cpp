// cl /EHsc /Zi main.cpp

#include <stdio.h>

extern void __declspec(dllimport) func();

#pragma comment(lib, "function")

int main()
{
  try
  {
    func();
  }
  catch (...)
  {
    puts("Exception");
    return 1;
  }
  return 0;
}
