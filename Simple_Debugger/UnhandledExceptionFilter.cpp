#include <windows.h>

#include <stdio.h>

LONG WINAPI cleanExit(EXCEPTION_POINTERS *exceptionInfo)
{
  printf("Exception 0x%8.8x\n", exceptionInfo->ExceptionRecord->ExceptionCode);
  exit(0);
}

int main()
{
  SetUnhandledExceptionFilter(cleanExit);
  char *p = 0;
  *p = 0;
}

