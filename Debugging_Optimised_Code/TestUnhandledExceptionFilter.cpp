/*
NAME
  TestStackWalker

DESCRIPTION
  Test the simple stack walker for MSVC
  with an unhandled exception filter

COPYRIGHT
  Copyright (C) 2023 by Roger Orr
  <rogero@howzatt.co.uk>

  This software is distributed in the hope
  that it will be useful, but without WITHOUT
  ANY WARRANTY; without even the implied
  warranty of MERCHANTABILITY or FITNESS FOR A
  PARTICULAR PURPOSE.

  Permission is granted to anyone to make or
  distribute verbatim copies of this software
  provided that the copyright notice and this
  permission notice are preserved, and that
  the distributor grants the recipent permission
  for further distribution as permitted by this
  notice.

  Comments and suggestions are always welcome.
  Please report bugs to rogero@howzatt.co.uk.
*/

// clang-format off
static char const szRCSID[] =
    "$Id: TestStackWalker.cpp 322 2021-09-03 22:50:38Z roger $";
// clang-format on

#include "SimpleStackWalker.h"

#include <iostream>
#include <random>
#include <sstream>
#include <thread>

class Source {
  std::random_device rd;
  mutable std::mt19937 mt{rd()};

public:
  unsigned int operator()() const { return mt(); }
};

LONG WINAPI logUnhandledExit(EXCEPTION_POINTERS *exceptionInfo) {
  printf("UnhandledExceptionFilter caught "
         "%8.8x\n",
         exceptionInfo->ExceptionRecord->ExceptionCode);
  for (DWORD idx = 0; idx != exceptionInfo->ExceptionRecord->NumberParameters;
       ++idx) {
    printf("  Parameter %d: %llx\n", idx,
           exceptionInfo->ExceptionRecord->ExceptionInformation[idx]);
  }

  HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, GetCurrentThreadId());
  std::stringstream ss;
  std::thread thr{[&]() {
    SimpleStackWalker eng{GetCurrentProcess()};
    eng.stackTrace(hThread, exceptionInfo->ContextRecord, ss);
  }};
  thr.join();

  std::cout << ss.str() << '\n';

  return EXCEPTION_CONTINUE_SEARCH;
}

// #pragma optimize("", off)

void process(Source &source) {
  int local_i = printf("This ");
  int local_j = printf("is ");
  int local_k = printf("a test\n");
  int local_l = source();

  char *volatile p = (char *)(uintptr_t)0xdeadbeef;
  *p = 0;

  if (local_i != 5 || local_j != 3 || local_k != 7) {
    std::cerr << "Something odd happened\n";
  }
}

// #pragma optimize("", on)

int test() {
  Source source;
  int return_value = source();
  process(source);
  return return_value;
}

int main() {
  SetUnhandledExceptionFilter(logUnhandledExit);
  std::thread t([]() { test(); });
  t.join();
}
