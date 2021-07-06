// cl /LD /EHsc /Zi function.cpp

void __declspec(dllexport) func()
{
  throw 27;
}

