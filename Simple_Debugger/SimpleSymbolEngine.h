#ifndef SIMPLESYMBOLENGINE_H
#define SIMPLESYMBOLENGINE_H

/**@file

  Wrapper for DbgHelper to provide common utility functions for processing
  Microsoft PDB information.

  @author Roger Orr <rogero@howzatt.co.uk>

  Copyright &copy; 2004, 2011.
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

  $Revision: 256 $
*/

// $Id: SimpleSymbolEngine.h 256 2020-04-09 21:35:25Z Roger $

#include <iosfwd>
#include <string>

#include <windows.h>

/** Symbol Engine wrapper to assist with processing PDB information */
class SimpleSymbolEngine
{
public:
  /** Default ctor */
  SimpleSymbolEngine();

  /** Tidy up */
  ~SimpleSymbolEngine();

  /** Initialise for the specified process */
  void init(HANDLE hProcess);

  /** Convert an address to a string */
  std::string addressToString(PVOID address);

  /** Add a Dll or Exe */
  void loadModule(HANDLE hFile, PVOID baseAddress, std::string const & fileName);

  /** Remove a Dll or Exe */
  void unloadModule(PVOID baseAddress);

  /** Provide a stack trace for the specified thread */
  void stackTrace(HANDLE hThread, std::ostream & os);

  /** Read a string from the target */
  std::string getString(PVOID address, BOOL unicode, DWORD maxStringLength);
private:
  /* don't copy or assign */
  SimpleSymbolEngine(SimpleSymbolEngine const &);
  SimpleSymbolEngine& operator=(SimpleSymbolEngine const &);

  HANDLE hProcess;
};

#endif // SIMPLESYMBOLENGINE_H
