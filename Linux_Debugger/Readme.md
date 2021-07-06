# Using the Unix PTrace API

## Introduction

In a previous article "[../Simple_Debugger/Readme.md](Using the Windows Debugging API)" I wrote about the Windows debugging API 
and promised to write a future article about the Unix debug API. Here is that article: slightly later than anticipated.

Many programmers spend a significant amount of time using debuggers but in my experience few of them have much understanding of 
how they work. It can be useful to look at this, for a couple of reasons:
- knowing a little about how something works can help you use it better
- you might wish to write your own tool to make use of the same API

The classic mechanism for debugging application programs in Unix is based around the `ptrace` function. This function allows the 
controlling process to perform a variety of tasks on the target process: for example to read and write memory, to examine and 
change the registers and to receive notifications of signals received by the target process. This forms the basis of the interactive
 debuggers `dbx` and `gdb`. Additionally Unix can invoke the `ptrace` interface whenever the target process makes a system call.
 The Unix `strace` utility uses this mechanism to intercept and record the system calls which are called by a process, together
 with the name of each system call, its arguments and its return value.

There are a number of problems with this API - it was originally designed a long time ago and some of its features have become
 slightly problematic since. Moreover, each flavour of Unix has made slightly different implementation decisions and often also
 added extensions to the basic API. There seem to be regular attempts to produce a new API without some of the peculiarities 
 of the existing implementations, but given the widespread availability of the API and the existence of many cross-platform tools 
 which use `ptrace` (albeit often containing platform-specific code to handle the differences) there has not yet been a clear
 successor.  Solaris, for instance, has added other debugging and profiling mechanisms which they consider better and removed
 `ptrace` as a system call - but the function is still *available* as it is emulated using the newer functionality.

## What's in the Unix debugging API?

Debugging is invoked when a process calls `ptrace` with the `PTRACE_TRACEME` argument (i.e. requesting to be debugged) or a 
process can try to request debugging of a target process by calling `ptrace` with the `PTRACE_ATTACH` argument. Only one process 
at a time can attach to a process so this call willl fail if a debugger is already attached. The use of `PTRACE_ATTACH` may be
 restricted by security policies on various versions of Unix - typically it only works if the target process has the same user 
 id but stronger restrictions can be - and often are - available. For example, on Mac OS X a process can use the
 `PTRACE_DENY_ATTACH` function code to prevent it from being debugged. This is one of the places where the age of the API shows 
 -- the security concerns often a necessary part of today's computing world were largely absent when `ptrace` was designed.

Once successfully attached to the target the debugging process typically runs in a loop, calling one of the `wait` functions 
to receive the next status change of the target and then using `ptrace` to inspect and/or modify the target process and then 
to continue its execution.

When using an interactive debugger it is common to set breakpoints in the target program - this is done by modifying the code 
in the target to add a breakpoint instruction and then responding to the resultant event. There is a lot of work behind the
 scenes in a good debugger to do such things as resolving the low level addresses and register values into high level constructs
 such as line numbers, source code and variables; these techniques are not covered in this article.

A debugger can detach from a debuggee by using `ptrace(PTRACE_DETACH, pid, 0, 0)` , letting the target process run freely. 
Unless this is done, when the debugging process terminates the target process is also automatically terminated. 

## Linux ProcessTracer

I am going to use a program that traces calls to `open` and `close` (and major events in a program's lifecycle) to provide the 
framework for exploring `ptrace`. Although the basic shape of the program would be the same on *any* flavour of Unix I 
have written and tested the program on Linux; I will try to point out in the text where the program is Linux-specific.

I will demonstrate the action of the process tracer by targetting an example program named `BadProgram`. The source code for
 `BadProgram` is very simple:

```
#include <stdio.h>

int main(int argc, char **argv)
{
  int idx;
  for (idx = 1; idx < argc; ++idx)
  {
    printf("** opening %s\n", argv[idx]);
    FILE *fp = fopen(argv[idx], "r");
    fclose(fp);
  }
}
```

As is probably apparent, if any of the command line arguments refer to a file that cannot be opened for read, the call to
 `fclose` will segfault when it tries to access the null value of `fp`.

Here is an example of the tracing in use on the example program when it accesses a null pointer after failing to open the 
`noexist` file:

```
$ ./ProcessTracer ./BadProgram /dev/null noexist
open("/etc/ld.so.cache") = 3
close(3) = 0
open("/lib/x86_64-linux-gnu/libc.so.6") = 3
close(3)
** opening /dev/null
open("/dev/null") = 3
close(3)
** opening noexist
open("noexist") = -2(No such file or directory")
Segmentation violation
Terminated by signal 11
```

Here we can see *four* calls to `open`. The first two calls are from the program loader itself opening files as part of 
starting the program. These calls are then followed by the two calls from the example code: a successful call to open
 `/dev/null` and a failed call to open `noexist`.

Now we've seen the process tracer in action let's go back to the beginning and build it up step by step.

## Getting started

The first step in ProcessTracer is to create the child process with tracing enabled. As describe above this is done by making a
 call to `ptrace` with argument `PTRACE_TRACEME` in the **child\b0  process. In Unix the standard way to create a child process 
 is to clone the current process using `fork `and then call `exec` to replace the clone with the desired target. The usual place 
 for the call to `ptrace` is therefore to put it in the code executed in the cloned child process after the call to `fork` which 
 created the new process and before the `exec` which loads the target.

Here is an extract from ProcessTracer (without error handling, for clarity) showing this:

```
pid_t const cpid = fork();
if (cpid > 0)
{
  // In the parent
  return cpid; 
}
else if (cpid == 0)
{
  // In the new child
  if (ptrace(PTRACE_TRACEME, 0, 0, 0) == -1)
  {
    // Handle error
  }
  execv( argv[0], argv );
  // Handle failing to start the new process
}
```
At this point the original process has been provided with the process ID of the new child and, since the child process has asked 
to be debugged, the call to `execv` in the child process is blocked internally waiting for the parent process to handle the 
associated ptrace event.

We must now take a look at the main debug loop.

## The heart of the matter

In its simplest form the "debug loop" looks like this:

```
void TrivialPtrace::run()
{
  int status(0); // Status of the stopped child process
  while ((pid = wait(&status)) != -1)
  {
    int send_signal(0);
    if (WIFSTOPPED(status))
    {
      int const signal(WSTOPSIG(status));
      os << "Signal: " << signal << std::endl;
      if (signal != SIGTRAP) send_signal = signal;
    }
    else if (WIFEXITED(status))
    {
      os << "Exit(" << WEXITSTATUS(status) << ")" << std::endl;
    }
    else if (WIFSIGNALED(status))
    {
      os << "Terminated: signal " << WTERMSIG(status)
         << std::endl;
    }
    else if (WIFCONTINUED(status))
    {
      os << "Continued" << std::endl;
    }
    else
    {
      os << "Unexpected status: " << status << std::endl;
    }

    ptrace(PTRACE_CONT, pid, 0, send_signal);
  }
  if (errno != ECHILD)
  {
    throw make_error("wait");
  }
}
```
This loop is purely reactive and isn't doing anything more than displaying the various debug events. Let's look at what it does 
and then extend it further in a moment.

The first call, to `wait`, halts the debugger until the next event is ready from the debuggee. On a successful return, the process 
id of the child raising the event is returned. The `wait` call returns -1 to indicate there are no more debug events to process -
 the child process(es) have completed and we can leave the main loop.

We check the error number is ECHILD (no children) and if not raise an error. (The implementation of `make_error` is in the full source
 code for the article.)

The call returns a status value for the event, and we use a set of macros defined in `wait.h` to extract the event type and any related 
arguments from the status field. As can be seen from the structure of the loop the possible event types are:

### Stopped (`WIFSTOPPED`)

The process has stopped at a signal event and the status contains the signal number. Normally you simply want to pass the signal 
on to the process that reported it so it can do its normal processing on the signal. One exception however is the `SIGTRAP` signal 
which is generated by a breakpoint and is also used by ptrace for some of the events that it generates. It is important that the
 `SIGTRAP` is **not** passed on to the target process as the default action is to terminate the process.

 The very first event you receive when debugging is a stop event from the child process.  (In our case this occurs when the child
 process is inside the call to `execv`.) This event gives the debugger a chance to set up the debugging environment and to decide how
 to let the child process continue. The only exception to this is if the `execv` fails because, for instance, the target binary does 
 not exist. In this case you may receive an 'Exited' event first, so for robustness you must ensure this possibility is catered for.

### Exited (`WIFEXITED`)

The process has called `exit` (or otherwise terminated normally) with the specified exit status.

### Signaled (`WIFSIGNALED`)

The process has terminated in response to the specified signal (for example, `SIGABRT` from calling `abort`)

### Continued (`WIFCONTINUED`)
The process has resumed by means of `SIGCONT` (newer versions of Linux only)

The final stage of the main loop is calling `ptrace` with the `PTRACE_CONT` argument which resumes the child process until the next 
debug event. The code then goes back to the controlling `wait` call until one of the next events occurs. Other arguments are possible
 when resuming the child process - for example `PTRACE_SINGLESTEP`, which resumes the child process for just a single instruction;
 or `PTRACE_SYSCALL`, which resumes the child process until the next time it enters or leaves a system call or generates a normal 
 debug event; we will use this value later on in the article.

Here is the output from running this trivial `ptrace` example against our "bad" program:

```
$ ./TrivialPtrace ./BadProgram /dev/null noexist
Signal: 5
** opening /dev/null
** opening noexist
Signal: 11
Terminated: signal 11
```
The first output "`Signal 5`" is caused by the initial `SIGTRAP` signal event returned by the `ptrace` API when the connection to 
the target process is first made. This first event would typically be used to initialise the debugger for the target process.

A quick check of `signal.h` shows that signal 11(on my system) is `SIGSEGV` - segmentation violation. The full `processTracer`
 program adds code to map signal numbers to more 'friendly' strings.

## Thread and process start and stop

The tracing program at this point suffers from what can be a fairly important restriction - it will debug only the initial target 
thread and process selected. We can demonstrate this by invoking a shell and then running our bad program from the shell.

```
$ ./TrivialPtrace /bin/sh
Signal: 5
$ ./BadProgram /dev/null noexist
** opening /dev/null
** opening noexist
Signal: 17
Segmentation fault (core dumped)
$ ^D
Exit(0)
```
Here we see no sign of the `SEGSEGV` signal (11) but only the `SIGCHLD` signal (17) which is received by the shell when *its* child 
process - `BadProgram` - exits.
It is very common to want to follow processes (and threads) created by the target process. How can we do this with `ptrace`?

In the early days of `ptrace` the usual way to do this was to turn on system call tracing and process the calls that created new processes,
 such as `fork`. When `fork` returns in the parent process the return value is the `pid` of the new child process. The debugger could
 then issue a call to `ptrace(PTRACE_ATTACH, pid, 0, 0)`. Unfortunately this was subject to a race condition as the child process may
 have already executed a number of instructions before the debugger received the event via `ptrace` and was able to attach to the new 
 pid. This mechanism is still available and may be used for portability reasons.

However Linux has added some extensions to `ptrace` to improve support for debugging child process and also to provide support for
 debugging multiple threads. *Note* this is one of the places where Linux diverges from other versions of Unix as it has provided 
 its own implementation for supporting threads, via the `clone` system call. 

I found a lot of questions on the Internet about the best way to program this - it seems that the documentation is not entirely 
clear. Here are the steps I have found minimally sufficient:

**Request tracing of child processes\b0
When the child process is stopped at the initial stop event we set additional `ptrace` options `PTRACE_O_TRACEFORK`,
 `PTRACE_O_TRACEVFORK` and `PTRACE_O_TRACECLONE`. This makes the system automatically turn on tracing for processes and threads 
 created by the three system calls `fork`, `vfork` and `clone`. Additionally, the newly created processes and threads in turn 
 inherit these `ptrace` settings. **Note** that setting these options can only be done when the child process is stopped on a `ptrace` event.

**Wait for all the child processes\b0
We have to replace `wait` with a call to `waitpid` and provide a Linux-specific value `__WALL` for the third argument. If we fail 
to do this we do not receive signals for all the additional tasks.

**Process the additional events\b0
One we have set the options above the system will deliver additional debug events using the 'Stopped' status. The debugging process 
receives notifications from the parent process with signal type `SIGTRAP` with additional flags OR'd in to the status value to indicate 
which system call the event was raised by. Additionally the debugger also receives an initial `SIGSTOP` signal from each newly created 
thread or process. It seems necessary, despite what the `ptrace` documentation states, to avoid sending on the `SIGSTOP` signal to the 
process being debugged. Here too `Linux` has added a further option, `PTRACE_O_TRACEEXEC`, which can be enabled to OR an additional flag 
value into these initial events.

I modified the earlier example to create a `MultiPtrace` class which handles threads and processes. I refactored the handling of stopped 
events into a separate function: `OnStop` which is invoked in the debug loop like this:

```
if (WIFSTOPPED(status))
{
  send_signal = OnStop(WSTOPSIG(status), status >> 16);
}
```
The `OnStop `function itself is:

```
int MultiPtrace::OnStop(int signal, int event)
{
  if (!initialised)
  {
    initialised = true;
    long const options =
      PTRACE_O_TRACEFORK |
      PTRACE_O_TRACEVFORK |
      PTRACE_O_TRACECLONE;
    if (ptrace(PTRACE_SETOPTIONS, pid, 0, options) == -1)
    {
      throw make_error("PTRACE_SETOPTIONS");
    }
    return 0;
  }

  if (event) 
  {
    os << "Event: " << event << std::endl;
  }
  else
  {
    os << "Signal: " << signal << std::endl;
  }
  return (signal == SIGTRAP || signal == SIGSTOP) ? 0 : signal;
}
```
Now when we run the previous example we receive the debugging events from *both* the direct child, the shell, and *also* from its child processes:

```
$ ./MultiPtrace /bin/sh
$ ./BadProgram /dev/null noexist
Event: 1
Signal: 19
Signal: 5
** opening /dev/null
** opening noexist
Signal: 11
Terminated: signal 11
Signal: 17
Segmentation fault (core dumped)
$ ^D
Exit(0)
```
## System calls

The next step is to look at tracing system calls; which is enabled by simply replacing `PTRACE_CONT` with `PTRACE_SYSCALL` in the main
 debugging loop. Having done this we get two more events on every system call; one event on entry to the system call and one event just 
 before exiting the system call. The event returned is a 'stopped' event with the stop signal value `SIGTRAP`. This is the same value used 
 for a software breakpoint event and while this makes sense of what is occurring it can mean additional work by the debugger to differentiate 
 between the two cases.

This overloading of the `SIGTRAP` processing is unnecessary - a distinct value for a system call event would have been a cleaner design. 
(Those who read the earlier article on the Windows debugging interface may recall a similar overload with the so-called 'initial breakpoint' 
event sent to notify the debugger that the initialisation has completed.) Some later versions of `ptrace` now support an additional option -
`PTRACE_O_TRACESYSGOOD` - which, when set, changes the signal value by ORing it with 0x80 and hence removes the ambiguity. Prior to this 
option (and for systems not supporting it) you normally disambiguate the two cases by calling `ptrace` with the `PTRACE_GETSIGINFO` 
argument to return information about the underlying signal - the returned value of `si_code` will be `SIGTRAP` for the system call 
entry/exit case. 

Now we can expand a little further on the initial `SIGTRAP` event we saw in the `TrivialPtrace` example: this is actually a system 
call exit event for the `execve` call in the child process.

The events generated on entry and exit to a system call are distinguished by the value of the register used for the return code, 
which contains a special value of `-ENOSYS` on entry and the actual return value or error code on exit. Note that this technique
 works as no system call returns `-ENOSYS` on failure.

This information is not provided by the ptrace event itself, the debugger has to make an additional call using the `PTRACE_GETREGS` 
function code to read the register values for the target process. We then have to get down to the specifics of the architecture - 
*which* register will contain the return code we are we looking at? The documentation for each hardware platform includes a section 
on calling conventions and the system calling convention gives us the information we are looking for.

**System call conventions on x86 and x64
\b0
The Linux convention on Intel x86 hardware is to use the `eax` register for the system call number on entry and the return code on 
exit. The arguments to the system call (up to six arguments depending on the call) are passed in `ebx`, `ecx`, `edx`, `esi`, `edi`
 and `ebp`. 

As described above, when a syscall entry event occurs the `eax` value is overwritten by the special value `-ENOSYS`. The `ptrace`
 interface therefore saves the original `eax` value in `orig_eax` so the debugger still has access to the system call number. It 
 also does this on the system call exit case so the debugger knows *which* system call is exiting.

The basic logic looks something like this:

```
  struct user_regs_struct regs;
  if (ptrace(PTRACE_GETREGS, pid, 0, &regs) == -1)
  {
    throw make_error("ptrace(PTRACE_GETREGS)");
  }

  int const rc = regs.eax;
  int const func = regs.orig_eax;

  if (rc == -ENOSYS)
  {
    OnCallEntry(func, regs.ebx, regs.ecx, ...);
  }
  else
  {
    OnCallExit(func, rc);
  }
```
The actual handling of each function will obviously involve decoding the actual value of each of the valid arguments to the function call.

For the x64 API the `rax` register replaces the `eax` register and the system call arguments are passed in `rdi`, `rsi`, `rdx`, `r10`,

 `r8` and `r9` with the original system call number returned in `orig_rax`. The resultant code is very similar to that shown above for the x86 case.

## Turning a register value into a file name

The process tracer program in this article is interested in just two system calls: `open` and `close`. The corresponding system call
 numbers are `__NR_open` and `__NR_close`.
The close call takes a single argument, the file handle to close, and so displaying this is easy. The `open` call is more complicated 
as we are interested in the first argument ("`const char * *path\i0`") and this argument is the **address** of the character string. 

The return code from the system call is negative on error and the error code is the negation of this number. It can be displayed as a 
text string by using the standard `strerror()` function. (This is a slight simplification: only 'small' negative numbers indicate an error.)

We need to invoke `ptrace` again to read the string, this time using the `PTRACE_PEEKDATA` function code. This function can be slightly
 awkward to use as it only provides access to memory one word at a time.
In order to read a `NUL`-terminated string we must bear in mind that the start of the string may not be aligned on a word boundary, so
 we may need to read (and ignore) a few bytes before the string starts. The algorithm then reads words one at a time and processes characters
 from each word until the terminating null is located.

Here is an implementation of this algorithm:

```
std::string ProcessTracer::readString(long addr)
{
  std::string result;

  int offset = addr % sizeof(long);
  char *peekAddr = (char *)addr - offset;

  // Loop round to find the end of the string
  bool stringFound = false;
  do
  {
    long const peekWord = 
      ptrace( PTRACE_PEEKDATA, pid, peekAddr, NULL );
    if ( -1 == peekWord )
    {
       throw make_error("ptrace(PTRACE_PEEKDATA)");
    }
    char const * const tmpString = (char const *)&peekWord;
    for (unsigned int i = offset; i != sizeof(long); i++)
    {
      if (tmpString[i] == '\0')
      {
        stringFound = true;
        break;
      }
      result.push_back(tmpString[i]);
    }
    peekAddr += sizeof(long);
    offset = 0;
  } while ( !stringFound );
  return result;
}
```
A further difficulty with the access mechanism occurs when reading arbitrary data (this issue doesn't occur when reading printable character
 strings) as the return code of -1 may be caused by an error reading from the target **or** by reading the value of -1 from the target process.
 The only way to differentiate between these two cases is to set `errno` to zero before making the call and examine its value afterwards: 
 it will still be zero for successful completion and the error number on failure.

This is another place where the original design of `ptrace` is showing signs of age. The mechanism is unwieldy to use and causes performance
 problems when reading larger data structures as each word of memory read involves a separate system call.

Many versions of Unix now provide other ways to access memory from another process. In Linux we can make use of the `/proc` pseudo-filesystem 
to access the memory of the target process using the filesystem API to read `/proc/*pid*/mem` for the target process id.

Here is the same method reimplemented in terms of this filesystem:

```
std::string ProcessTracer::readString(long addr)
{
  std::string result;  std::ostringstream os;
  os << "/proc/" << pid << "/mem";
  std::ifstream mem(os.str().c_str(), std::ios::binary);
  mem.exceptions(std::ios::failbit);
  mem.seekg((std::streampos)addr);
  std::getline(mem, result, '\0');
  return result;
}
```

In production code it is likely that the open file stream would be cached for performance.

## Extending process tracer

We have now completed the implementation of the simple process tracer demonstrated at the beginning of the article. The basic framework
 is in place to handle the debug events generated by the child process(es) and thread(s) and we have looked at how to access data in the
 target process.

The standard system tool `strace` performs all this, and more, and for most of the process tracing needs you might have this is likely
 to be the right solution! It is already configured to understand and display the various system call argument types and has a range
 of options available.

However there are times when a more specific tool is required and techniques like the ones described in this article can be used to
 implement such tools.

## Conclusion

I have covered only the basics of a call tracer in this article and there is obviously a lot more that must be added to write a proper
 interactive debugger. However I hope that the overview of the debug API that I have presented here has given you some understanding
 of the bare bones of the interaction between the debugger and the target; and some sympathy for the complexity of the task involved
 in providing a tool such as `gdb` or `strace`.

## Acknowledgements

Many thanks to Irfan Butt for reviewing this article and correcting a number of mistakes.

### Source code
The full source code for this article can be found [in github](https://github.com/rogerorr/articles/tree/master/Linux_Debugging).

<hr>
Copyright Roger Orr 2021-07-06 - First published in C Vu, Nov 2012.