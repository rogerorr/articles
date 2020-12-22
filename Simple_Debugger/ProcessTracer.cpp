/*
NAME
    ProcessTracer

DESCRIPTION
    About the simplest debugger which is useful!

COPYRIGHT
    Copyright (C) 2011 by Roger Orr <rogero@howzatt.co.uk>

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

static char const szRCSID[] = "$Id: ProcessTracer.cpp 256 2020-04-09 21:35:25Z Roger $";

#include <windows.h>

#include <iostream>
#include <map>
#include <stdexcept>

#include "SimpleSymbolEngine.h"

/** Simple process tracer */
class ProcessTracer
{
private:
  HANDLE hProcess;
  std::map<DWORD, HANDLE> threadHandles;
  SimpleSymbolEngine eng;

public:
  /** Run the debug loop */
  void run();

  /** Process has been created */
  void OnCreateProcess(DWORD processId, DWORD threadId, CREATE_PROCESS_DEBUG_INFO const & createProcess);

  /** Process has exited */
  void OnExitProcess(DWORD threadId, EXIT_PROCESS_DEBUG_INFO const & exitProcess);

  /** Thread has been created */
  void OnCreateThread(DWORD threadId, CREATE_THREAD_DEBUG_INFO const & createThread);

  /** Thread has exited */
  void OnExitThread(DWORD threadId, EXIT_THREAD_DEBUG_INFO const & exitThread);

  /** DLL has been loaded */
  void OnLoadDll(LOAD_DLL_DEBUG_INFO const & loadDll);

  /** DLL has been unloaded */
  void OnUnloadDll(UNLOAD_DLL_DEBUG_INFO const & unloadDll);

  /** OutputDebugString has been called */
  void OnOutputDebugString(OUTPUT_DEBUG_STRING_INFO const & debugString);

  /** An exception has occurred */
  void OnException(DWORD threadId, DWORD firstChance, EXCEPTION_RECORD const & exception);
};

/////////////////////////////////////////////////////////////////////////////////////////////////
void ProcessTracer::run()
{
  bool completed = false; 
  bool attached = false;

  while ( !completed )
  {
    DEBUG_EVENT DebugEvent;
    if ( !WaitForDebugEvent(&DebugEvent, INFINITE) )
    {
      throw std::runtime_error("Debug loop aborted");
    }

    DWORD continueFlag = DBG_CONTINUE;
    switch (DebugEvent.dwDebugEventCode)
    {
    case CREATE_PROCESS_DEBUG_EVENT:
      OnCreateProcess(DebugEvent.dwProcessId, DebugEvent.dwThreadId, DebugEvent.u.CreateProcessInfo);
      break;

    case EXIT_PROCESS_DEBUG_EVENT:
      OnExitProcess(DebugEvent.dwThreadId, DebugEvent.u.ExitProcess);
      completed = true;
      break;
 
    case CREATE_THREAD_DEBUG_EVENT:
      OnCreateThread(DebugEvent.dwThreadId, DebugEvent.u.CreateThread);
      break;

    case EXIT_THREAD_DEBUG_EVENT:
      OnExitThread(DebugEvent.dwThreadId, DebugEvent.u.ExitThread);
      break;
 
    case LOAD_DLL_DEBUG_EVENT:
      OnLoadDll(DebugEvent.u.LoadDll);
      break;

    case UNLOAD_DLL_DEBUG_EVENT:
      OnUnloadDll(DebugEvent.u.UnloadDll);
      break;
 
    case OUTPUT_DEBUG_STRING_EVENT:
      OnOutputDebugString(DebugEvent.u.DebugString);
      break;

    case EXCEPTION_DEBUG_EVENT:
      if (!attached)
      {
        // First exception is special
        attached = true;
      }
      else
      {
        OnException(DebugEvent.dwThreadId, DebugEvent.u.Exception.dwFirstChance, DebugEvent.u.Exception.ExceptionRecord);
        continueFlag = DBG_EXCEPTION_NOT_HANDLED;
      }
      break;

    default:
      std::cerr << "Unexpected debug event: " << DebugEvent.dwDebugEventCode << std::endl;
    }

    if ( !ContinueDebugEvent(DebugEvent.dwProcessId, DebugEvent.dwThreadId, continueFlag) )
    {
       throw std::runtime_error("Error continuing debug event");
    }
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
void ProcessTracer::OnCreateProcess(DWORD processId, DWORD threadId, CREATE_PROCESS_DEBUG_INFO const & createProcess)
{
  hProcess = createProcess.hProcess;
  threadHandles[threadId] = createProcess.hThread;
  eng.init(hProcess);

  eng.loadModule(createProcess.hFile, createProcess.lpBaseOfImage, std::string());

  std::cout << "CREATE PROCESS " << processId << " at " << eng.addressToString(createProcess.lpStartAddress) << std::endl;

  if (createProcess.hFile)
  {
    CloseHandle(createProcess.hFile);
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
void ProcessTracer::OnExitProcess(DWORD threadId, EXIT_PROCESS_DEBUG_INFO const & exitProcess)
{
  std::cout << "EXIT PROCESS " << exitProcess.dwExitCode << std::endl;

  eng.stackTrace(threadHandles[threadId], std::cout);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
void ProcessTracer::OnCreateThread(DWORD threadId, CREATE_THREAD_DEBUG_INFO const & createThread)
{
  std::cout << "CREATE THREAD " << threadId << " at " << eng.addressToString(createThread.lpStartAddress) << std::endl;

  threadHandles[threadId] = createThread.hThread;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
void ProcessTracer::OnExitThread(DWORD threadId, EXIT_THREAD_DEBUG_INFO const & exitThread)
{
  std::cout << "EXIT THREAD " << threadId << ": " << exitThread.dwExitCode << std::endl;

  eng.stackTrace(threadHandles[threadId], std::cout);

  threadHandles.erase(threadId);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
void ProcessTracer::OnLoadDll(LOAD_DLL_DEBUG_INFO const & loadDll)
{
  void *pString = 0;
  ReadProcessMemory(hProcess, loadDll.lpImageName, &pString, sizeof(pString), 0);
  std::string const fileName(eng.getString(pString, loadDll.fUnicode, MAX_PATH) );

  eng.loadModule(loadDll.hFile, loadDll.lpBaseOfDll, fileName);

  std::cout << "LOAD DLL " << loadDll.lpBaseOfDll << " " << fileName << std::endl;

  if (loadDll.hFile)
  {
    CloseHandle(loadDll.hFile);
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
void ProcessTracer::OnUnloadDll(UNLOAD_DLL_DEBUG_INFO const & unloadDll)
{
  std::cout << "UNLOAD DLL " << unloadDll.lpBaseOfDll << std::endl;

  eng.unloadModule(unloadDll.lpBaseOfDll);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
void ProcessTracer::OnOutputDebugString(OUTPUT_DEBUG_STRING_INFO const & debugString)
{
  std::string const output(eng.getString(debugString.lpDebugStringData,
         debugString.fUnicode,
         debugString.nDebugStringLength));

  std::cout << "OUTPUT DEBUG STRING: " << output << std::endl;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
void ProcessTracer::OnException(DWORD threadId, DWORD firstChance, EXCEPTION_RECORD const & exception)
{
  std::cout << "EXCEPTION 0x" << std::hex << exception.ExceptionCode << std::dec;
  std::cout << " at " << eng.addressToString(exception.ExceptionAddress);

  if (firstChance)
  {
    if (exception.NumberParameters)
    {
      std::cout << "\n  Parameters:";
      for (DWORD idx = 0; idx != exception.NumberParameters; ++idx)
      {
        std::cout << " " << exception.ExceptionInformation[idx];
      }
    }
    std::cout << std::endl;

    eng.stackTrace(threadHandles[threadId], std::cout);
  }
  else
  {
    std::cout << " (last chance)" << std::endl;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
void CreateProcess(char ** begin, char ** end)
{
  std::string cmdLine;
  for (char **it = begin; it != end; ++it)
  {
    if (!cmdLine.empty()) cmdLine += ' ';
 
    if (strchr(*it, ' '))
    {
      cmdLine += '"';
      cmdLine += *it;
      cmdLine += '"';
    }
    else
    {
      cmdLine += *it;
    }
  }

  STARTUPINFO startupInfo = { sizeof( startupInfo ) };
  startupInfo.dwFlags = STARTF_USESHOWWINDOW;
  startupInfo.wShowWindow = SW_SHOWNORMAL; // Assist GUI programs
  PROCESS_INFORMATION ProcessInformation = {0};

  if ( ! CreateProcess(0, const_cast<char*>(cmdLine.c_str()),
     0, 0, true,
     DEBUG_ONLY_THIS_PROCESS,
     0, 0, &startupInfo, &ProcessInformation) )
  {
    throw std::runtime_error(std::string("Unable to start ") + *begin);
  }

  CloseHandle( ProcessInformation.hProcess );
  CloseHandle( ProcessInformation.hThread );
}

int main( int argc, char **argv )
{
  if ( argc <= 1 )
  {
    printf( "Syntax: ProcessTracer command_line\n" );
    return 1;
  }
  ++argv;
  --argc;

  // Use the normal heap manager
  _putenv("_NO_DEBUG_HEAP=1");

  try
  {
    CreateProcess(argv, argv + argc);
    ProcessTracer().run();
  }
  catch ( std::exception &ex)
  {
    std::cerr << "Unexpected exception: " << ex.what() << std::endl;
    return 1;
  }   

  return 0;
}
