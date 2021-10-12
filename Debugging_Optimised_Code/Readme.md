# Showing Variables Using the Windows Debugging API
Roger Orr

## Introduction

In previous articles [ CVu2011, CVu2012 ] I demonstrated the basic principles of using the Windows Debugging API
 to manage a program being debugged and to produce a simple stack trace.

This article looks in more detail about what is needed to access **variables** in the program using the debugging
interface and additionally discusses some of the issues with optimised code that make debugging it a challenge.
While the techniques may be useful in their own right, they are described principally to try and help explain
what interactive debuggers, such as Visual Studio or WinDbg, are doing for us "under the hood" to achieve some
of their functionality.

While the article is written explicitly using the Windows Debug API targetting x64 programs many of the principles
apply to other environments even though the precise details will differ.

## Presenting the example code

The code in this article works through varying levels of detail in viewing the local variables in the following,
deliberately fairly simple, piece of code. For this example I am using an "invasive" explicit call to `stackWalk`
which I will gradually expand to obtain information about the local variables:
```
void process(Source &source) {
  int local_i = printf("This ");
  int local_j = printf("is ");
  int local_k = printf("a test\n");
  int local_l = source();

  printStack(); // << Here is our 'invasive' function call

  if (local_i != 5 || local_j != 3 ||
      local_k != 7) {
    std::cerr << "Something odd happened\n";
  }
}

int test() {
  Source source;
  int return_value = source();
  process(source);
  return return_value;
}

int main() { return test(); }
```
The `printStack` function simply creates a separate thread to perform the actual stack trace, and then joins this thread.
This technique allows the program to print its own stack trace; in the previous articles cited in the introduction I
used a separate debugging process to control the target process. Both techniques have their uses!

We would like to *programmatically* obtain the values of the local variables and the return value of the calling function
Most of us will have done this sort of thing before, but using an interactive debugger.

We will start out compiling the example code without optimisation, and then later on look at the issues that result
from turning on optimisation.

The first step in our quest is to walk the call stack. This basic code was described in the earlier articles, and
is also relatively well known, so I provide a quick summary of the principles and the sample stack walking code.

## Quick summary of Stack Tracing with the Win32 debugging API

The mechanism used by DbgHelp for Win32 stack tracing revolves around the function `StackWalk64`. The programmer sets
up the `stackFrame` and `context` data for the start point on the stack and then calls `StackWalk64` in a loop until
either it returns `false` or the frames of interest have all been processed.

The reason for there being *two* structures involved is that the `stackFrame` structure is portable and is
passed as a pointer to a `STACKFRAME64`, but the `context` structure contains environment-specific values
- this argument is passed by `void*` and it is up to the programmer to provide a pointer to the correct
structure for the environment being debugged. 

While the basic operation is the same for each platform supported by Win32, there are slight differences.
For the purposes of simplifying this article I am only supporting the x64 platform. In this scenario the
Windows headers set up the `CONTEXT` typedef to refer to the default, x64, context record and we must
pass  the `MachineType` of `IMAGE_FILE_MACHINE_AMD64` as the first argument to `StackWalk64`. Other
use cases, such as debugging an x86 process, would need to populate the appropriate actual context
structure name and set the corresponding value for `MachineType`.

The code for walking the stack starting from the 'current location' of the target thread looks like this:
```
void SimpleStackWalker::stackTrace(
    HANDLE hThread, std::ostream &os) {
  CONTEXT context = {0};
  STACKFRAME64 stackFrame = {0};

  context.ContextFlags = CONTEXT_FULL;
  GetThreadContext(hThread, &context);

  stackFrame.AddrPC.Offset = context.Rip;
  stackFrame.AddrPC.Mode = AddrModeFlat;

  stackFrame.AddrFrame.Offset = context.Rbp;
  stackFrame.AddrFrame.Mode = AddrModeFlat;

  stackFrame.AddrStack.Offset = context.Rsp;
  stackFrame.AddrStack.Mode = AddrModeFlat;

  os << "Frame               Code "
        "address\n";

  while (::StackWalk64(
      IMAGE_FILE_MACHINE_AMD64, hProcess,
      hThread, &stackFrame, &context, nullptr,
      ::SymFunctionTableAccess64,
      ::SymGetModuleBase64, nullptr)) {
    DWORD64 pc = stackFrame.AddrPC.Offset;
    DWORD64 frame =
        stackFrame.AddrFrame.Offset;
    if (pc == 0) {
      os << "Null address\n";
      break;
    }
    os << "0x" << (PVOID)frame << "  "
       << addressToString(pc) << "\n";
  }
}
```
The `addressToString` function is unchanged from the earlier articles cited above and, as its implementation
is not relevant to this article, will therefore not be explained further here.

## Printing the basic call stack

If we compile the example program with no optimising and with debug symbols ("/Zi") from the command line then,
with the `stackTrace` function shown above, `printStack` produces output like this:

### Listing 1 - Basic stack trace
(Note to make the output easier to read I've shortened long paths by replacing the middle of the path with "...")
```
Frame               Code address
0x0000007CF88FE0A0  0x00007FFD554CCEA4 NtWaitForSingleObject + 20
0x0000007CF88FE140  0x00007FFD530B19CE WaitForSingleObjectEx + 142
0x0000007CF88FE180  0x00007FFD38672E24 Thrd_join + 36
0x0000007CF88FE1E0  0x00007FF67DB85C2F std::thread::join + 95   C:\Program Files (x86)\...\include\thread(130) + 32 bytes
0x0000007CF88FE380  0x00007FF67DB81C60 printStack + 112   c:\article\TestStackWalker.cpp(65)
0x0000007CF88FE3C0  0x00007FF67DB81D2C process + 76   c:\article\TestStackWalker.cpp(76)
0x0000007CF88FF7A0  0x00007FF67DB81DB1 test + 65   c:\article\TestStackWalker.cpp(87)
0x0000007CF88FF7D0  0x00007FF67DB81DE9 main + 9   c:\article\TestStackWalker.cpp(90) + 9 bytes
0x0000007CF88FF820  0x00007FF67DB8AEA9 invoke_main + 57   d:\agent\_work\4\s\src\vctools\crt\vcstartup\src\startup\exe_common.inl(79)
0x0000007CF88FF890  0x00007FF67DB8AD4E __scrt_common_main_seh + 302   d:\agent\_work\4\s\src\vctools\crt\vcstartup\src\startup\exe_common.inl(288) + 5 bytes
0x0000007CF88FF8C0  0x00007FF67DB8AC0E __scrt_common_main + 14   d:\agent\_work\4\s\src\vctools\crt\vcstartup\src\startup\exe_common.inl(331)
0x0000007CF88FF8F0  0x00007FF67DB8AF3E mainCRTStartup + 14   d:\agent\_work\4\s\src\vctools\crt\vcstartup\src\startup\exe_main.cpp(17)
0x0000007CF88FF920  0x00007FFD53B07034 BaseThreadInitThunk + 20
0x0000007CF88FF9A0  0x00007FFD55482651 RtlUserThreadStart + 33
```
While printing a call stack like is extremely useful for debugging problems and getting clearer ideas of the flow of the program,
it is possible to enrich the information provided. 

## But how does it *work*?

In the x64 world stack walking uses the same logic that is used to support exception handling. The compiler generates some metadata
(tagged as "xdata" and "pdata") which is bound into the executable image and is available at runtime.

You can dump out this data with the `dumpbin` program supplied with Visual Studio, for example:

```
C:> dumpbin /unwindinfo TestStackWalker.exe
...
           Begin    End      Info      Function Name
...
  0000000C 00001EF0 00001F69 000188D4  ?process@@YAXAEAVSource@@@Z (void __cdecl process(class Source &))
    Unwind version: 1
    Unwind flags: None
    Size of prologue: 0x09
    Count of codes: 1
    Unwind codes:
      09: ALLOC_SMALL, size=0x3
...
```
The function `SymFunctionTableAccess64` is the one used by the stack walker to obtain the address of the function table
entry metadata for the various code addresses found during stack unwinding. This data provides, among other things,
information about the size of the current stack frame and the offset of the return address. The stack walking logic
uses this data on each iteration to work up the stack to the calling frame and to update the `stackFrame` and `context`
data to reflect this new frame.

This logic does *not* require any additional debug information that might be present in the PDB file, it simply uses
the read-only tables in the binary.

## What is in the PDB file?

If we delete the PDB file and re-run the program it is easy to see what information from the PDB file is used by the
stack trace code: the same **number** of stack frames is produced with the same **addresses** (subject to any
relocations performed by Address Storage Layout Randomisation) but the **names** for the functions inside the
executable and the **source file** information are no longer printed; these are being obtained from the PDB file.

The Microsoft PDB files contain a lot of data. For the example program I have a PDB file that is over 17 times larger
than the executable! All we have used so far is a small part of the overall data - to map **addresses** to **function names**
and **source file **information.

However, there is also a huge amount of detail available for the types and variables inside the program. There is
enough detail that we can, for instance, generate the full definition of the data members and class hierarchy for
the C++ classes used by the program or, as we do next, to introspect on the variables within the program. Note
that the full type information is not *always* available - for example Microsoft's public symbol files for the
Windows binaries normally only expose function names.

The PDB file format is not, to the best of my knowledge, publicly documented but there are various public APIs
to read the data. However, I have found that the documentation is often quite thin on detail and this can make
it quite slow to successfully make use of the API in your own programs. See for example the [dbghelp.h] documentation.

## Getting the names of local variables at each point in the call stack

The first step we take towards our goal is to use the DbgHelp function `SymEnumSymbols` to enumerate the local
variables at each point in the call stack and then simply printing the names of these variables to demonstrate
the enumeration works.

We add this functionality by writing a new function, `showVariablesAt`, which is called on each iteration of
the main loop in the stackTrace function. This function first calls `SymSetContext` (which requires populating
a slightly *different* stack frame structure: `IMAGEHLP_STACK_FRAME`) to ensure the subsequent call to
SymEnumSymbols will search at the location of the call site. For each variable found a callback function
we provide is called by the symbol engine, passing us a pointer to the symbol information and a user-supplied value.
(Note that the callback function is also passed a `SymbolSize`, which we ignore here because the information
is also available in the `Size` field of the `SYMBOL_INFO` structure.)

The SymEnumSymbols function operates in a variety of modes -- you can for instance use it to enumerate *all*
symbols within a binary file matching a specified filter string. The callback function invoked for each symbol
found allows the option of terminating early if the item sought has been found. In our use case we want to
enumerate all the local symbols in scope at a given call site, so we provide `"*"` as the filter and always
return `TRUE` from our callback function to ensure we continue to enumerate.

This user-supplied value can be used to pass arbitrary data to the callback; here we use the common technique
when calling such a C API from C++ and pass a pointer to an instance of a user defined structure as the user
defined value, dereference this in the callback function, and finally call its `operator()`:
```
struct EnumLocalCallBack {
  // Called by the Symbol Engine
  static BOOL CALLBACK
  enumSymbolsProc(PSYMBOL_INFO pSymInfo,
                  ULONG /*SymbolSize*/,
                  PVOID UserContext) {
    auto &self =
        *(EnumLocalCallBack *)UserContext;
    self(*pSymInfo);
    return TRUE;
  }

  EnumLocalCallBack(
      const SimpleStackWalker &eng,
      std::ostream &os,
      const STACKFRAME64 &stackFrame,
      const CONTEXT &context)
      : eng(eng), os(os),
        frameOffset(frameOffset),
        context(context) {}

  void operator()(
      const SYMBOL_INFO &symInfo) const;

private:
  const SimpleStackWalker &eng;
  std::ostream &os;
  const STACKFRAME64& stackFrame;
  const CONTEXT &context;
};
```
The `SYMBOL_INFO` structure that we are passed in the callback contains a number of fields obtained from
the PDB information for the module being examined. We will need several of these fields in order to successfully
decode the values of the variables we are interested in. The first pair we are interested in are `Name` and
`NameLen`, as these provide us with the name of each variable found.

However, we should also consider the `Flags` field. We are only interested in symbols where `SYMFLAG_LOCAL`
is set and we should also exclude any symbols marked with `SYMFLAG_NULL`. We will be using *other* flags
in the `Flags` field in due course.

We will enrich the contents of the `EnumLocalCallBack` function call operator as we proceed, but the first
implementation is quite simple:
```
// Simplest useful implementation
void EnumLocalCallBack::
operator()(const SYMBOL_INFO &symInfo) const {
  if (!(symInfo.Flags & SYMFLAG_LOCAL)) {
    // Ignore anything not a local variable
    return;
  }
  if (symInfo.Flags & SYMFLAG_NULL) {
    // Ignore 'NULL' objects
    return;
  }
  std::string name(symInfo.Name,
                   symInfo.NameLen);
  os << "  " << name << '\n';
}
```
The `showVariablesAt` function populates the structures and invokes `SymEnumSymbols`:
```
void SimpleStackWalker::showVariablesAt(
    std::ostream &os,
    const STACKFRAME64 &stackFrame,
    const CONTEXT &context) const {

  EnumLocalCallBack callback(
      *this, os, stackFrame, context);

  IMAGEHLP_STACK_FRAME imghlp_frame = {0};
  imghlp_frame.InstructionOffset =
      stackFrame.AddrPC.Offset;

  SymSetContext(hProcess, &imghlp_frame,
               nullptr);

  SymEnumSymbols(
      hProcess, 0, "*",
      EnumLocalCallBack::enumSymbolsProc,
      &callback);
}
```
Finally we must add a call to this function to the end of the main loop in stackTrace:

` showVariablesAt(os, stackFame, context);`

With all this in place we now get a list of the local variables at each function in the stack trace
(or at least, those for which the debug information is available)

### Listing 2 - Stack trace including names of local variables
```
0x000000BBDEAFE0E0  0x00007FFD554CCEA4 NtWaitForSingleObject + 20
0x000000BBDEAFE180  0x00007FFD530B19CE WaitForSingleObjectEx + 142
0x000000BBDEAFE1C0  0x00007FFD38672E24 Thrd_join + 36
0x000000BBDEAFE220  0x00007FF7CF505C2F std::thread::join + 95   C:\Program Files (x86)\...\include\thread(130) + 32 bytes
  this
0x000000BBDEAFE3C0  0x00007FF7CF501C60 printStack + 112   c:\article\TestStackWalker.cpp(65)
  ss
  hThread
  thr
0x000000BBDEAFE400  0x00007FF7CF501D2C process + 76   c:\article\TestStackWalker.cpp(76)
  source
  local_k
  local_i
  local_l
  local_j
0x000000BBDEAFF7E0  0x00007FF7CF501DB1 test + 65   c:\projects\articles\2021-09-
...
```
Note that the **order** in which the local variables are enumerated does not match declaration order in the source code.

## How to identify the types of these variables

The PDB information for each symbol also includes type information. This information is held by index
(since multiple variables can have the same type) and the index to this information is provided in the
`TypeIndex` field of the `SYMBOL_INFO` structure.

We add a new function to the `SimpleStackWalker` to encapsulate adding the type to the variable name:
```
void decorateName(std::string &name,
                  DWORD64 ModBase,
                  DWORD TypeIndex) const;
```
We call this with the `name` to add type information before printing it - note that the function needs to *modify* the
name because the declaration rules in C and C++ may result in embedding the variable name in the complete declaration
(for example "`void (*func)()`").

The `decorateName` function makes use of another function in the symbol library, `SymGetTypeInfo`. This function
provides access to various attributes of the type, selected by the `IMAGEHLP_SYMBOL_TYPE_INFO` enumeration type
passed as the fourth argument. The actual data is returned in the final argument, where the format of the data
depends on the value of the `GetType` parameter.

We provide a member function, `GetTypeInfo()`, that adds `hProcess` as the first argument to avoid having to specify this everywhere.

There are many different classes of symbol information, all accessed using this method and the type index. We pass
`TI_GET_SYMTAG` to `SymGetTypeInfo` to provide the *tag* type of the corresponding symbol information. These tag
values are defined in the enumeration `SymTagEnum` in `cvconst.h` (found in the "`DIA SDK\include`"
subdirectory of Visual Studio, which is not by default in the include path) or alternatively from `DbgHelp.h`,
if you define the symbol `_NO_CVCONST_H`.

Since each tag holds different information the tag is used as the condition for a `switch` statement. For the
purposes of this article only four types are of interest and I describe each in turn and show the code for that
`case` statement.

   * 1. Built-in data types

The value `SymTagBaseType` is used for "built-in" data types, such as `int` and `double`. The `TI_GET_BASETYPE`
and `TI_GET_LENGTH` arguments to `SymGetTypeInfo` provide the underlying type (taken from the `BasicType` enumeration, for example `btUInt`) and the data length (for example, 4).

The code uses a helper function, `std::string getBaseType(DWORD baseType, ULONG64 length)`, to convert the data
to C++ data types such as "`unsigned short`".
The `getBaseType` function uses a data structure holding type, length, and corresponding C++ type name, for example:
```
  ...
  {btUInt, sizeof(unsigned short),
   "unsigned short"},
  {btUInt, sizeof(unsigned int),
   "unsigned int"}
  ...
```
In action `getBaseType` just returns the name found in the matching element of this structure.
The complete `case` statement is then this:
```
  case SymTagBaseType: {
    DWORD baseType{};
    ULONG64 length{};
    getTypeInfo(modBase, typeIndex,
                TI_GET_BASETYPE, &baseType);
    getTypeInfo(modBase, typeIndex,
                TI_GET_LENGTH, &length);
    name.insert(0, " ");
    name.insert(
        0, getBaseType(baseType, length));
    break;
  }
```
   * 2. User Defined Types

The value `SymTagUDT` is used for user defined types, such as `Source` in our example code. The first call
we make in the function uses `TI_GET_SYMNAME` value which retrieves the full type name as a wide character string.
```
  case SymTagUDT: {
    WCHAR *typeName{};
    if (getTypeInfo(modBase, typeIndex,
                    TI_GET_SYMNAME,
                    &typeName)) {
      name.insert(0, " ");
      name.insert(0, strFromWchar(typeName));
      // We must free typeName
      LocalFree(typeName);
    }
    break;
  }
```
(Where `strFromWchar` simply creates an `std::string` from a `WCHAR*`)

   * 3. Pointers and arrays

Pointers and arrays are identified by the `SymTagPointerType` and `SymTagArrayType`, respectively.
In both cases the dependent type is obtained using `TI_GET_TYPEID` and we recursively call `decorateName` on this type index.
```
  case SymTagPointerType: {
    name.insert(0, "*");
    recurse = true;
    break;
  }
  case SymTagArrayType: {
    if (name[0] == '*') {
      name.insert(0, "(");
      name += ")";
    }
    DWORD Count{};
    getTypeInfo(modBase, typeIndex,
                TI_GET_COUNT, &Count);
    name += "[";
    if (Count) {
      name += std::to_string(Count);
    }
    name += "]";
    recurse = true;
    break;
  }
```
The recurse logic is common and is at the end of the `decorateName` function:
```
  if (recurse) {
    DWORD ti{};
    if (getTypeInfo(modBase, typeIndex,
                    TI_GET_TYPEID, &ti)) {
      decorateName(name, modBase, ti);
    }
  }
```
### Listing 3. Stack trace with names and type of local variables
```
Frame               Code address
0x000000CD0F6FE110  0x00007FFD554CCEA4 NtWaitForSingleObject + 20
0x000000CD0F6FE1B0  0x00007FFD530B19CE WaitForSingleObjectEx + 142
0x000000CD0F6FE1F0  0x00007FFD36972E24 Thrd_join + 36
0x000000CD0F6FE250  0x00007FF61B674F4F std::thread::join + 95   C:\Program Files (x86)\...\include\thread(130) + 32 bytes
  std::thread *this
0x000000CD0F6FE3F0  0x00007FF61B671E40 printStack + 112   c:\article\TestStackWalker.cpp(64)
  std::basic_stringstream<char,std::char_traits<char>,std::allocator<char> > ss
  void *hThread
  std::thread thr
0x000000CD0F6FE430  0x00007FF61B671F0C process + 76   c:\article\TestStackWalker.cpp(75)
  Source *source
  int local_k
  int local_i
  int local_l
  int local_j
0x000000CD0F6FF810  0x00007FF61B671F81 test + 49   c:\article\TestStackWalker.cpp(85)
...
```
Note in listing 3 that the type of '`source`' in the `process` function is shown as "`Source *`"
although the argument is actually passed by reference. In the PDB file the distinction in the C++
code between pointers and references is lost.

## Showing the actual *values* for local variables

We now know the name and the type of our local variables, what about their **value**? In an
unoptimised program local variables are held in the stack frame; if we look at the assembly
output from compiling the program we can see this (produced when we add `/Fasc` to the command line):
```
local_i$ = 32
...
?process@@YAXAEAVSource@@@Z PROC ; process
... 
  00015 89 44 24 20 mov   DWORD PTR local_i$[rsp], eax
```
The compiler output uses a symbolic name for the variable and uses this value as an offset from the stack pointer (rsp).

In the symbol engine this is indicated in the `SYMBOL_INFO` by a flag value of `SYMFLAG_REGREL`. The
base register is provided in the `Register` field and the offset (32 for `local_i`, in this example)
is supplied in the `Address` field.

There is a large enumeration in `cvconst.h` listing all the various register values - the one we want here for
`local_i` is `CV_AMD64_RSP` (which is 335).

We can encapsulate the access to the register value by creating a `struct` and a helper function:
```
struct RegInfo {
  RegInfo(std::string name, DWORD64 value)
      : name(std::move(name)), value(value) {}
  std::string name;
  DWORD64 value;
};

RegInfo getRegInfo(ULONG reg,
                   const CONTEXT &context);
```
This function returns the correct name and value for the supplied '`reg`', at this point the "Minimal viable product" is:
```
RegInfo getRegInfo(ULONG reg,
                   const CONTEXT &context) {
  switch (reg) {
  case  CV_AMD64_RSP:
    return RegInfo("rsp", context.Rsp);
  }
  return RegInfo("", 0);
}
```
We will come back to this function before we have finished....

So the steps we need to obtain the value of the variable are:

   * detect it is a register relative value
   * add the offset to the corresponding register value
   * read 'Size' bytes from the resulting address 

In code this looks like:
```
  if (symInfo.Flags & SYMFLAG_REGREL) {
    const RegInfo reg_info =
        getRegInfo(symInfo.Register, context);
    if (reg_info.name.empty()) {
      opf << " [register '"
          << symInfo.Register << "']";
    } else {
      opf << std::hex
          << " [" << reg_info.name << " + "
          << symInfo.Address << "]";
      if (symInfo.Size != 0 &&
          symInfo.Size <= 8) {
        DWORD64 data{};
        eng.readMemory(
            (PVOID)(reg_info.value +
                    symInfo.Address),
            &data, symInfo.Size);
        opf << " = 0x" << data;
      }
      opf << std::dec;
    }
  }
```
(where `eng.readMemory` is a simple wrapper for `ReadProcessMemory` that adds the current `hProcess`)

### Listing 4. Stack trace with names, types, and values of local variables
```
Frame               Code address
0x00000097900FE570  0x00007FFD554CCEA4 NtWaitForSingleObject + 20
0x00000097900FE610  0x00007FFD530B19CE WaitForSingleObjectEx + 142
0x00000097900FE650  0x00007FFD2E512E24 Thrd_join + 36
0x00000097900FE6B0  0x00007FF7196A62CF std::thread::join + 95   C:\Program Files (x86)\...\include\thread(130) + 32 bytes
  std::thread *this [rsp+60] = 0x97900fe6f8
0x00000097900FE850  0x00007FF7196A1E70 printStack + 112   c:\article\TestStackWalker.cpp(64)
  std::basic_stringstream<char,std::char_traits<char>,std::allocator<char> > ss [rsp+60]
  void *hThread [rsp+20] = 0xc4
  std::thread thr [rsp+38]
0x00000097900FE890  0x00007FF7196A1F3C process + 76   c:\article\TestStackWalker.cpp(75)
  Source *source [rsp+40] = 0x97900fe8d0
  int local_k [rsp+28] = 0x7
  int local_i [rsp+20] = 0x5
  int local_l [rsp+2c] = 0xfda93c3e
  int local_j [rsp+24] = 0x3
0x00000097900FFC70  0x00007FF7196A1FB1 test + 65   c:\article\TestStackWalker.cpp(85)
  int return_value [rsp+20] = 0x799c244e
  Source source [rsp+30]
...
```
Hurrah! We have successfully walked the stack and printed the values of the (simple) local variables we find.
We could, if we wished, expand the code further to print out the contents of C++ classes by reflecting on the
fields and their offsets.

However, the code so far has been demonstrated against an *unoptimised* program.

## What happens when we start to optimise the code?

As many readers are likely to be already aware, it is usually harder to debug optimised code because of the
changes made to the executable code during optimisation. Here are a few of the troublesome optimisations:

   * code movement, so things no longer occur in the order of the source code syntax
   * heavy use of registers rather than storing values on the stack
   * elimination of "dead stores" (values stored but not subsequently loaded)
   * inline function calls

We can some see these at work in the example program if we enable `/O1` - the local variables displayed
in the stack trace for the `process` function are shown as:
```
  Source *source
  int local_k
  int local_i
  int local_j
```
The compiler has eliminated `local_l` which you might have noticed was written to but not read. (The *compiler*
noticed too - a debug build gives a warning:
`C4189: 'local_l': local variable is initialized but not referenced`

The optimised build elides storing the return value into `local_l`, and doesn't even write any information for
the variable into the pdb.

Secondly, the *values* are no longer shown. This is because the optimiser is now using **registers** to store
the values - they do not need to be stored on the stack in the function.

If we examine the assembly output  from the compiler we can see this:
```
    lea    rcx, OFFSET FLAT:??_C@_05PFHNGCBD@This?5@
    call   printf

; 69   :   int local_j = printf("is ");

    lea    rcx, OFFSET FLAT:??_C@_03FLKGGKMB@is?5@
    mov    ebx, eax\b0
```
The complier is using the 32bit register `ebx` to hold the value of `local_i`.

On the x86 instruction set a general purpose register can be treated as a 64-bit, a 32-bit,  a 16-bit,
or an 8-bit value. Writing to the 32-bit value, for instance, modifies only the lower 32 bits of the
full 64-bit register value. Hence, the `context` at this point will have the `ebx` value as the low
32 bits of the value in `context.Rbx`.

To see this information in the symbol engine we check another flag in the `Flags` field: `SYMFLAG_REGISTER`.
This flag indicates that the value of the variable is held in a register (and the field `Register` holds the
register used - in this case `CV_AMD64_EBX`)

The first thing we need to do is to implement a fuller version of the `getRegInfo()` function we used to decode
the values of the stack based variables in the *unoptimised* case.

There are two things we need to do to this function; the first one is to add the other general purpose registers
to the `switch` statement and the other thing we need to do is to mask the values for the registers which are
using only *part* of the 64bit general purpose register.

So, when processing `local_i` the line in the switch statement that will be executed is:
```
 case CV_AMD64_EBX:
    return RegInfo("ebx", context.Rbx & ~0u);
```
 We then add handling for the `SYMFLAG_REGISTER` flag to the function call operator of `EnumLocalCallBack`,
 just after the existing code for the `SYMFLAG_REGREL` flag, like this:
```
  } else if (symInfo.Flags &
             SYMFLAG_REGISTER) {
    opf << "  " << name;
    const RegInfo reg_info =
        getRegInfo(symInfo.Register, context);
    if (reg_info.name.empty()) {
      opf << " (register '"
          << symInfo.Register << "\')";
    } else {
      opf << " (" << reg_info.name << ") = 0x"
          << std::hex << reg_info.value
          << std::dec;
    }
  }
```
With these changes we now get values printed for the local variables in the optimised build too:

### Listing 5. Stack trace with names, types, and values of local variables in optimised build.
```
Frame               Code address
0x000000A0EA72E390  0x00007FFD554CCEA4 NtWaitForSingleObject + 20
0x000000A0EA72E430  0x00007FFD530B19CE WaitForSingleObjectEx + 142
0x000000A0EA72E460  0x00007FFD4B2E2DFF Thrd_join + 31
0x000000A0EA72E5F0  0x00007FF72B7A3Df2 printStack + 2f2   c:\article\TestStackWalker.cpp(62) + 47 bytes
  std::basic_stringstream<char,std::char_traits<char>,std::allocator<char> > ss [rsp + 50]
  void *hThread [rsp + 40]
  std::thread thr [rsp + 30]
0x000000A0EA72E620  0x00007FF72B7A3F4F process + 79   c:\article\TestStackWalker.cpp(75)
  Source *source (rdi) = 0xa0ea72e650
  int local_k (esi) = 0x7
  int local_i (ebx) = 0x5
  int local_j (ebp) = 0x3
0x000000A0EA72F9F0  0x00007FF72B7A440F test + 115   c:\article\TestStackWalker.cpp(85)
  int return_value (ebx) = 0x673ae2c3
  Source source [rsp + 20]
...
```
**But ... what ? Where does the *right* register value come from?**

The code changes we had to make to display local variables in an optimised build were quite small. The "magic"
is that we were provided with the *correct* value of `Rbx` in the context record. When we read the initial thread
context in the `stackTrace` method:

`GetThreadContext(hThread, &context);`

then I see the value of `Rbx` as **0**. Where, you might wonder, does the runtime find the value of **5**
which `Rbx` had further up the call stack?

The "Register usage" documented in [x64abi] states that the `Rbx` register value must be preserved when a
function is called, and restored on return. The register set is basically divided into ones that must be restored
- also called 'non-volatile' and ones that are 'scratch' - also called 'volatile'.

So, if the called function wants to use the register, it must save it somewhere. There is no point using
another register to save it as the called function could simply use the other register itself so the value
is saved on the stack, and restored when the function returns. This of course applies to the process function
as well, so the function prologue contains the instruction:

`    mov    QWORD PTR [rsp+8], rbx`

and the function epilogue reverses this:

`    mov   rbx, QWORD PTR [rsp+48]`

(The stack offsets are *different* because the function manipulates the stack pointer during its prologue and
epilogue, and the ordering in the epilogue is not the reverse of that in the prologue)

This works well during normal flow, but what if an exception is thrown somewhere? The runtime needs to ensure
the register convention is maintained otherwise the code catching the exception might find a local variable, held
in a register, had suddenly changed value!

However it would be expensive (and hard!) for the runtime to try and do this by disassembling the code in each
function as it unwinds in order to work out which registers were saved and what frame offset should be used to
restore them.

This information is also saved in the unwind meta-data I touched on briefly in the discussion of stack tracing
("But how does it *work*?")

We can examine this data for the `process` function as before using dumpbin, but this time on the optimised program:
```
C:> dumpbin /unwindinfo TestStackWalker.exe
...
           Begin    End      Info      Function Name
...
  00000264 00003F00 00003F86 0001095C  ?process@@YAXAEAVSource@@@Z (void __cdecl process(class Source &))
    Unwind version: 1
    Unwind flags: None
    Size of prologue: 0x14
    Count of codes: 8
    Unwind codes:
      14: SAVE_NONVOL, register=rsi offset=0x40
      14: SAVE_NONVOL, register=rbp offset=0x38
      14: SAVE_NONVOL, register=rbx offset=0x30
      14: ALLOC_SMALL, size=0x20
      10: PUSH_NONVOL, register=rdi
```
The table contains the information for each non-volatile register and how it should be restored. At runtime
the unwind logic uses this information as it works up the stack to restore the register values at each level.
The symbol engine code does *exactly* the same thing when you produce a stack trace - it reads the unwind metadata
to update the `context` with the register values that were currently at each call site.

## More extreme optimisation

It's not possible to undo the effect on debugging of all optimisations. As we saw, even in our simple example,
`local_l` is not saved. The return value from `source()` is returned in the `Eax` register, but this is a *volatile*
register and here the fourth instruction in `printStack` overwrites the Rax register and this value is lost forever.

A particular problem is debugging inlined functions. Inlining not only removes the function prologue and epilogue, but
it also allows the compiler to further optimise the instructions in the called function as part of the body of the caller.
This results in code where the assembly instructions executed may toggle back and forth between logically different functions.

Windows debuggers are able to make use of additional data in the PDB file which identifies where the various parts of
the inlined functions end up in the binary. This allows the debugger in Visual Studio to make a reasonable stab at
debugging even quite heavily optimised code.

I'm not going to attempt to do this here.

## How can we debug and get performance?

As a developer there is a tension between performance and debuggability.

The ideal case is where the program behaves in a sufficiently similar way with and without optimisation,
so you can run an interactive debugger against an unoptimised build and get the same behaviour as with
the released product.

This is the well known pattern of having a separate "Release" and "Debug" builds.
If this works in your case it is likely to be the easiest way to resolve problems. Of course, this pattern
only works if you are able to reproduce a problem originally occurring with a Release build when using a Debug one.

More nuanced control is possible, however, with care. 

The naive approach of mixing together object files from a Debug and Release build unfortunately very rarely works.
This is because many compiler flags differ between the two projects, which means things like structure sizes and
layouts may not match.

However, you can change *just* the optimisation setting for individual files in the Release build  and for even
finer control you can change the optimisation setting for individual functions. 

This can be very useful when you know roughly which functions are involved in a failure case but cannot, for
whatever reason, use the full Debug build.

Let us try this out in our simple example. If we compile with full optimisation (for example with the compiler
option `/Ox`) our stack trace is now not very useful:

### Listing 6. Output from a fully optimised program.
```
This is a test
Frame               Code address
0x000000A72BAFE420  0x00007FFB259ACEA4 NtWaitForSingleObject + 20
0x000000A72BAFE4C0  0x00007FFB230619CE WaitForSingleObjectEx + 142
0x000000A72BAFE4F0  0x00007FFB20272DFF Thrd_join + 31
0x000000A72BAFE6B0  0x00007FF7602F1E6A printStack + 346   c:\article\TestStackWalker.cpp(62) + 49 bytes
  std::basic_stringstream<char,std::char_traits<char>,std::allocator<char> > ss [rsp+80]
  void *hThread [rsp+70]
  std::thread thr [rsp+40]
0x000000A72BAFFA80  0x00007FF7602F2302 main + 178   c:\article\TestStackWalker.cpp(88) + 178 bytes
0x000000A72BAFFAC0  0x00007FF7602FA830 __scrt_common_main_seh + 268   d:\a01\...\startup\exe_common.inl(288) + 34 bytes
  bool has_cctor [rsp+20]
0x000000A72BAFFAF0  0x00007FFB24A67034 BaseThreadInitThunk + 20
0x000000A72BAFFB70  0x00007FFB25962651 RtlUserThreadStart + 33
```
Here we see that the call stack in our program has collapsed - `main` calls `printStack` directly and the intervening
calls to both `test` and `process` have been inlined.

If we wrap the `process` function in a `#pragma optimize("", off)` / `#pragma optimize("", on)` pair then this function
will not be optimised and therefore easy to debug, without affecting the optimisation elsewhere in the program.

### Listing 7. Output from a fully optimised program with an *unoptimised* function.
```
This is a test
Frame               Code address
0x0000006E236FE4F0  0x00007FFB259ACEA4 NtWaitForSingleObject + 20
0x0000006E236FE590  0x00007FFB230619CE WaitForSingleObjectEx + 142
0x0000006E236FE5C0  0x00007FFB20272DFF Thrd_join + 31
0x0000006E236FE780  0x00007FF7FA041E6A printStack + 346   c:\article\TestStackWalker.cpp(62) + 49 bytes
  std::basic_stringstream<char,std::char_traits<char>,std::allocator<char> > ss [rsp+80]
  void *hThread [rsp+70]
  std::thread thr [rsp+40]
0x0000006E236FE7C0  0x00007FF7FA0420FC process + 76   c:\article\TestStackWalker.cpp(75)
  Source *source [rsp+40] = 0x6e236fe7f0
  int local_k [rsp+28] = 0x7
  int local_i [rsp+20] = 0x5
  int local_l [rsp+2c] = 0xd9e328d8
  int local_j [rsp+24] = 0x3
0x0000006E236FFB90  0x00007FF7FA04224D main + 125   c:\article\TestStackWalker.cpp(89) + 125 bytes
0x0000006E236FFBD0  0x00007FF7FA04A830 __scrt_common_main_seh + 268   d:\a01\...\startup\exe_common.inl(288) + 34 bytes
  bool has_cctor [rsp+20]
0x0000006E236FFC00  0x00007FFB24A67034 BaseThreadInitThunk + 20
0x0000006E236FFC80  0x00007FFB25962651 RtlUserThreadStart + 33
```
## Conclusion

In this article I have sketched some of the techniques used by an interactive debugger to provide values for local variables.
I've also showed some of the ways in which more work is needed to do this when optimisations are applied.

The implementers of the various Windows debuggers have done a great job at providing a powerful environment which works
amazingly well even on optimised programs.

However, there are times when if you wish to obtain debugging information at runtime you may need to compromise on the
performance, at least for the parts of the program under investigation which you are focussing on.

## References

[CVu2011] Using the Windows Debugging API
C Vu, 23(1) https://accu.org/journals/cvu/23/1/cvu23-1.pdf

[CVu2011] Using the Windows Debugging API on Windows 64
C Vu, 23(6) https://accu.org/journals/cvu/23/6/cvu23-6.pdf

[dbghelp.h] dbghelp.h header documentation
https://docs.microsoft.com/en-us/windows/win32/api/dbghelp/

[x64abi] Microsoft x64 Software Conventions
https://docs.microsoft.com/en-us/cpp/build/x64-software-conventions

## Source code

The full source code for this article can be found at:
https://github.com/rogerorr/articles/tree/main/Debugging_Optimised_Code

<hr>
Copyright (c) Roger Orr 2021-10-12 12:39:41Z (First published in Overload 165, Oct 2021)
