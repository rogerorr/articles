#ifndef SIMPLE_STACK_WALKER_H
#define SIMPLE_STACK_WALKER_H

/**@file

  Wrapper for DbgHelper to help provide stack
  walking using MSVC

  @author Roger Orr <rogero@howzatt.co.uk>

  Copyright &copy; 2004, 2023.
  This software is distributed in the hope
  that it will be useful, but without WITHOUT
  ANY WARRANTY; without even the implied
  warranty of MERCHANTABILITY or FITNESS FOR A
  PARTICULAR PURPOSE.

  Permission is granted to anyone to make or
  distribute verbatim copies of this software
  provided that the copyright notice and this
  permission notice are preserved, and that
  the distributor grants the recipent
  permission for further distribution as
  permitted by this notice.

  Comments and suggestions are always welcome.
  Please report bugs to rogero@howzatt.co.uk.

  $Revision: 343 $
*/

// clang-format off
// $Id: SimpleStackWalker.h 343 2023-07-31 21:10:27Z roger $
// clang-format on

#include <iosfwd>
#include <string>

#include <windows.h>
#include <dbghelp.h>

/** Symbol Engine wrapper to assist with
 * processing PDB information */
class SimpleStackWalker {
public:
  /** Construct a StackWalker for the
   * specified target process */
  SimpleStackWalker(HANDLE hProcess);

  ~SimpleStackWalker();

  /** Provide a stack trace for the specified
   * thread in the target process */
  void stackTrace(HANDLE hThread, std::ostream &os);

  /** Provide a stack trace for the specified
   * thread using the supplied context in the
   * target process */
  void stackTrace(HANDLE hThread, PCONTEXT context, std::ostream &os);

  /** Wrppaer for SymGetTypeInfo */
  bool getTypeInfo(DWORD64 modBase, ULONG typeId,
                   IMAGEHLP_SYMBOL_TYPE_INFO getType, PVOID pInfo) const;

  /** Wrapper for ReadProcessMemory */
  bool readMemory(LPCVOID lpBaseAddress, // base of memory
                                         // area
                  LPVOID lpBuffer,       // data buffer
                  SIZE_T nSize) const;   // bytes to read

  /** Add type information to a name */
  void decorateName(std::string &name, DWORD64 modBase, DWORD typeIndex) const;

private:
  /* don't copy or assign */
  SimpleStackWalker(SimpleStackWalker const &) = delete;
  SimpleStackWalker &operator=(SimpleStackWalker const &) = delete;

  /** Convert an address to a string */
  std::string addressToString(DWORD64 address) const;

  /** Convert an inline address to a string */
  std::string inlineToString(DWORD64 address, DWORD inline_context) const;

  void showVariablesAt(std::ostream &os, const STACKFRAME64 &stackFrame,
                       const CONTEXT &context) const;

  void showInlineVariablesAt(std::ostream &os, const STACKFRAME64 &stackFrame,
                             const CONTEXT &context,
                             DWORD inline_context) const;

  HANDLE hProcess;
};

#endif // SIMPLE_STACK_WALKER_H
