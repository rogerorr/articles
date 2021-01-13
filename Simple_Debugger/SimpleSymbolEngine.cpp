/*
NAME
  SimpleSymbolEngine

DESCRIPTION
  Simple symbol engine functionality.
  This is demonstration code only - it is non. thread-safe and single instance.

COPYRIGHT
  Copyright (C) 2004, 2011 by Roger Orr <rogero@howzatt.co.uk>

  This software is distributed in the hope that it will be useful, but
  without WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

  Permission is granted to anyone to make or distribute verbatim
  copies of this software provided that the copyright notice and
  this permission notice are preserved, and that the distributor
  grants the recipent permission for further distribution as permitted
  by this notice.

  Comments and suggestions are always welcome.
  Please report bugs to rogero@howzatt.co.uk.
*/

#define _CRT_SECURE_NO_WARNINGS

#include "SimpleSymbolEngine.h"

#include <windows.h>

#include <cstddef>
#include <iostream>
#include <sstream>
#include <vector>

#include <dbghelp.h>

#pragma comment( lib, "dbghelp" )

static char const szRCSID[] = "$Id: SimpleSymbolEngine.cpp 256 2020-04-09 21:35:25Z Roger $";

namespace
{
  // Helper function to read up to maxSize bytes from address in target process
  // into the supplied buffer.
  // Returns number of bytes actually read.
  SIZE_T ReadPartialProcessMemory(HANDLE hProcess, LPCVOID address, LPVOID buffer, SIZE_T minSize, SIZE_T maxSize)
  {
    SIZE_T length = maxSize;
    while (length >= minSize)
    {
      if ( ReadProcessMemory(hProcess, address, buffer, length, 0) )
      {
        return length;
      }
      length--;

      static SYSTEM_INFO SystemInfo;
      static BOOL b = (GetSystemInfo(&SystemInfo), TRUE);

      SIZE_T pageOffset = ((ULONG_PTR)address + length) % SystemInfo.dwPageSize;
      if (pageOffset > length)
        break;
      length -= pageOffset;
    }
    return 0;
  }
}

/////////////////////////////////////////////////////////////////////////////////////
SimpleSymbolEngine::SimpleSymbolEngine()
{
  DWORD dwOpts = SymGetOptions();
  dwOpts |= SYMOPT_LOAD_LINES | SYMOPT_OMAP_FIND_NEAREST;
  SymSetOptions(dwOpts);
}

/////////////////////////////////////////////////////////////////////////////////////
void SimpleSymbolEngine::init(HANDLE process)
{
  this->hProcess = process;
  ::SymInitialize(hProcess, 0, false);
}

/////////////////////////////////////////////////////////////////////////////////////
SimpleSymbolEngine::~SimpleSymbolEngine()
{
  ::SymCleanup(hProcess);
}

/////////////////////////////////////////////////////////////////////////////////////
std::string SimpleSymbolEngine::addressToString(PVOID address)
{
  std::ostringstream oss;

  // First the raw address
  oss << "0x" << address;

  // Then symbol, if any
  struct 
  {
    SYMBOL_INFO symInfo;
    char name[ 4 * 256 ];
  } SymInfo = { {sizeof(SymInfo.symInfo)}, "" };

  PSYMBOL_INFO pSym = &SymInfo.symInfo;
  pSym->MaxNameLen = sizeof(SymInfo.name);

  DWORD64 uDisplacement(0);
  if (SymFromAddr(hProcess, reinterpret_cast<ULONG_PTR>(address), &uDisplacement, pSym))
  {
    oss << " " << pSym->Name;
    if (uDisplacement != 0)
    {
      LONG_PTR displacement = static_cast<LONG_PTR>(uDisplacement);
      if (displacement < 0)
        oss << " - " << -displacement;
      else
        oss << " + " << displacement;
    }
  }
        
  // Finally any file/line number
  IMAGEHLP_LINE64 lineInfo = {sizeof(lineInfo)};
  DWORD dwDisplacement(0);
  if (SymGetLineFromAddr64(hProcess, reinterpret_cast<ULONG_PTR>(address), &dwDisplacement, &lineInfo))
  {
    oss << "   " << lineInfo.FileName << "(" << lineInfo.LineNumber << ")";
    if (dwDisplacement != 0)
    {
      oss << " + " << dwDisplacement << " byte" << (dwDisplacement == 1 ? "" : "s");
    }
  }

  return oss.str();
}

/////////////////////////////////////////////////////////////////////////////////////
void SimpleSymbolEngine::loadModule(HANDLE hFile, PVOID baseAddress, std::string const & fileName)
{
  ::SymLoadModule64(hProcess, hFile, const_cast<char*>(fileName.c_str()), 0, reinterpret_cast<ULONG_PTR>(baseAddress), 0);
}

/////////////////////////////////////////////////////////////////////////////////////
void SimpleSymbolEngine::unloadModule(PVOID baseAddress)
{
  
  ::SymUnloadModule64(hProcess, reinterpret_cast<ULONG_PTR>(baseAddress));
}

/////////////////////////////////////////////////////////////////////////////////////
// StackTrace: try to trace the stack to the given output
void SimpleSymbolEngine::stackTrace(HANDLE hThread, std::ostream & os )
{
  CONTEXT context = {0};
  context.ContextFlags = CONTEXT_FULL;
  GetThreadContext(hThread, &context);

  STACKFRAME64 stackFrame = {0};

#ifdef _M_IX86
  stackFrame.AddrPC.Offset = context.Eip;
  stackFrame.AddrPC.Mode = AddrModeFlat;

  stackFrame.AddrFrame.Offset = context.Ebp;
  stackFrame.AddrFrame.Mode = AddrModeFlat;

  stackFrame.AddrStack.Offset = context.Esp;
  stackFrame.AddrStack.Mode = AddrModeFlat;
#elif _M_AMD64
  stackFrame.AddrPC.Offset = context.Rip;
  stackFrame.AddrPC.Mode = AddrModeFlat;

  stackFrame.AddrFrame.Offset = context.Rbp;
  stackFrame.AddrFrame.Mode = AddrModeFlat;

  stackFrame.AddrStack.Offset = context.Rsp;
  stackFrame.AddrStack.Mode = AddrModeFlat;
#elif _M_IA64
  stackFrame.AddrPC.Offset = context.StIIP;
  stackFrame.AddrPC.Mode = AddrModeFlat;

  stackFrame.AddrFrame.Offset = context.RsBSP;
  stackFrame.AddrFrame.Mode = AddrModeFlat;

  stackFrame.AddrStack.Offset = context.IntSp;
  stackFrame.AddrStack.Mode = AddrModeFlat;

  stackFrame.AddrBStore.Offset = context.RsBSP;
  stackFrame.AddrBStore.Mode = AddrModeFlat;
#else
#error Unsupported target platform
#endif // _M_IX86
  DWORD64 lastBp = 0; // Prevent loops with optimised stackframes

  os << "  Frame       Code address\n";
  while (::StackWalk64(
#ifdef _M_IX86
IMAGE_FILE_MACHINE_I386,
#elif _M_AMD64
IMAGE_FILE_MACHINE_AMD64,
#elif _M_IA64
IMAGE_FILE_MACHINE_IA64,
#endif
    hProcess, hThread,
    &stackFrame, &context,
    0, ::SymFunctionTableAccess64, ::SymGetModuleBase64, 0))
  {
    os << "  0x" << reinterpret_cast<PVOID>(stackFrame.AddrFrame.Offset) << "  " << addressToString(reinterpret_cast<PVOID>(stackFrame.AddrPC.Offset)) << "\n";
    if (lastBp >= stackFrame.AddrFrame.Offset)
    {
      os << "Stack frame out of sequence...\n";
      break;
    }
    lastBp = stackFrame.AddrFrame.Offset;
  }

  os.flush();
}

/////////////////////////////////////////////////////////////////////////////////////
// Read a string from the target
std::string SimpleSymbolEngine::getString(PVOID address, BOOL unicode, DWORD maxStringLength)
{
  if (unicode)
  {
    std::vector<wchar_t> chVector(maxStringLength + 1);
    ReadPartialProcessMemory(hProcess, address, &chVector[0], sizeof(wchar_t), maxStringLength * sizeof(wchar_t));
    size_t const wcLen = wcstombs(0, &chVector[0], 0);
    if (wcLen == (size_t)-1)
    {
       return "invalid string";
    }
    else
    {
       std::vector<char> mbStr(wcLen + 1);
       wcstombs(&mbStr[0], &chVector[0], wcLen);
       return &mbStr[0];
    }
  }
  else
  {
    std::vector<char> chVector(maxStringLength + 1);
    ReadPartialProcessMemory(hProcess, address, &chVector[0], 1, maxStringLength);
    return &chVector[0];
  }
}
