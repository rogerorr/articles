#include "SimpleSymbolEngine.h"

#include <iostream>
#include <sstream>
#include <thread>

int bottom()
{
  HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, GetCurrentThreadId());
  std::stringstream ss;
  std::thread thr([&]() {
    SimpleSymbolEngine eng;
    eng.init(GetCurrentProcess());
    eng.stackTrace(hThread, ss);
  });
  thr.join();
  
  int errors{};
  std::string out(ss.str());
  std::cout << out << '\n';
  for (auto* name : { "bottom", "middle", "top", "main"}) {
    if (out.find(name) == std::string::npos) {
      std::cerr << "Error: unable to find '" << name << "' in stack trace\n";
      ++errors;
    }
  }

  return errors;
}

int middle()
{
  return bottom();
}

int top()
{
  return middle();
}

int main()
{
  return top();
}
