/*
NAME
    ProcessTracer

DESCRIPTION
    About the simplest Linux debugger which is useful!

COPYRIGHT
    Copyright (C) 2012 by Roger Orr <rogero@howzatt.co.uk>

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

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>
#include <asm/unistd.h>
#include <sys/ptrace.h>
#include <sys/user.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace
{
  std::runtime_error make_error(std::string const &action)
  {
    std::string const what(action + " failed: " + strerror(errno));
    return std::runtime_error(what);
  }

  static struct
  {
    int code;
    char const *name;
  } signals[] = {
  { SIGHUP,    "hangup" },
  { SIGINT,    "interrupt" },
  { SIGQUIT,   "quit" },
  { SIGILL,    "illegal instruction" },
  { SIGTRAP,   "trap" },
  { SIGABRT,   "abort" },
  { SIGBUS,    "bus error" },
  { SIGFPE,    "floating point exception" },
  { SIGKILL,   "kill" },
  { SIGUSR1,   "user 1" },
  { SIGSEGV,   "segmentation violation" },
  { SIGUSR2,   "user 2" },
  { SIGPIPE,   "broken pipe" },
  { SIGALRM,   "alarm" },
  { SIGTERM,   "terminate" },
  { SIGSTKFLT, "stack fault" },
  { SIGCHLD,   "child" },
  { SIGCONT,   "continue" },
  { SIGSTOP,   "stop" },
  { SIGTSTP,   "tty stop" },
  { SIGTTIN,   "tty in" },
  { SIGTTOU,   "tty out" },
  { SIGURG,    "urgent" },
  { SIGXCPU,   "exceeded CPU" },
  { SIGXFSZ,   "exceeded file size"},
  { SIGVTALRM, "virtual alarm" },
  { SIGPROF,   "profiling" },
  { SIGWINCH,  "window size change" },
  { SIGPOLL,   "poll" },
  };

  /** Strema helper for signals */
  class sigstrm
  {
    int const signal;
  public:
    sigstrm(int signal) : signal(signal) {}

    friend std::ostream & operator<<(std::ostream& os, sigstrm const &rhs)
    {
      int idx = 0;
      for (; idx != sizeof(signals)/sizeof(signals[0]); ++idx)
      {
        if (signals[idx].code == rhs.signal)
        {
          os << signals[idx].name;
          break;
        }
      }
      if (idx == sizeof(signals)/sizeof(signals[0]))
      {
        os << "signal " << rhs.signal;
      }
      return os;
    }
  };
} // namespace

/** Simple ptrace user */
class ProcessTracer
{
private:
  pid_t pid;
  std::ostream &os;
  bool initialised;

public:
  /** Create */
  ProcessTracer(pid_t pid, std::ostream &os)
  : pid(pid), os(os), initialised(false) {}

  /** Run the debug loop */
  void run();

private:
  /** Stop received */
  int OnStop(int signal, int event);

  /** PTrace event received */
  void OnEvent(int event);

  /** System trap received */
  void OnTrap();

  /** System call received */
  void OnSysCall();

  /** Check if specified system call is of interest */
  bool SelectedCall(int func);

  /** System call 'func' being entered */
  void OnCallEntry(int func, long int args[]);

  /** Sytem call 'func' being exited */
  void OnCallExit(int func, int rc);

  /** Signal received */
  bool OnSignal(int signal);

  /** Read a string from the taget process */
  std::string readString(long addr);
};

/////////////////////////////////////////////////////////////////////////////////////////////////
void ProcessTracer::run()
{
  int status(0); // Status of the stopped child process

  // This is the main tracing loop. When the child stops,
  // we examine the system call and its arguments
  while ((pid = waitpid(-1, &status, __WALL)) != -1)
  {
    int send_signal(0);
    if (WIFSTOPPED(status))
    {
      send_signal = OnStop(WSTOPSIG(status), status >> 16);
    }
    else if (WIFEXITED(status))
    {
      os << "Exit(" << WEXITSTATUS(status) << ")" << std::endl;
    }
    else if (WIFSIGNALED(status))
    {
      os << "Terminated: " << sigstrm(WTERMSIG(status)) << std::endl;
    }
    else if (WIFCONTINUED(status))
    {
      os << "Continued" << std::endl;
    }
    else
    {
      os << "Unexpected status: " << status << std::endl;
    }

    ptrace(PTRACE_SYSCALL, pid, 0, send_signal);
  }
  if (errno != ECHILD)
  {
    throw make_error("wait");
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
int ProcessTracer::OnStop(int signal, int event)
{
  if (!initialised)
  {
    initialised = true;
    long const options = PTRACE_O_TRACESYSGOOD | PTRACE_O_TRACEFORK |
                         PTRACE_O_TRACEVFORK | PTRACE_O_TRACECLONE |
                         PTRACE_O_TRACEEXEC;
    if (ptrace(PTRACE_SETOPTIONS, pid, 0, options) == -1)
    {
      throw make_error("PTRACE_SETOPTIONS");
    }
  }
  else if (signal == SIGTRAP)
  {
    if (event)
    {
      OnEvent(event);
    }
    else
    {
      OnTrap();
    }
  }
  else if (signal == (SIGTRAP | 0x80))
  {
    OnSysCall();
  }
  else
  {
    if (OnSignal(signal))
    {
      return signal;
    }
  }
  return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
void ProcessTracer::OnEvent(int event)
{
  unsigned long message(0);
  switch (event)
  {
  case PTRACE_EVENT_CLONE:
  case PTRACE_EVENT_FORK:
  case PTRACE_EVENT_VFORK:
    if (ptrace(PTRACE_GETEVENTMSG, pid, 0, &message) == 0)
    {
      os << "New pid: " << message << std::endl;
    }
    break;
  }
}
  
/////////////////////////////////////////////////////////////////////////////////////////////////
void ProcessTracer::OnTrap()
{
  siginfo_t siginfo;
  if (ptrace(PTRACE_GETSIGINFO, pid, 0, &siginfo) == -1)
  {
     throw make_error("ptrace(PTRACE_GETSIGINFO)");
  }
  if (siginfo.si_code == SIGTRAP)
  {
    OnSysCall();
  }
  else
  {
    os << "Breakpoint" << std::endl;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
void ProcessTracer::OnSysCall()
{
  struct user_regs_struct regs;
  if (ptrace(PTRACE_GETREGS, pid, 0, &regs) == -1)
  {
     throw make_error("ptrace(PTRACE_GETREGS)");
  }

#if __x86_64__
  int const rc = regs.rax;
  int const func = regs.orig_rax;
  long int args[] = { regs.rdi, regs.rsi, regs.rdx, regs.r10, regs.r8, regs.r9 };
#elif __i386__
  int const rc = regs.eax;
  int const func = regs.orig_eax;
  long int args[] = { regs.ebx, regs.ecx, regs.edx, regs.esi, regs.edi, regs.ebp };
#else
#error Unknown target architecture
#endif // __x86_64__

  if (SelectedCall(func))
  {
    if (rc == -ENOSYS)
    {
      OnCallEntry(func, args);
    }
    else
    {
      OnCallExit(func, rc);
    }
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
bool ProcessTracer::SelectedCall(int func)
{
  switch (func)
  {
  case __NR_open:
  case __NR_close:
    return true;
  }
  return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
void ProcessTracer::OnCallEntry(int func, long int args[])
{
  if (func == __NR_open)
  {
    os << "open(\"" << readString(args[0]) << "\") = " << std::flush;
  }
  else if (func == __NR_close)
  {
    os << "close(" << args[0] << ") = " << std::flush;
  }
  else
  {
    os << '#' << func << "(" << args[0] << ") = " << std::flush;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
void ProcessTracer::OnCallExit(int func, int rc)
{
  if (rc < 0)
  {
    os << rc << "(" << strerror(-rc) << ")" << std::endl;
  }
  else
  {
    os << std::hex << rc << std::dec << std::endl;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
bool ProcessTracer::OnSignal(int signal)
{
  os << "Signal: " << sigstrm(signal) << std::endl;
  bool bDeliver(true);
  switch (signal)
  {
    case SIGCHLD:
    case SIGSTOP:
      bDeliver = false;
      break;
  }
  return bDeliver;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
std::string ProcessTracer::readString(long addr)
{
  std::string result;
#if 1

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
#else
  std::ostringstream os;
  os << "/proc/" << pid << "/mem";
  std::ifstream mem(os.str().c_str(), std::ios::binary);
  mem.exceptions(std::ios::failbit);
  mem.seekg((std::streampos)addr);
  std::getline(mem, result, '\0');
#endif // 0
  return result;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// Fork a child process, running under ptrace, and return its pid
pid_t CreateProcess(int argc, char **argv)
{
  pid_t const cpid = fork();
  if (cpid > 0)
  {
    // In the parent
    return cpid; 
  }
  else if (cpid == 0)
  {
    // In the child
    if (ptrace(PTRACE_TRACEME, 0, 0, 0) == -1)
    {
      throw make_error("ptrace(PTRACE_TRACEME)");
    }
    execv(argv[0], argv);
    throw make_error("execv");
  }
  else
  {
    throw make_error("fork");
  }
}

int main(int argc, char **argv)
{
  int rc(1);

  if (argc <= 1)
  {
    std::cout << "Syntax: ProcessTracer command_line" << std::endl;
    return 1;
  }
  ++argv;
  --argc;

  try
  {
    pid_t pid = CreateProcess(argc, argv);
    ProcessTracer(pid, std::cerr).run();
    rc = 0;
  }
  catch ( std::exception &ex)
  {
    std::cerr << "Unexpected exception: " << ex.what() << std::endl;
  }   

  return rc;
}
