/*
NAME
    TrivialPtrace

DESCRIPTION
    A very simple working example of ptrace

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

static char const szRCSID[] = "$Id: TrivialPtrace.cpp 256 2020-04-09 21:35:25Z Roger $";

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

/** Trivial ptrace user */
class TrivialPtrace
{
private:
  pid_t pid;
  std::ostream &os;

public:
  /** Create */
  TrivialPtrace(pid_t pid, std::ostream &os)
  : pid(pid), os(os) {}

  /** Run the debug loop */
  void run();
};

/////////////////////////////////////////////////////////////////////////////////////////////////
void TrivialPtrace::run()
{
  int status(0); // Status of the stopped child process

  // This is the main tracing loop. When the child stops,
  // we examine the system call and its arguments
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
    throw make_error("wait");
  }
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
    std::cout << "Syntax: TrivialPtrace command_line" << std::endl;
    return 1;
  }
  ++argv;
  --argc;

  try
  {
    pid_t pid = CreateProcess(argc, argv);
    TrivialPtrace(pid, std::cerr).run();
    rc = 0;
  }
  catch ( std::exception &ex)
  {
    std::cerr << "Unexpected exception: " << ex.what() << std::endl;
  }   

  return rc;
}
