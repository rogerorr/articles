/*
NAME
    MultiPtrace

DESCRIPTION
    A simple working example of ptrace for multiple threads and processes

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

static char const szRCSID[] = "$Id: MultiPtrace.cpp 256 2020-04-09 21:35:25Z Roger $";

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>
#include <sys/ptrace.h>

#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
  std::runtime_error make_error(std::string const &action)
  {
    std::string const what(action + " failed: " + strerror(errno));
    return std::runtime_error(what);
  }
} // namespace

/** Multi thread/process ptrace user */
class MultiPtrace
{
private:
  pid_t pid;
  std::ostream &os;
  bool initialised;

public:
  /** Create */
  MultiPtrace(pid_t pid, std::ostream &os)
  : pid(pid), os(os), initialised(false) {}

  /** Run the debug loop */
  void run();

private:
  /** Stop received */
  int OnStop(int signal, int event);
};

/////////////////////////////////////////////////////////////////////////////////////////////////
void MultiPtrace::run()
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
      os << "Terminated: signal " << WTERMSIG(status) << std::endl;
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
    throw make_error("waitpid");
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// Handle the various types of ptrace 'stop' events 
// 1) the first stop when starting debugging
// 2) extension events from fork/vfork/clone
// 3) signals
//
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
    // In the new child
    if (ptrace(PTRACE_TRACEME, 0, 0, 0) == -1)
    {
      throw make_error("ptrace(PTRACE_TRACEME)");
    }
    execv( argv[0], argv );
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
    std::cout << "Syntax: MultiPtrace command_line" << std::endl;
    return 1;
  }
  ++argv;
  --argc;

  try
  {
    pid_t pid = CreateProcess(argc, argv);
    MultiPtrace(pid, std::cerr).run();
    rc = 0;
  }
  catch ( std::exception &ex)
  {
    std::cerr << "Unexpected exception: " << ex.what() << std::endl;
  }   

  return rc;
}
