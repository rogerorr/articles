# Using the Windows debugging API on Windows 64 #

## Introduction

In March this year I wrote an article in CVu "[Using the Windows Debugging API](Readme.md)" and I illustrated the basic operation of the API
with a simple process tracing application. This showed such events as the loading of DLLs, the starting and ending of threads and any exceptions.

This article uses the experience of porting the program to 64-bit Windows to provide a brief introduction to some of the issues of
 migrating applications from 32-bit Windows to 64-bits.

Note that there are two main flavours of 64-bit hardware that supports Windows. One is the Itanium architecture from Intel and the
 other is the AMD/64 architecture from AMD but now also supported (with minor modifications) by Intel.

However the Itanium architecture has not been a great success and Microsoft announced last year that they are not supporting Itanium
 beyond Windows Server 2008 R2 and Visual Studio 2010. So I am ignoring Itanium in this article. (Interested readers can Google for
 articles such as "How the Itanium Killed the Computer Industry" by John C. Dvorak.)

## A very quick refresher on ProcessTracer

ProcessTracer works by creating a target process using the `CreateProcess()` function in conjunction with the `DEBUG_ONLY_THIS_PROCESS` flag. 
The flag results in Windows creating a channel between the parent and child process allowing the parent to receive notification of
 key events in the child process' lifecycle.

Process Tracer then enters the main debugging loop which consists of:
 - get the next debug event by calling `WaitForDebugEvent()`
 - display details of the event as required
 - acknowledge it with `ContinueDebugEvent()`.
The main loop ends when the child process exits.

Some of the debug events provide virtual addresses, such as the instruction pointer, and Process Tracer maps these to symbolic addresses.
 This functionality is provided by the `SimpleSymbolEngine` class, which wraps the Microsoft DbgHelp API (and was originally written
 about in Overload 67: http://accu.org/index.php/journals/276) 

## Compiling ProcessTracer for 64-bit Windows

I found porting the code to 64-bit Windows was very easy: I set up a command prompt for x64 development using `vcvarsall x64`
 (in the `VC` subdirectory of the Visual Studio 2010 installation) and modified the makefile to write output to the `x64` directory.
 I then ran `nmake` - everything compiled except the stack walking code in `SimpleSymbolEngine.cpp`.

The error is that the code setting up the `StackFrame64` structure was written for 32-bit code and referred to the `Esp`,
 `Ebp` and `Eip` fields of the `CONTEXT` structure. The equivalent fields for the 64-bit code are `Rsp`, `Rbp` and `Rip`. 
 Additionally the machine type in the call to `StackWalk64` must be changed to `IMAGE_FILE_MACHINE_AMD64` from `IMAGE_FILE_MACHINE_I386`.

Here is a code fragment with the changes:

```
  context.ContextFlags = CONTEXT_FULL;
  GetThreadContext(hThread, &context);

  stackFrame.AddrPC.Offset = context.Rip; // was Eip
  stackFrame.AddrPC.Mode = AddrModeFlat;

  stackFrame.AddrFrame.Offset = context.Rbp; // was Ebp
  stackFrame.AddrFrame.Mode = AddrModeFlat;

  stackFrame.AddrStack.Offset = context.Rsp; // Was Esp
  stackFrame.AddrStack.Mode = AddrModeFlat;

  while (::StackWalk64(IMAGE_FILE_MACHINE_AMD64, // was ...I386
    hProcess, hThread,
    &stackFrame, pContext,
    0, ::SymFunctionTableAccess64, ::SymGetModuleBase64, 0))
  {
    // ...
  }
```

The rest of the code compiled (and worked) without problem, which is I think indicative that Microsoft has done a good job of making
 the current version of the API portable between 32-bit and 64-bit. However this was not achieved without some changes: the older
 functions in the DbgHelp library were not portable, hence the introduction over the years of the various Xxxx64 structures and
 types which are portable between 32-bit and 64-bit Windows.

The same thing has been done with much of the Windows API: parameters and data structures are defined using typedefs to the 
appropriately sized underlying values. The same techniques can be used in application code that needs to compile in both environments.

There are two slightly different approaches taken with making the API portable between 32 -and 64 bit windows. In general the
 Windows API has made use of \f2 typedef`s for data types which map to the appropriate sized underlying type for each platform.
 So for example the ULONG_PTR type is defined in the Microsoft documentation as "an unsigned long type used for pointer precision.
 It is used when casting a pointer to a long type to perform pointer arithmetic." So the actual type is 32-bit when compiled for
 32-bit Windows and 64 when compiling for 64-bit applications. However the DbgHelp API has simply widened everything to 64-bits 
 so the code produced for the previous article, targetting 32-bit Windows, was using types like STACKFRAME64 and calling methods like StackTrace64.

## Comparing 32-bit and 64-bit in action

I used a 'minimal' program for testing ProcessTracer. There are various ways to write this using MSVC, here is one way:

```
--- TrivialProgram.cpp ---
#pragma comment(linker, "/nodefaultlib")
int main() \{return 0;\}
int mainCRTStartup() \{return 0;\}
```
When compiled and linked this produces a very small executable with no dependencies.

Here is the output from running the 32-bit version of ProcessTracer from the previous article on 32-bit Windows 7:

```
C:> x86\ProcessTracer x86\TrivialProgram.exe
CREATE PROCESS 3896 at 0x001D1010
LOAD DLL 77340000 ntdll.dll
LOAD DLL 75900000 C:\Windows\system32\kernel32.dll
LOAD DLL 75670000 C:\Windows\system32\KERNELBASE.dll
EXIT PROCESS 0
  Frame       Code address
  0x003EF994  0x773870B4 KiFastSystemCallRet
  0x003EF9A8  0x7736F652 RtlExitUserThread + 65
  0x003EF9B8  0x7594ED73 BaseThreadInitThunk + 25
  0x003EF9F8  0x773A37F5 RtlInitializeExceptionChain + 239
  0x003EFA10  0x773A37C8 RtlInitializeExceptionChain + 194
```
The obvious change we will see when running the 64-bit program is that the addresses are now 64-bit numbers rather
 than 32-bit ones. The output for the equivalent 64-bit program is:

```
C:> x64\ProcessTracer x64\TrivialProgram.exe
CREATE PROCESS 4692 at 0x000000013FE91010
LOAD DLL 0000000077C90000 ntdll.dll
LOAD DLL 0000000077700000 C:\Windows\system32\kernel32.dll
LOAD DLL 000007FEFE3D0000 C:\Windows\system32\KERNELBASE.dll
EXIT PROCESS 0
  Frame       Code address
  0x00000000001DF720  0x0000000077CE15DA NtTerminateProcess + 10
  0x00000000001DF750  0x0000000077CB418B RtlExitUserProcess + 155
  0x00000000001DF790  0x0000000077CD697F RtlExitUserThread + 79
  0x00000000001DF7C0  0x0000000077716535 BaseThreadInitThunk + 21
  0x00000000001DF810  0x0000000077CBC521 RtlUserThreadStart + 33
```
There are a few changes in the call stack on process exit caused by implementation differences between the 32-bit and 64-bit Win32 subsystem.

## Next step - running the 32-bit code on 64-bit

One of the main reasons why the AMD approach to a 64-bit architecture has been more successful than the Intel one is to do with running
 **existing** 32-bit programs. The AMD processor can be run in 64-bit and 32-bit mode and in the latter case it runs the existing
 32-bit x86 instruction set. Hence, at least in theory, you should be able to run existing 32-bit code on the new chip without requiring 
 changes and without any reduction in performance.

Of course, most programs require support from an operating system, so in order to run an existing 32-bit program the OS needs to 
provide an environment that is as close as possible to the 32-bit version of the OS. 64-bit Windows provides such an environment 
"out of the box" and supports 32-bit applications using the 'Windows on Windows 64' subsystem, abbreviated to WOW64, which runs in 
user mode and maps the 32-bit calls to the operating system kernel into an equivalent 64-bit call. This is normally almost invisible
 to the calling program, but the debugging API exposes some of the scaffolding as shall see.

Windows provides a set of 64-bit DLLs in `%windir%\system32` and an equivalent set of 32-bit DLLs in `%windir%\syswow64`. 
In fact the bulk of the binary images in this directory are identical to the same files in the `system32` directory on a 
32-bit Windows installation. (It seems to me an unfortunate naming issue that the **64**-bit DLLs live in system**32** and 
the **32**-bit ones live in syswow**64**, but there it is!)

So here is the output when we run the 32-bit program on 64-bit Windows 7:

```
C:> x86\ProcessTracer x86\TrivialProgram.exe
CREATE PROCESS 4844 at 0x00A21010
LOAD DLL 77E70000 ntdll.dll
UNLOAD DLL 77700000
UNLOAD DLL 77080000
UNLOAD DLL 77700000
UNLOAD DLL 77820000
LOAD DLL 77080000 C:\Windows\syswow64\kernel32.dll
LOAD DLL 76920000 C:\Windows\syswow64\KERNELBASE.dll
EXIT PROCESS 0
  Frame       Code address
  0x001EFC04  0x77E8FCB2 ZwTerminateProcess + 18
  0x001EFC18  0x77ECD5D9 RtlExitUserThread + 65
  0x001EFC28  0x770933A1 BaseThreadInitThunk + 25
  0x001EFC68  0x77EA9ED2 RtlInitializeExceptionChain + 99
  0x001EFC80  0x77EA9EA5 RtlInitializeExceptionChain + 54
```
The two main differences (apart from minor changes in virtual addresses) are firstly the presence of a few `UNLOAD DLL`
 messages with no corresponding `LOAD DLL` (we shall return to these later on) and secondly the subdirectory used for
 system DLLs is `syswow64` rather than `system32`.

The WOW64 subsystem maps all attempts by 32-bit programs to access files in the `system32` subdirectory to requests for 
the equivalent file in the `syswow64` subdirectory. Hence when trivialProgram.exe loads ntdll, kernel32 and kernelbase
 the WOW64 layer transparently loads these files from the `syswow64` directory (even though the PATH does not include it).

This mapping can be turned off, per thread, using two functions designed for this specific purpose: `Wow64DisableWow64FsRedirection`
 and `Wow64RevertWow64FsRedirection`. Note however that, since the redirection was designed to make existing 32-bit applications
 load the right DLL, use of these functions with code that loads system DLLs (implicitly or explicitly) can lead to strange 
 failures. Alternatively the application can use the virtual subdirectory name `sysnative`, which the WOW64 subsystem maps to `system32`.

This can be somewhat confusing in practice when you use a mix of 32-bit and 64-bit programs on the same machine. As a 
simple example, consider this sequence of operations:
- start a command prompt 
- type `notepad` and press Enter.
Windows starts the 64-bit notepad program from C:\Windows\system32.
- type `C:\Windows\SysWow64\cmd.exe` and press Enter to run a 32-bit command shell.
- type `notepad` and press Enter
Windows now starts the **32-bit** notepad program simply because the current shell is a 32-bit process and WOW modifies the 
target directories used when searching the PATH for the notepad program. I defy you to identify which notepad is which:
 but the same sequence of key strokes in two different command shells has executed two different programs because of the
 special treatment of the system32 directory the WOW subsystem.

The other main translation that the WOW64 layer provides is for access to the Windows registry. The 32-bit specific parts of
 the registry are held underneath the `Wow6432Node` key in various places in the registry, such as `HKEY_LOCAL_MACHINE`, and
 so a request from a 32-bit program to `HKLM\Software\Test` is transparently mapped by WOW64 to a request for
 `HKLM\Software\Wow6432Node\Test`. Note that the `Wow6432Node` key name is reserved and applications should not
 program against this value - the registry functions have been enhanced to allow a 32-bit process to see 64-bit registry keys (and vice versa).

I am not going to cover more of this mapping in this article. I refer interested readers to the Microsoft web site: for
 example "Running 32-bit Applications" at http://msdn.microsoft.com/en-us/library/aa384249%28v=VS.85%29.aspx

## Mixing it up

We have so far tried running a pair of 64-bit programs and a pair of 32-bit programs on 64-bit Windows. What happens if we mix and match?

First off, we try to execute the 32-bit process tracer with a 64-bit target:

```
C:> x86\ProcessTracer x64\TrivialProgram.exe
Unexpected exception: Unable to start x64\TrivialProgram.exe: 50
```
This failing error code (50) is "`ERROR_NOT_SUPPORTED`" - Windows does not allow a 32-bit program to **debug** a 64-bit program.
 This makes some sort of sense: an existing 32-bit debugging program would be unlikely to be able to interpret the 64-bit values
 correctly for addresses, register values, etc. As far as I know there is no (documented) way to work around this.

This is one place where an approach like that taken by the DbgHelp API,of widening everything to 64-bits, might have enabled 
cross boundary debugging in this direction.

However we have more success if we try it the other way round:

```
C:> x64\ProcessTracer x86\TrivialProgram.exe
CREATE PROCESS 3004 at 0x0000000000121010
LOAD DLL 0000000077C90000 ntdll.dll
LOAD DLL 0000000077E70000 ntdll32.dll
LOAD DLL 0000000073EF0000 C:\Windows\SYSTEM32\wow64.dll
LOAD DLL 0000000073E90000 C:\Windows\SYSTEM32\wow64win.dll
LOAD DLL 0000000073E80000 C:\Windows\SYSTEM32\wow64cpu.dll
LOAD DLL 0000000077700000 WOW64_IMAGE_SECTION
UNLOAD DLL 0000000077700000
LOAD DLL 0000000077080000 WOW64_IMAGE_SECTION
UNLOAD DLL 0000000077080000
LOAD DLL 0000000077700000 NOT_AN_IMAGE
UNLOAD DLL 0000000077700000
LOAD DLL 0000000077820000 NOT_AN_IMAGE
UNLOAD DLL 0000000077820000
LOAD DLL 0000000077080000 C:\Windows\syswow64\kernel32.dll
LOAD DLL 0000000076920000 C:\Windows\syswow64\KERNELBASE.dll
EXCEPTION 0x4000001f at 0x0000000077F10F3B LdrVerifyImageMatchesChecksum + 2412
  Parameters: 0
  Frame       Code address
  0x000000000046F414  0x0000000077F10F3C LdrVerifyImageMatchesChecksum + 2413
  0x000000000046F41C  0x000000007716936D WakeConditionVariable + 127125
  0x000000000046F424  0x7EFDE00000000000
  0x000000000046F42C  0x0046F41C00000000
  0x000000000046F434  0x0046F60477EE1ECD
  0x000000000046F43C  0x00B9DABD77EE1ECD
  0x000000000046F444  0x0046F5C400000000
  0x000000000046F44C  0x7EFDD00077EF1323
  0x000000000046F454  0x77F7206C7EFDE000
EXIT PROCESS 0
  Frame       Code address
  0x000000000029E190  0x0000000077CE15DA NtTerminateProcess + 10
  0x000000000029E1C0  0x0000000073F0601A Wow64EmulateAtlThunk + 34490
  0x000000000029EA80  0x0000000073EFCF87 Wow64SystemServiceEx + 215
  0x000000000029EB40  0x0000000073E82776 TurboDispatchJumpAddressEnd + 45
  0x000000000029EB90  0x0000000073EFD07E Wow64SystemServiceEx + 462
  0x000000000029F0E0  0x0000000073EFC549 Wow64LdrpInitialize + 1065
  0x000000000029F5D0  0x0000000077CD4956 RtlUniform + 1766
  0x000000000029F640  0x0000000077CD1A17 RtlCreateTagHeap + 167
  0x000000000029F670  0x0000000077CBC32E LdrInitializeThunk + 14
```
However some of the output produced is a little unexpected! After all, we are debugging **exactly** the same process that
 we debugged earlier under "Next step - running the 32-bit code on 64-bit" but we are getting much more output and a very
 different call stack on program exit. What is going on?

The first change when using the 64-bit debug API is that the debugger is notified about additional DLLs loaded into 
the process address space. The first DLL loaded, `ntdll.dll`, is in fact the **64-bit** version of the DLL and it is 
followed by the 32-bit `ntdll32.dll`. Note that the 32-bit mode debugger doesn't see the 64-bit DLL, only the 32-bit one.

The next few DLLs are the WOW64 implementation - these are 64-bit DLLs loaded into the target process. Again, the 32-bit 
mode debugger does not receive any notification when these DLLs are loaded into the process. Notice however that, probably
 through an oversight in the implementation of the 32-bit debug interface, the 32bit debugger sees the` UNLOAD DLL` messages 
 for the special WOW64 DLLs.

We then receive an unexpected exception - code `0x4000001f `- which turns out to be a new value, STATUS_WX86_BREAKPOINT.
This exception code is barely documented but it appears to be a 'start up' breakpoint much like the initial breakpoint
that the debugger always receives. On older versions of Windows the default processing for this exception code terminates
the process, on Windows 7 it is ignored.

So I enhanced the exception handling in `ProcessTracer::run()` to cater for this new breakpoint:

```
  case EXCEPTION_DEBUG_EVENT:
    if (!attached)
    {
      // First exception is special
      attached = true;
    }
#ifdef _M_X64
    else if (DebugEvent.u.Exception.ExceptionRecord.ExceptionCode ==
             STATUS_WX86_BREAKPOINT)
    {
      std::cout << "WOW64 initialised" << std::endl;
    }
#endif // _M_X64
    else
    {
      OnException(DebugEvent.dwThreadId, 
        DebugEvent.u.Exception.dwFirstChance, 
        DebugEvent.u.Exception.ExceptionRecord);
      continueFlag = (DWORD)DBG_EXCEPTION_NOT_HANDLED;
    }
    break;
```

The last issue with the output from the debugger is that the call stack shown on process exit has almost nothing in
 common with the call stack from the 32-bit version of process tracer.
This is because the 64-bit debugger is displaying the call stack in the 64-bit context of the WOW64 call to terminate
 the process. In the 32-bit debugger we see the other half of the picture - the call stack in the 32-bit part of the
 call stack **above** the WOW64 emulation layer.

## Reading 32-bit context from 64-bit debugger

Windows provides a way for 64-bit debuggers to access the same data the 32-bit debug interface gives. The debugger
 can use `IsWow64Process()` to determine if the target process is a 32-bit process or a 64-bit one, and in the former
 case it can read the 32-bit context using `Wow64GetThreadContext()`. (Note: this API was added in Windows 7. The
 information can be obtained on earlier versions of 64-bit Windows but the mechanism is a little more complicated).

Once the 32-bit context has been obtained it can be used to populate the `StackFrame64` structure and passed into to the
`StackWalk64()` function together with the `IMAGE_FILE_MACHINE_I386` machine type to print out the 32-bit call stack.

Here is the final version of the stack walking code, which compiles on both 32-bit and 64-bit builds and handles
 32-bit targets in 64-bit mode.

```
void SimpleSymbolEngine::stackTrace(HANDLE hThread,
  std::ostream & os )
 {
  CONTEXT context = {0};
  PVOID pContext = &context;

  STACKFRAME64 stackFrame = {0};

#ifdef _M_IX86
  DWORD const machineType = IMAGE_FILE_MACHINE_I386;
  context.ContextFlags = CONTEXT_FULL;
  GetThreadContext(hThread, &context);

  stackFrame.AddrPC.Offset = context.Eip;
  stackFrame.AddrPC.Mode = AddrModeFlat;

  stackFrame.AddrFrame.Offset = context.Ebp;
  stackFrame.AddrFrame.Mode = AddrModeFlat;

  stackFrame.AddrStack.Offset = context.Esp;
  stackFrame.AddrStack.Mode = AddrModeFlat;
#elif _M_X64
  DWORD machineType;

  BOOL bWow64(false);
  WOW64_CONTEXT wow64_context = {0};
  IsWow64Process(hProcess, &bWow64);
  if (bWow64)
  {
    machineType = IMAGE_FILE_MACHINE_I386;
    wow64_context.ContextFlags = WOW64_CONTEXT_FULL;
    Wow64GetThreadContext(hThread, &wow64_context);
    pContext = &wow64_context;
    stackFrame.AddrPC.Offset = wow64_context.Eip;
    stackFrame.AddrPC.Mode = AddrModeFlat;

    stackFrame.AddrFrame.Offset = wow64_context.Ebp;
    stackFrame.AddrFrame.Mode = AddrModeFlat;

    stackFrame.AddrStack.Offset = wow64_context.Esp;
    stackFrame.AddrStack.Mode = AddrModeFlat;
  }
  else
  {
    machineType = IMAGE_FILE_MACHINE_AMD64;
    context.ContextFlags = CONTEXT_FULL;
    GetThreadContext(hThread, &context);

    stackFrame.AddrPC.Offset = context.Rip;
    stackFrame.AddrPC.Mode = AddrModeFlat;

    stackFrame.AddrFrame.Offset = context.Rbp;
    stackFrame.AddrFrame.Mode = AddrModeFlat;

    stackFrame.AddrStack.Offset = context.Rsp;
    stackFrame.AddrStack.Mode = AddrModeFlat;
  }
#else
#error Unsupported target platform
#endif // _M_IX86

  DWORD64 lastBp = 0; // Prevent loops with optimised stackframes

  os << "  Frame       Code address\n";
  while (::StackWalk64(machineType,
    hProcess, hThread,
    &stackFrame, pContext,
    0, ::SymFunctionTableAccess64, ::SymGetModuleBase64, 0))
  {
    if (stackFrame.AddrPC.Offset == 0)
    {
      os << "Null address\n";
      break;
    }
    PVOID frame = 
      reinterpret_cast<PVOID>(stackFrame.AddrFrame.Offset);
    PVOID pc = 
      reinterpret_cast<PVOID>(stackFrame.AddrPC.Offset);
    os << "  0x" << frame << "  " << addressToString(pc) << "\n";
    if (lastBp >= stackFrame.AddrFrame.Offset)
    {
      os << "Stack frame out of sequence...\n";
      break;
    }
    lastBp = stackFrame.AddrFrame.Offset;
  }

  os.flush();
}
```
The reading of the 32-bit context from 64-bit mode also works where the program is currently executing in 
32-bit mode (such as when a breakpoint occurs).

The transition between 32-bit and 64-bit mode occurs when the 32-bit program calls into the operating system.
 In the case we've looked at when our 32-bit program exits it calls `ZwTerminateProcess` in the 32-bit world.
 This function sets up parameters in exactly the same way as it does on 32-bit Windows but then jumps to
 `X86SwitchTo64BitMode` in the WOW64 subsystem. This jump switches the CPU from 32-bit mode back into 64-bit mode. 
 The target function saves away the 32-bit register state and (after some further processing) calls into the
 operating system in 64-bit mode. The reverse process occurs on return from 64-bit mode when the function completes.

The `Wow64GetThreadContext()` function retrieves the 32-bit context by reading it from the memory location where 
the WOW64 process has saved it, when the target thread is in 64-bit mode, or by mapping the current context back 
into the 32-bit structure when the target thread is in 32-bit mode.

## Conclusion

Microsoft have done a reasonably good job of allowing application to be ported to 64-bit operation and also 
support transparently running 32-bit applications on 64-bit Windows. For many users whether a particular application 
is 32-bit or 64-bit is simply not relevant, nor obvious.

However, as programmers, it is useful to have understanding of how the process operates and what issues there can be; 
especially when running existing 32-bit applications on 64-bit.

The debugging API provides one way of looking at some of the implementation details of supporting the Windows-on-Windows
 subsystem and I hope this understanding will help inform those migrating from 32-bit to 64-bit Windows.

## Source code

The full source code for this article can be found at https://github.com/rogerorr/articles/tree/master/Simple_Debugger

<hr>
Copyright (c) Roger Orr 2021-07-05 (First published in CVu Jan 2012)
 