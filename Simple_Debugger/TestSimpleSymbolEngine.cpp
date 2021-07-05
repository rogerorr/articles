#include "SimpleSymbolEngine.h"

#include <iostream>
#include <thread>

void bottom()
{
  HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, GetCurrentThreadId());
  std::thread thr([=]() {
    SimpleSymbolEngine eng;
    eng.init(GetCurrentProcess());
    eng.stackTrace(hThread, std::cout);
  });
  thr.join();
}

void middle()
{
  bottom();
}

void top()
{
  middle();
}

int main()
{
  top();
}
