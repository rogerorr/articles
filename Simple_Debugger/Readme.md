# Using the Windows Debugging API #

## Introduction
A significant amount of most programmers' time is spent in debugging. Wikipedia defines this activity as: "a methodical process of finding and reducing the number of bugs, or defects, in a computer program or a piece of electronic hardware, thus making it behave as expected."

There are many different (overlapping) _ways_ to debug; but one of the commonest is to use an interactive debugger. There are a large number of different kinds of interactive debuggers, such as hardware probes for testing new hardware or emulators for embedded software components, but most programmers probably think of debugging an application running on a desktop operating system. However even on a desktop operating system you may have:

- operating system level debuggers (so called 'kernel debuggers') which usually require a secondary machine to host the debugger and provide access to the entirety of the machine, device drivers, operating system facilities and application programs.

- general application debuggers, such as Visual Studio in 'Native' debugging mode on Windows or gdb on Linux

- virtual machine debuggers, such as Visual Studio in 'Managed' debugging mode for .NET programs or the Eclipse Java debugger for JVM programs.

What does a debugger do?  Well, a compiler compiles programs so you might naively expect that a debugger debugs them. Sadly this is not usually the case; an interactive debugger is a tool used by programmers to find errors in their program.

I personally dislike the term 'debugger': in my experience the best tools for *automatically* debugging are static code analysis tools; but it's probably too late to change the name now.
So what does a debugger actually provide?  In general, most interactive debuggers provide the ability to:
- stop a program when errors occur
- inspect the state of the program
- set breakpoints
- step through the program
- provide symbolic names and source code for objects in the program
- modify the state of the program's memory

Given this list, perhaps a better description of what they do is "interactive tracing and visualisation".
Many programmers are familiar with what they do, but few people seem to know how they work.

This article focuses on the Windows application debugging API that is used by the general application debuggers in the list above. (Kernel debuggers and virtual machine debuggers use very different mechanisms to perform their task.) There is a huge amount of work in a good interactive debugger and it would take several articles to describe all the features that need implementing. I am restricting this article to describing the basic debugger API, and will work through a simple example of how to use this API to trace the key events of a program's execution.

(I hope to produce a subsequent article doing something similar with the Unix debug API which will provide a contrasting mechanism for achieving a similar end on a different operating system.)

## What's in the Win32 debugging API?

The two main methods in the Win32 debug interface are `WaitForDebugEvent` and `ContinueDebugEvent`. These provide the mechanism for the debugger to be notified of debug events from the process being debugged and to resume the target process once the event has been processed.

These methods are activated when a child process is created with one of the special flags: `DEBUG_ONLY_THIS_PROCESS` or `DEBUG_PROCESS`.  This sets up a  communication channel between the child process (known as the 'debuggee' or 'target process') and the parent ('debugger') process.

The debugger process then receives notification events from the debuggee on:
- process start and exit
- thread start and exit
- DLL load and unload
- OutputDebugString calls
- each occurrence of an exception

In each case the event provides some additional data with the event to give the debugger (just) enough information to be able to make sense of it. For example when a DLL is unloaded the event contains the base address of the unloaded DLL.

Note that several different things are included in the category of "exception" -- such as access violations, software generated errors and break points (used in a full-scale debugger to add support for stepping through a program).

## What is missing?

The Windows debug API is solely concerned with the debug events and does not of itself provide any other access to the target process. (Note that this is very different from the ptrace interface used in most of the Unix world.)

All other access to the target process is achieved using general purpose functions that are not restricted to a debugger; given suitable permissions *any* process can use these functions. So for example the debugger can use the functions `ReadProcessMemory` and `WriteProcessMemory` to read or write memory in the target process and `GetThreadContex`t to read the process registers for a thread.

This has the advantage that much of the functionality of a debugger does not require the specific debugger-debuggee relationship between the processes and so a variety of tools can be written to provide specific functionality, such as visualising a program's data structures or giving a stack dump of all the active threads. The Microsoft debugger WinDbg (included in "Debugging Tools for Windows" freely available from  "http://www.microsoft.com/whdc/DevTools/Debugging/default.mspx") has a "non-invasive" attach mode that demonstrates just how much debugging you can do _without _the debug API.

## Interpreting symbolic information

The main method used by Microsoft for attaching symbolic information to executable files is the PDB ("program database)" format. I wrote an earlier article (for [Overload 67](http://accu.org/index.php/journals/276) giving an introduction to the Microsoft symbol engine and I've used the code from that article (slightly expanded) to help provide symbolic information in this example program. I'm not going to go into further details of the symbol engine implementation, apart from a note about the getString method (under "DLL load and unload" below).

In the example code below the field `eng` refers to this symbol engine and the methods from it used below include `loadModule` (loads the debug information for a module), `addressToString` (converts a target address into a symbolic string) and `stackTrace` (prints the call stack).

## ProcessTracer

I am going to use a program that traces major events in a program's lifecycle to provide the framework for exploring the Windows Debug API. Here is an example of the program in use (paths edited for clarity):

```
C:> ProcessTracer BadProgram.exe
CREATE PROCESS 4092 at 0x00401398 mainCRTStartup   
     f:\\dd\\...\\crtexe.c(404)
LOAD DLL 77230000 ntdll.dll
LOAD DLL 75BC0000 C:\\Windows\\system32\\kernel32.dll
LOAD DLL 75430000 C:\\Windows\\system32\\KERNELBASE.dll
LOAD DLL 6BD90000 C:\\Windows\\WinSxS\\...\\MSVCR80.dll
LOAD DLL 76E40000 C:\\Windows\\system32\\msvcrt.dll
EXCEPTION 0xc0000005 at 0x0040108C Test::doit + 12 c:\\article\\badprogram.cpp(10) + 12 bytes
  Parameters: 0 0
  Frame       Code address
  0x0012FF34  0x0040108C Test::doit + 12 c:\\article\\badprogram.cpp(10) + 12 bytes
  0x0012FF44  0x00401045 main + 21 c:\\article\\badprogram.cpp(16) + 15 bytes
  0x0012FF88  0x004011F7 __tmainCRTStartup + 271 f:\\dd\\...\\crtexe.c(597) + 23 bytes
  0x0012FF94  0x75C11194 BaseThreadInitThunk + 18
  0x0012FFD4  0x7728B3F5 RtlInitializeExceptionChain + 99
  0x0012FFEC  0x7728B3C8 RtlInitializeExceptionChain + 54
EXCEPTION 0xc0000005 at 0x0040108C Test::doit + 12 c:\\article\\badprogram.cpp(10) + 12 bytes (last chance)
 EXIT PROCESS 3221225477
  Frame       Code address
  0x0012FF34  0x0040108C Test::doit + 12 c:\\article\\badprogram.cpp(10) + 12 bytes
  0x0012FF44  0x00401045 main + 21 c:\\article\\badprogram.cpp(16) + 15 bytes
  0x0012FF88  0x004011F7 __tmainCRTStartup + 271 f:\\dd\\...\\crtexe.c(597) + 23 bytes
  0x0012FF94  0x75C11194 BaseThreadInitThunk + 18
  0x0012FFD4  0x7728B3F5 RtlInitializeExceptionChain + 99
  0x0012FFEC  0x7728B3C8 RtlInitializeExceptionChain + 54
```
## Getting started

The first step is to create the child process (BadProgram.exe) with the correct flags. This is the relevant call to `CreateProcess`:
```c++
CreateProcess(0, const_cast<char*>(cmdLine.c_str()),
     0, 0, true,
     DEBUG_ONLY_THIS_PROCESS,
     0, 0, &startupInfo, &ProcessInformation)\f0
```
We don't need the process and thread handles returned by the `CreateProcess` call as the debug API will provide them, so we close these handles immediately.
One of the things I find confusing about the debug API is remembering which handles need to be closed manually and which ones are handled by the system \endash  we will revisit this issue later on.

The debug API is designed to support debugging multiple processes; to achieve this you should pass the `DEBUG_PROCESS` flag to CreateProcess. I've not done that in this example program as implementing a debugger that correctly manages multiple child processes would make the code significantly more complex without really adding much new material.

## The heart of the matter

The main driving loop of ProcessTracer is the "debug loop", which looks like this:

```c++
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
      ... // cases elided, for now

    default:
      std::cerr << "Unexpected debug event: " << 
        DebugEvent.dwDebugEventCode << std::endl;
    }

    if ( !ContinueDebugEvent(DebugEvent.dwProcessId,
        DebugEvent.dwThreadId, continueFlag) )
    {
       throw std::runtime_error("Error continuing debug event");
    }
  }
```
The first call halts the debugger until the next event is ready from the debuggee and, on a successful return, the appropriate fields of the `DebugEvent` structure will be populated. When one of the various debug events occurs in the target, the operating system blocks *all* the threads in the process and passes the appropriate debugging event and associated data to the debugger. Execution of the target will not resume until the debugger signals that it has completed its handling of the event by calling `ContinueDebugEvent`.

`ContinueDebugEvent` needs the process and thread ID: in this example the process ID will always be the same (but the thread ID may vary if the target process creates additional threads.)  The function also takes a '`continueFlag`' argument.  This is only relevant when the event is an exception and I'll cover the use of this argument when I look at handling exception events.

This synchronous call-based mechanism of passing events between the debuggee and the debugger makes it slightly tricky to write an interactive debugger since the debugger has to be responsive to user actions via the GUI and also wait for debug events from the target process. This normally means the debug loop runs in its own dedicated thread, decoupling it from the user interface.  However in the ProcessTracer example there is no UI and so the implementation can be a simple single threaded application.

## Process start and stop

The first event you receive is a CREATE_PROCESS_DEBUG_EVENT. The code in the debug loop in ProcessTracer is:

```c++
switch (DebugEvent.dwDebugEventCode)
{
  case CREATE_PROCESS_DEBUG_EVENT:
    OnCreateProcess(DebugEvent.dwProcessId, DebugEvent.dwThreadId,
      DebugEvent.u.CreateProcessInfo);
    break;
  ...
```
The implementation of `OnCreateProcess` is:

```c++
void ProcessTracer::OnCreateProcess(
  DWORD processId, DWORD threadId,
  CREATE_PROCESS_DEBUG_INFO const & createProcess)
{
  hProcess = createProcess.hProcess;
  threadHandles[threadId] = createProcess.hThread;
  eng.init(hProcess); // Initialise the symbol engine

  eng.loadModule(createProcess.hFile, 
   createProcess.lpBaseOfImage, std::string());

  std::cout << "CREATE PROCESS " << processId << " at " <<
   eng.addressToString(createProcess.lpStartAddress) <<
   std::endl;

  if (createProcess.hFile)
  {
    CloseHandle(createProcess.hFile);
  }
}
```
The `CreateProcessInfo` debug event data includes a handle to the process and to the main thread. Subsequent events will not provide these handles so it is important to retain them while the process is active: I keep the process handle in a simple field and the thread handle in a map indexed by thread ID. (To my mind this is a poor API design since it forces *each* user of the API to implement a mechanism to manage mapping process and thread IDs to handles.)

Also included in the create process event data is a handle to the file containing the executable program and the base address of the image. This can be passed to the symbol engine to populate the data for the main executable. Sadly, although the event data contains a field `lpImageName` that is documented as "may contain the address of a string pointer in the address space of the process being debugged" the string is, as far as I can tell, *always* absent. So we have a handle to the file but do not know its name -- I could write another article on the various mechanisms to get the file name from a file handle but for simplicity I've simply passed an empty string as the module name. (This is another place where the debug API appears to have been poorly implemented.)

Finally note that the debug API manages the *process* and *thread* handles and we must *not* attempt to close them, but that we *are* responsible for closing the *file* handle (if it was provided) - failing to do this can result in a long running debugger leaking file handles and keeping files locked. (As I said earlier, this confusion over the ownership of open handles complicates the job of writing a debugger; I can see no good reason for this asymmetric design in the API.)

In this simple case the file handle will be provided as we create the child process using the same credentials as the parent process; in more complex uses of the debug API involving processes with different credentials you may find that the debugger process has no permission to access the file and then no file handle is provided.

When the process ends the EXIT_PROCESS_DEBUG_EVENT is generated as the last debug event; the process handle is then closed by the debug API. In ProcessTracer we log the event, print a stack trace using the symbol engine and then set `completed` to `true` to terminate the debug loop.

## Thread start and stop

The process start event implicitly includes a thread start event of the main application thread (and the process exit event implicitly includes a thread exit event for the last thread closed). On the creation of additional threads a separate event is raised, containing the start address and thread handle for the newly created thread.

Our code simply logs the event and adds the thread handle to the map. Here is the method called from the debug loop:

```c++
void ProcessTracer::OnCreateThread(DWORD threadId, CREATE_THREAD_DEBUG_INFO const & createThread)
{
  std::cout << "CREATE THREAD " << threadId << " at " << 
    eng.addressToString(createThread.lpStartAddress) <<
    std::endl;

  threadHandles[threadId] = createThread.hThread;
}
```

When a thread exits the associated data includes the exit code for the thread; in process tracer we simply log this and print a stack trace using the symbol engine.
We also remove the thread handle from the map since the debug API closes the thread handle for us on the next call to `ContinueDebugEvent`.

## DLL load and unload

As a DLL is loaded into the target process a debug event is generated containing, among other things, a file handle and a pointer to the file name of the DLL (in the target address space).
The file name is usually a fully qualified path, but the *first *DLL loaded (which is always `ntdll`) has a path-less file name simply consisting of 'ntdll.dll'. If you need the full file name you can use the fact that *all* Win32 processes load ntdll.dll from the same location and so obtain the full file name by using `GetModuleFileName` on the _debugger_ process.

Here is the code in ProcessTracer that handles the DLL load event:

```c++
void ProcessTracer::OnLoadDll(LOAD_DLL_DEBUG_INFO const & loadDll)
{
  void *pString = 0;
  ReadProcessMemory(hProcess, loadDll.lpImageName,
    &pString, sizeof(pString), 0);
  std::string const fileName(eng.getString(
    pString, loadDll.fUnicode, MAX_PATH) );

  eng.loadModule(loadDll.hFile, loadDll.lpBaseOfDll, fileName);

  std::cout << "LOAD DLL " << loadDll.lpBaseOfDll << " " <<
    fileName << std::endl;

  if (loadDll.hFile)
  {
    CloseHandle(loadDll.hFile);
  }
}
```
Note that closing the file handle is, once again, our responsibility.

Similarly the unload DLL event is handled by logging the event and calling 
`eng.unloadModule(unloadDll.lpBaseOfDll)`.

## How long is a NUL terminated string?

One slight twist I will mention in getString is that, since the string is NUL terminated, we don't know how long the string is until we have read it. The naive implementation of just trying to read MAX_PATH characters sometimes fails since the string being read is near a page boundary. It is a shame that the debug API doesn't report the string length too.

The error code reported by calling `GetLastError` on a failed call to `ReadProcessMemory` is `ERROR_PARTIAL_COPY` but in fact no partial copying has been done - the `ReadProcessMemory` function fails if we try to read data from the target process where only some of the pages of memory are accessible. Do not be misled by the last argument to this function:  `SIZE_T *lpNumberOfBytesRead`. This value can *only *be either 0 on failure or the full buffer size on success!

My solution is to first try and read the entire MAX_PATH buffer from the target, as this is usually successful. Should this fail I reduce the number of bytes read to the next lowest page boundary.

## OutputDebugString events

Windows provides the `OutputDebugString` function specifically to send a string to the debugger for display.  A debug event is generated when the target process calls this function and the associated debug information provides the buffer address -- and length -- so reading the data from the target and displaying it is very easy.

## Exceptions

These are probably the most interesting debug events and there is a lot of processing that can be done here.  The debug API allows the debugger a *first* chance to look at the exception and, if neither the debugger nor the debuggee handles the exception, a *last* chance to look at the unhandled exception before terminating the process.

The detailed flow of control when an exception occurs is as follows.

1) An exception occurs in the target process
2) The debugger gets a debug event with the `dwFirstChance` flag set.
3) The debugger calls `ContinueDebugEvent` with `dwContinueStatus` set to (a) DBG_EXCEPTION_NOT_HANDLED or (b) DBG_CONTINUE
The following stages will then be either

4a - DBG_EXCEPTION_NOT_HANDLED) The debuggee follows the usual search and dispatch logic of normal exception flow
5) If no handler is found the debugger gets a second debug event - but this time with the dwFirstChance flag set to zero
6) When the debugger calls `ContinueDebugEvent` the target process is terminated.

or

4b - DBG_CONTINUE) The target process resumes execution back at the exception context as if the exception had not occurred. The context may be the _next _instruction (for example after a breakpoint exception) or the _same _instruction (for example after an access violation); in the latter case if the underlying cause of the exception hasn't changed the exception will occur again and you end up back at (1).

Microsoft Visual Studio, for example, uses the first chance exception to pop up a dialog box with the options Break, Continue or Ignore.  'Break' leaves the debugger active and defers the `ContinueDebugEvent` call until later; somewhat confusingly 'Continue' corresponds to option 4a (DBG_EXCEPTION_NOT_HANDLED) and 'Ignore' corresponds to option 4b (DBG_CONTINUE).

Just to make things interesting, when a process is being debugged, the program loader generates a breakpoint exception event when the process start up has completed (this exception should always be continued). I think this is a false economy in the API and it would have been better to designate a specific event for dealing with this case. In particular reuse of the breakpoint event makes it hard to process any exceptions that occur while loading a DLL during process start-up. In ProcessTrace we simply test and set a boolean variable `attached` to detect the first exception event. In all other cases we set the `continueFlag` to `DBG_EXCEPTION_NOT_HANDLED` to trace into the handling of the exception by the target process.

Our handling of an exception looks like this:

```c++
void ProcessTracer::OnException(DWORD threadId,
  DWORD firstChance, EXCEPTION_RECORD const & exception)
{
  std::cout << "EXCEPTION 0x" << std::hex <<
    exception.ExceptionCode << std::dec << " at " <<
    eng.addressToString(exception.ExceptionAddress);

  if (firstChance)
  {
    if (exception.NumberParameters)
    {
      std::cout << "\\n  Parameters:";
      for (DWORD idx = 0; idx != exception.NumberParameters;
        ++idx)
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
```
The event data contains the code for the exception and the address where the exception occurred. Some exceptions also include additional information describing the exception; for example `EXCEPTION_ACCESS_VIOLATION` indicates the access mode that failed (read, write, execute) in the first entry of the exception information and the address of the inaccessible data in the second entry.

## Changes in the debugged process

When a process is running under the debug API several things are different. It is important to be aware of these as behaviour that is different under the debugger is hard to debug!

Firstly, by default, the Windows Heap manager runs in a 'debug' mode when the process is being debugged. This adds additional checking to memory allocations and writes check data before and after each allocation to allow detection of under- and over-runs. In current versions of Windows this default behaviour can be turned off by defining the environment variable _NO_DEBUG_HEAP.
ProcessTracer does this automatically using:

  `_putenv("_NO_DEBUG_HEAP=1");`

Secondly, the CloseHandle function throws an exception with exception code `STATUS_INVALID_HANDLE` (0xC0000008) when an invalid handle value is closed. The theory is that this ensures bad handle values are made visible when you are debugging the program; but this can mean the behaviour of an application changes under a debugger since exception unwinding can be invoked. One option is to set the continueFlag to DBG_CONTINUE for this exception code.

Thirdly, if `SetUnhandledExceptionFilter` is used to set the unhandled exception filter for a process this will be *ignored* if the process is being debugged. This is not usually a problem, but does make debugging an unhandled exception filter troublesome. 

Fourthly, text sent to `OutputDebugString` goes to the application debugger rather than to the system debugger (or a tool like SysInternals Dbgview).
Finally the action of the debugger changes the runtime behaviour of the process as each debug event involves stopping all the threads in the process and several context switches back and forth to the debugger thread. This can make some sorts of race condition hard to debug since the action of debugging the process changes the timings of the interactions between the affected threads.

There are two API calls that can be used to check if a process is being debugged: CheckRemoteDebuggerPresent (which checks the presence of a debug connection from another process) and IsBeingDebugged (which reads the value of the `BeingDebugged` flag located at byte offset 2 in the Process Environment Block). 

## Attaching to existing processes

The debug API can also be used to attach to an already running process by using `DebugActiveProcess`.
Most of the debug events described above are exactly the same in this case; the biggest change is that when the process start and DLL load events are generated the file handle and file name are usually both zero. However the `GetModuleFileNameEx` function in the Windows 'Process Status' library (PSAPI) can be used to get the file name in this case (unfortunately this method *only* works when attaching to an existing process).

Additionally in order to debug a process with different credentials you may need to have the `SeDebugPrivilege` privilege and appropriate permissions - the details are outside the scope of this particular article.

Finally a debugger can detach from a debuggee by using `DebugActiveProcessStop`.

## Writing a fully fledged interactive debugger

I haven't covered more than the basics of a debugger and there is obviously a lot more that must be added to write a proper interactive debugger. However I hope that the overview of the debug API that I have presented here has given you some understanding of the bare bones of the interaction between the debugger and the target.

## Acknowledgements

*Many thanks to Lee Benfield and Baris Acar for reviewing this article and providing numerous useful suggestions for improvement.*

<hr>
Copyright (c) Roger Orr 2020-04-09 (First published in CVu March 2011)
