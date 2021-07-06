# Windows 64-bit calling conventions

## Introduction

There are many layers of technology in computing: we even use the term "technology stack" when trying to name the set of components used
in the development of a given application. While it may not be necessary to understand all the layers to make use of them a little
comprehension of what's going on can improve our overall grasp of the environment and, sometimes help us to work with,
rather than against, the underlying technology.

Besides, it's interesting!

Many of us write programs that run on the Windows operating system and increasingly many of these programs are running as 64-bit applications.
While Microsoft have done a fairly good job at hiding the differences between the 32-bit and 64-bit windows environments there are 
differences and some of the things we may have learnt in the 32-bit world are no longer true, or at least have changed subtly.

In this article I will cover how the calling convention has changed for 64-bit Windows. Note that while this is very *similar* to
the 64-bit calling conventions used in other environments, notably Linux, on the same 64-bit hardware I'm not going to specifically
address other environments (other than in passing.) I am also only targeting the "x86-64" architecture (also known as "AMD 64")
and I'm not going to refer to the Intel "IA-64" architecture. I think Intel have lost that battle.

I will be looking at a small number of assembler instructions; but you shouldn't need to understand much assembler to make sense of
the principles of what the code is doing. I am also using the 32-bit calling conventions as something to contrast the 64-bit ones with;
but again, I am not assuming that you are already familiar with these.

## A simple model of stack frames

Let's start by looking in principle at the stack usage of a typical program.

The basic principle of a stack frame is that each function call operates against a 'frame' of data held on the stack that includes all the directly
visible function arguments and local variables.
When a second function is called from the first, the call sets up a new frame further into the stack
(confusingly the new frame is sometimes described as "further up" the stack and sometimes as "further down").

This design allows for good encapsulation: each function deals with a well-defined set of variables and, in general, you do not 
need to concern yourself with variables outside the current stack frame in order to fully understand the behaviour and semantics of 
a specific function. Global variables, pointers and references make things a little more complicated in practice.

The "base" address of the frame is not necessarily the lowest address in the frame, and so some items in the frame may have a higher
address than the frame pointer (+ve offset) and some may have a lower address (-ve offset).

In the 32-bit world the `esp` register (the sp stands for "stack pointer") holds the current value of the stack pointer and
the `ebp` register (the bp stands for "base pointer") is used **by default** as the pointer to the
base address of the stack frame (the use of 'frame pointer optimisation' (FPO) - also called 'frame pointer omission' - can repurpose this register.)
Later on we'll see what happens in 64-bits.

Let's consider a simple example function `foo`:

```
void foo(int i, char ch)
{
   double k;
   // ...
}
```

Here is how the stack frame might look when compiled as part of a 32-bit program:


<table>
<tr> <td class="alt"/> <th>Offset</th> <th>Size</th> <th>Contents</th></tr>
<tr> <td class="alt">*High mem*</td> <td>+16</td><td>&nbsp;</td> <td>*Top of frame*</td></tr>
<tr style="color:#606060"> <td class="alt"/> <td>+13</td><td>3 bytes</td> <td>*padding*</td></tr>
<tr> <td class="alt"/> <td>+12</td> <td>1 byte</td> <td>`char ch` *(arguments pushed R to L)*</td></tr>
<tr> <td class="alt"/> <td>+8</td> <td>4 bytes</td> <td>`int i`</td></tr>
<tr> <td class="alt"/> <td>+4</td> <td>4 bytes</td> <td>*return address*`</td></tr>
<tr> <td class="alt" style="text-align:right">`ebp`-&gt;</td> <td>+0</td><td>4 bytes</td> <td>*previous frame base*</td></tr>
<tr> <td class="alt" style="text-align:right">`esp`-&gt;*</td> <td>-8</td> <td>8 bytes</td> <td>`double k `*(and other local variables)*</td></tr>
<tr> <td class="alt">*Low mem*</td> <td>&nbsp;</td><td>&nbsp;</td> <td>&nbsp;</td></tr>
</table>
(* `esp` starts here but takes values lower in memory as the function executes.)

Note: the assembler listing output that can be obtained from the MSVC compiler (`/FAsc`) handily displays many of these offsets, for example:
```
_k$ = -8   ; size = 8
_i$ = 8    ; size = 4
_ch$ = 12  ; size = 1
```

This is conceptually quite simple and, at least without optimisation, the actual implementation of the 
program in terms of the underlying machine code and use of memory may well match this model.
This is the model that many programmers are used to
and they may even implicitly translate the source code into something like the above memory layout.

The total size of the stack frame is 24 bytes: there are 21 bytes in use (the contiguous range from -8 to +13) but the frame top is
rounded up to the next 4 byte boundary.

You can demonstrate the stack frame size in several ways; one way is by calling a function that takes the address of a local variable
before and during calling `foo`
(although note that this simple-minded technique may not work as-is when aggressive optimisation is enabled).
For example:
```
void check()
{
  static char *prev = 0;
  char ch(0);
  if (prev)
  {
    printf("Delta: %i\n", (prev - &ch));
  }
  prev = &ch;
}
```

In the caller:
```
  check();
  foo(12, 'c');
```

In the function:
```
  void foo(int i, char ch)
  {
    check();
    double k;
    // ...
```


The 32-bit stack frame for `foo` may be built up like this when it is called:


<table>
<tr> <th style="text-align:left" colspan="2">In the caller</th> </tr>
<tr> <td>`push 99 `</td> <td>set `ch` in what will become `foo`'s frame to 'c'</td></tr>
<tr> <td>`push 12 `</td> <td>set up `i` in `foo`'s frame</td></tr>
<tr> <td>`call foo`</td> <td>enter `foo`, return address now in place</td></tr>

<tr> <th style="text-align:left" colspan="2">In `foo`</th> </tr>
<tr> <td style="text-align:left" colspan="2">*Function prolog*</td></tr>
<tr> <td>`push ebp`</td> <td>save the previous frame register </td></tr>
<tr> <td>`mov ebp, esp`</td> <td>set register `ebp` to point to the frame base</td></tr>
<tr> <td>`sub esp, 8`</td> <td> reserve space in the stack for the variable `k` </td></tr>

<tr> <td style="text-align:left" colspan="2">*Function body starts*</td></tr>
<tr> <td> ... </td> <td>&nbsp;</td></tr>
<tr> <td> `mov eax, dword ptr [ebp+8]`</td> <td>example of accessing the argument `i` </td></tr>
<tr> <td> `movsd qword ptr [ebp-8], xmm0`</td> <td>example of accessing the variable `k` </td></tr>
<tr> <td> ... </td> <td>&nbsp;</td></tr>

<tr> <td style="text-align:left" colspan="2">*Function epilog starts*</td></tr>
<tr> <td>`mov esp,ebp` </td> <td>reset the stack pointer to a known value</td></tr>
<tr> <td>`pop ebp`</td> <td> restore the previous frame pointer </td></tr>
<tr> <td>`ret`</td> <td> return to the caller </td><tr>

<tr> <th style="text-align:left" colspan="2">Back in the caller</th> </tr>
<tr> <td>`add esp,8`</td> <td>restore the stack pointer </td></tr>
</table>


While the code might be changed in various ways when, for example, optimisation is applied or a different calling convention
is used there is still a reasonable correlation between the resultant code and this model.

- Optimisers often re-use memory addresses for various different purposes and may make
extensive use of registers to avoid having to read and write values to the stack.
- The "stdcall" calling convention used for the Win32 API itself slightly changes the function return: the called
function is responsible for popping the arguments off the stack, but the basic principles are unchanged.
- The 'fastcall' convention passes one or two arguments in registers, rather than on the stack.
- 'frame pointer optimisation' re-purposes the `ebp` register as a general purpose register and uses the `esp` register,
suitably biased by its current offset, as the frame pointer register.

In the 64-bit world while what happens from the programmer's view will be identical, the underlying implementation has some differences.

Here is a summary of how the stack frame for `foo` might look when compiled as part of a 64-bit program:

<table>
<tr> <th>Offset</th> <th>Size</th> <th>Contents</th></tr>
<tr> <td>+72</td> <td>1 byte</td> <td>`char ch`</td></tr>
<tr> <td>+64</td> <td>4 bytes</td> <td>`int i`</tr>
<tr> <td>+56</td> <td>8 bytes</td> <td>*return address*`</tr>
<tr> <td>+32</td> <td>8 bytes</td> <td>`double k`</tr>
</table>

(Again, offsets can be taken from the assembler output). Notice that all the offsets are positive, and the smallest offset is 32.
Perhaps even more surprising is that the stack frame size (from a debugger, the assembler output or by adding calls to a checking function)
is **64** bytes;
more than double the 24 bytes used in the 32-bit case. Why would this be the case -- we might expect *some* of the items to double
in size to match the word size but something else is going on here.

In the 64-bit calling convention the caller passes the first **four** arguments in registers but must reserve space on the stack for them.
This provides a well-defined location for the called program to save and restore
the corresponding argument if the value passed in the register would otherwise be overwritten.

Additionally space for four arguments is *always* reserved even when the function takes fewer than that.
(These 32 bytes just above the return address on each function call are sometimes called the "home space".)
So in our example, although we only have two arguments (`i` and `ch`) our caller will have reserved space for two other
(unused) arguments. The full stack frame for `foo` can therefore be written like this:

<table>
<tr> <td class="alt"/> <th>Offset</th><th>Size</th><th>Contents</th></tr>
<tr> <td class="alt">*High mem*</td> <td>+96</td> <td>&nbsp;</td> <td>*top of frame*</td></tr>
<tr style="color:#606060"> <td class="alt"/> <td>+88</td> <td>8 bytes</td> <td>*reserved for 4th argument*</td></tr>
<tr style="color:#606060"> <td class="alt"/> <td>+80</td> <td>8 bytes</td> <td>*reserved for 3rd argument*</td></tr>
<tr style="color:#606060"> <td class="alt"/> <td>+73</td> <td>7 bytes</td> <td>*padding*</tr>
<tr> <td class="alt"/> <td>+72</td> <td>1 byte</td> <td>`char ch`</td></tr>
<tr style="color:#606060"> <td class="alt"/> <td>+68</td> <td>4 bytes</td> <td>*padding*</tr>
<tr> <td class="alt"/> <td>+64</td> <td>4 bytes</td> <td>`int i`</tr>
<tr> <td class="alt"/> <td>+56</td> <td>8 bytes</td> <td>*return address*`</tr>
<tr style="color:#606060"> <td class="alt"/> <td>+40</td> <td>16 bytes</td> <td>*padding*</tr>
<tr> <td class="alt"></td> <td>+32</td> <td>8 bytes</td> <td>`double k`</tr>
<tr> <td class="alt" style="text-align:right">`rsp`-&gt;*</td> <td style="color:#606060">+0</td> <td style="color:#606060">32 bytes</td> <td style="color:#606060">*argument space for child functions*</td></tr>
<tr> <td class="alt">*Low mem*</td> <td>&nbsp;</td><td>&nbsp;</td> <td>&nbsp;</td></tr>
</table>
(* `rsp` normally remains here for the duration of the function.)

The first offset in the stack frame is +32 because this function will in turn need to reserve stack space for up to four arguments when it 
calls another function. So the function presets the stack pointer just below these four words to avoid having to modify the stack pointer
when making function calls - it can just make the call.

The actual size of the first offset can be greater than 32 if, for example, more than four arguments are passed to a child function;
but it can only be *less* if the function itself does not call any other functions.
 
Note that although we're only using 21 bytes of memory the stack frame is 64 bytes in size: that's over twice as much being
wasted as being used. The 64-bit calling convention does, in general,
seem to increase the stack consumption of the program. However there are a couple of things that help to reduce the stack consumption. 
- Firstly the 64-bit architecture has more registers (eight more general-purpose registers `r8` - `r15`).
This allows more temporary results (or local variables) to be held in registers without requiring stack space to be reserved.
- Secondly, the uniform stack frame convention increases the number of places where a nested function call at the end of a function
can be replaced with a jump. This technique, known as "tail call elimination", allows the called function to 'take over' the current 
stack frame without requiring additional stack usage.

However, it still seems odd (at least to me) that Microsoft did not change the default stack size for applications when compiled as 64-bits: 
by default both 32-bit and 64-bit applications are given a 1Mb stack.

If your existing 32-bit program gets anywhere near this stack limit you may find the 64-bit equivalent needs a bigger stack 
(obviously this is very dependent on the exact call pattern of your program).  This can be set, if necessary, by using the
`/stack` linker option when the program is created - or even after the program has been linked using the same
`/stack` option with the `editbin` program provided with Visual Studio.

This is a possible sequence of instructions for setting up the stack frame when `foo` is called in a 64-bit application:

<table>
<tr> <th style="text-align:left" colspan="2">In the caller</th> </tr>
<tr> <td>`mov dl, 'c' ` </td> <td> set up low 8 bits of the `rdx` register with 'ch'</td></tr>
<tr> <td>`mov ecx, 12 ` </td> <td> set up low 32 bits of the `rcx` register with 'i'</td></tr>
<tr> <td>`call foo`</td> <td>enter `foo`, return address now in place</td></tr>

<tr> <th style="text-align:left" colspan="2">In `foo`</th> </tr>
<tr> <td style="text-align:left" colspan="2">*Function prolog starts*</td></tr>
<tr> <td>`mov byte ptr [rsp+16], dl`</td> <td>save the second argument in the stack frame</td</tr>
<tr> <td>`mov dword ptr [rsp+8], ecx`</td> <td>save the first argument in the stack frame</td</tr>
<tr> <td>`sub rsp,56`</td> <td>reserve space for the local variables,<br>
 and 32 bytes for when `foo` calls a further function</td></tr>

<tr> <td style="text-align:left" colspan="2">*Function body starts *</td></tr>
<tr> <td> ... </td> <td>&nbsp;</td></tr>
<tr> <td> `mov eax, ecx`</td> <td>example of accessing the argument `i` </td></tr>
<tr> <td> `movsdx qword ptr [rsp+32], xmm0`</td> <td>example of accessing the variable `k` </td></tr>
<tr> <td> ... </td> <td>&nbsp;</td></tr>

<tr> <td style="text-align:left" colspan="2">*Function epilog starts*</td></tr>
<tr> <td>`add rsp, 56` </td> <td>reset the stack pointer</td></tr>
<tr> <td>`ret`</td> <td> return to the caller </td><tr>

<tr> <th style="text-align:left">Back in the caller</th> <td>*nothing to see here ... move along *</td></tr>
</table>

As you can see the 64-bit code is simpler than the 32-bit code because most things are done with the 'mov' instruction rather
than using 'push' and 'pop'.
Since the stack pointer register `rsp` does not change once the prolog is completed it can be used as the pointer to the stack frame,
which releases the `rbp` register to be a general-purpose register.

Note too that the 64-bit code only updates the relevant part of the register and memory location. This has the unfortunate effect that, if you
are writing tools to analyse a running program or are debugging code to which you do not have the complete source, you cannot as easily tell the
actual value of function arguments as the complete value in the 64-bit register or memory location may include artifacts from earlier.
In the 32-bit case, when an 8bit char was pushed into the stack, the high 24bits of the 32-bit value were set to zero.

One other change in the 64-bit convention is that the stack pointer must (outside the function prolog and epilog) **always** be aligned to
a multiple of 16 bytes (not, as you might at first expect, 8 bytes to match the word size).
This helps to optimise use of the various instructions that read multiple words of memory at once, without requiring each function
to align the stack dynamically.

Finally note that the 64-bit convention means that the called function returns with the stack restored to its value on entry. 
This means function calls can be made with a variable number of arguments and the caller will ensure the stack is managed correctly.

*Note* that in Visual Studio 2013 Microsoft have added a second (explicit) calling convention for **both** 32-bit and 64-bit programs,
the `__vectorcall` convention. This passes up to six 128bit or 256bit values using the SSE2 registers
`xmm` and `ymm`. I'm not discussing this convention further - interested readers
can investigate this by looking up the keyword on MSDN.

## More on passing variables

The 64-bit bit convention dictates that the first four arguments are passed in fixed registers. These registers,
for integral and pointer values are
`rcx`, `rdx`, `r8` and `r9`.
For floating point values the arguments are passed in `xmm0` - `xmm3`.
(The older x87 FPU registers are not used to pass floating point values in 64-bit mode.)

If there is a mix of integral and floating point arguments the unused registers are normally simply skipped, for example passing a 
`long`, a `double` and an `int`
would use `rcx`, `xmm1` and `r8`.

However, when the function prototype uses ellipses (i.e. it takes a variable number of arguments), any floating pointing values are placed in
**both** the integral **and** the corresponding xmm register since the caller does not know the argument type expected by the called function.

For example, `printf("%lf\n", 1.0);` will pass the 64-bit value representing 1.0 in both the `xmm1` and `rdx` registers.

When a member function is called, the `this` pointer is passed as an implicit argument; it is always the first argument and hence
is passed in `rcx`.

The overall register conventions in the x64 world are quite clearly defined. The documentation [Register Usage] describes how each register is used
and lists which register values must be retained over a function call and which ones might be destroyed.

## Bigger (or odder) values

Another change in the 64-bit calling convention is how larger variables (those too big for a single 64-bit register) or "odd" sized variables
(those that are not **exactly** the size of a char, short, int or full register) are passed.
In the 32-bit world arguments passed by value were simply copied onto the stack, taking up as many complete 32-bit words of stack space as required.
The resulting temporary variable (and any padding bytes) would be contiguous in memory with the other function arguments.

In the 64-bit world any argument that isn't 8, 16 32 or 64-bits in size is passed by **reference** - the caller is responsible for 
making a copy of the argument on the stack and passing the address of this temporary as the appropriate argument. (Note that this passing by
reference is transparent to the source code.)
Additionally the caller must ensure that any temporary so allocated is on a 16byte aligned address.
This means that the temporary variables themselves will not necessarily be contiguous in memory - the 'over-sized' variables will be located
in working storage above the stack frame for the target function. While this should very rarely affect any code it is something to be aware 
of when working with code that tries to play 'clever tricks' with its function arguments.

## Local variables

The compiler will reserve stack space for local variables (whether named or temporary) unless they can be held in registers.
However it will re-order the variables for various reasons - for example to pack two `int` values next to each other.
Additionally arrays are normally placed together at one end to try and reduce the damage that can be done by a buffer overrun.

## Return values

Integer (or pointer) values up to 64-bits in size are returned from a function using the `rax` register, and
floating point values are returned in `xmm0`.
Values other than these are constructed in memory at an address specified by the caller.
The pointer to this memory is passed as a hidden first argument to the function (or a hidden second argument,
following the `this` pointer, when calling a member function)
and then returned in `rax`.

## Debugging

In this example the first two instructions in the prolog save the argument values, passed in as register values, in the stack frame.
When optimisation is turned on this step is typically omitted and the register values are simply used directly.
This saves at least one memory access for each argument and so improves performance.

The compiler can now use the stack space for saving intermediate results (which reduces its need for other stack space) knowing
there will always be space for four 64-bit values on the stack.

Unfortunately this performance benefit comes at the price of making debugging much harder: since the function arguments are
now only held in (volatile)
registers it can become hard to determine their values in a debugger (or when examining a dump). In many cases the value has
simply been lost and you have
to work back up the call stack to try and identify what the value *might* have been on entry to the function.

While this sort of optimisation is common in 32-bits for *local* variables the fact the arguments are (usually) passed on the
stack does increase their
longevity. For example, consider this simple function:
```
void test(int a, int b, int c, int d, int e)
{
  printf("sum: %i\n", a + b + c + d + e);
}
```

If I build a program with an optimised build of this function and breakpoint on the printf statement, in a 32-bit application
I can still see the values of the arguments (when using the default calling convention, 
unless the optimiser has made the entire function inlined). If I try the same thing with a 64-bit optimised build I get, for example:

<table>
<tr><th> Argument </th><th> Actual value passed </th><th> Value displayed in the debugger </th></tr>
<tr><td> a </td><td> 11111 </td><td> 1 </td></tr>
<tr><td> b </td><td> 22222 </td><td> 1067332614 </td></tr>
<tr><td> c </td><td> 33333 </td><td> 1 </td></tr>
<tr><td> d </td><td> 44444 </td><td> 4058400 </td></tr>
<tr><td> e </td><td> 55555 </td><td> 55555 </td></tr>
</table>

As you can see, the fifth argument is displayed correctly because, as described above, only the first four arguments
(in Windows 64-bit) are passed in registers.
In general the debugger cannot locate the values from the register (even assuming the value is still there!) and so it
simply displays what is in the stack frame
location reserved for that argument; but as the argument has not been persisted to the stack frame in the release build
the contents are arbitrary.

In some cases, for example where the argument is a pointer value, the arbitrary value can even break the debugger (at least in VS 2012).

This can make it significantly harder to identify the reason for a failure in a build of an application where some or all
of the code is compiled with optimisation.
This is a particular problem when you get a dump of a production system, where reproducing the fault yourself on a non-optimised
build may be hard.

## Stack walking

The other main area where the 64-bit calling convention differs from the 32-bit one is when walking the stack.
There are two main cases when the stack is walked.

- Handling an exception
- In various debugging scenarios

In the 32-bit world these two cases were handled very differently. 

For exceptions, each thread in the Win32 subsystem contained a singly-linked list of exception handlers, maintained in the stack 
with the address of the first handler held in the thread environment block.

Additionally the MSVC compiler maintained a simple state machine for each function containing exception handling logic (either implicit or explicit) 
and used the state variable, which was also held in the stack frame, during unwinding of the stack when handling an exception.

This state variable was used by the exception handling logic, in conjunction with some other tables built into the binary image by the compiler,
to find the catch handler, if any, for the thrown exception and also to identify the completely constructed objects on the stack that
should be destroyed when unwinding the stack.

There were at least three main problems with this approach.
- The exception code was fragile under accidental or malicious stack overwrites
- Management of the exception chain had a measurable performance impact
- The stack frame was larger, again impacting performance (among other things)

For the various debugging scenarios the simplest approach was to follow the chained base pointers: the value of the `ebp` register
provides the address of the current frame. Each frame was expected to have the entry at +0 containing the previous base pointer and the entry
at +4 to contain the return address.

This mechanism was, like the exception chain, quite fragile and was also complicated by the 'frame pointer optimisation'. The MSVC toolchain
would add additional data to the debugging files (with the 'pdb' extension) for each module containing information to enable a debugger to
locate the stack frame even when this optimisation was enabled, but this required the PDB files to be present and accessible to allow the stack
to be reliably walked. The tables were also quite slow to access, meaning their use was unfeasible in some of the places where
stack walking at runtime was used (such as tracking memory allocations and deallocations).

As you may know, although the frame pointer optimisation does produce a performance benefit (normally a low single digit percentage),
Microsoft have disabled it in their operating system builds since Windows XP service pack 2 as they considered the increased ability
to debug production problems was more significant than the loss of performance.

The 64-bit convention uses a pure table-based system that is linked into the binary image and is used to walk the stack in **both** the cases above.
This has several benefits. Firstly, there is improved robustness and security since the data structures are in read-only memory rather than created
on the stack. Secondly, there is a small performance improvement as the tables are fixed and only accessed if and when stack walking is
required. Thirdly, since the data structures are held in the binary itself, stack walking is reliable even if the PDB file is not present.

The instruction pointer is used to find the currently executing image and the offset into that image. This offset is then used to look up the
correct entry for the current function in the tables for the current module. The table entries contain, among other things, a description
of the stack frame for each function: in particular which register contains the base address and what the vital offset values are.
This information allows the stack walker to reliably walk up the list of stack frames to identify the calling functions and/or
find the correct exception handler without relying on data tables held in the stack itself.

While not quite as fast a simply chaining up a linked list of exception records or frame pointers the mechanism is fast enough to be
used at runtime. The Win32 API exposes a method, `CaptureStackBackTrace` that understands the data structures involved
and can be used by application programs to capture the address of the functions in the call stack.

There are further functions providing support for this stack walking: such as `RtlLookupFunctionEntry` which obtains
a pointer to the relevant data for a specific address; but I recommend that you use the supplied stack capture function: as while 
the data structures are (at least partly) documented making correct use of them is not for the faint hearted.

If you need to write 64-bit *assembler* code then you need to ensure that these tables are correctly built. There are a number
of restrictions as to the instructions that can be used in the function prolog and epilog; and additional assembler directives
must be written to ensure the assembler generates the correct data structures.

An even more complicated activity is when you need to generate executable code on the fly at runtime. The Win32 API provides 
a function `RtlAddFunctionTable` that you can use to dynamically add function table entries to the running module.
Unfortunately there do not seem to be any helper functions to facilitate building up the required data structures.

Additionally, it can be hard to verify that the data structures are in fact correct - the first indication that they are incorrect 
may occur when the system is trying to handle an exception as, if it is unable to correctly process the function table entry
for your dynamically created code this will almost certainly result in unexpected program termination.

However, since the same control structures are used for walking the call stack in the debugger as are used when
an exception is thrown, some checking can be done by verifying the call stack displays correctly in a debugger
as you step through the generated code.

## Dump busting

While in general the data tables in the binary images do provide a very reliable way to walk the stack, the technique can fail when processing
a dump if the actual binaries are not accessible to the debugger that is reading the dump file.

As an example, consider a very simple function that throws an exception:
```
void func()
{
  throw 27;
}
```
If we package this function in a DLL, and call this DLL from a main program, the exception handling logic walks up the chain from the
site of the exception (which is actually the `RaiseException` function inside kernelbase.dll) to the handler, if any, in the 
calling function. At the time of the exception all these tables are present in the executing binaries; but if a minidump is taken then
the code modules may well not be included in the dump. This will depend on which options are used when the minidump is created,
but space is often at a premium and so a complete memory dump may not be realistic.

Let us look at what happens when the resulting dump is processed on a *different* machine where not all the binaries
are present: in this example we have access to a copy of the `main.exe` program but not of the `function.dll`.
In the 32-bit world the absence of the binaries simply means the function names are missing, but the stack walk itself
can complete (I've disabled FPO in this example).

**The 32-bit exception in a debugger**
```
  KERNELBASE!RaiseException+0x58
  function.dll!_CxxThrowException(void * pExceptionObject, const _s__ThrowInfo * pThrowInfo) Line 152
  function.dll!func() Line 4
  main.exe!main() Line 12
  main.exe!__tmainCRTStartup() Line 241
  kernel32.dll!@BaseThreadInitThunk@12()
  ntdll.dll!___RtlUserThreadStart@8()
  ntdll.dll!__RtlUserThreadStart@8()
```
**The same exception, examined from a minidump on another machine**
```
  KERNELBASE!RaiseException+0x58
  function+0x108b
  function+0x1029
  main.exe!main() Line 12
  main.exe!__tmainCRTStartup() Line 241
  kernel32.dll!@BaseThreadInitThunk@12()
  ntdll.dll!___RtlUserThreadStart@8()
  ntdll.dll!__RtlUserThreadStart@8()
```

If the same operations are performed with a 64-bit build of the same program the results when reading the dump are quite different.

**The 64-bit exception in a debugger**
```
  KernelBase.dll!RaiseException()
  function.dll!_CxxThrowException(void * pExceptionObject, const _s__ThrowInfo * pThrowInfo) Line 152
  function.dll!func() Line 4
  main.exe!main() Line 11
  main.exe!__tmainCRTStartup() Line 241
  kernel32.dll!BaseThreadInitThunk()
  ntdll.dll!RtlUserThreadStart()
```
(Note that debuggers sometimes display slightly different line numbers - the entry for `main.exe!main()` from which `func` was
called is shown as Line 11 in the 64-bit debugger (correctly) but as the *next* line, Line 12, in the 32-bit debugger.)

**The same exception, examined from a minidump on another machine**
```
  KERNELBASE!RaiseException+0x39
  function+0x1110
  function+0x2bd90
  0x2af870
  0x1
  function+0xf0
  0x00000001`e06d7363`
```
The stack walk is unable to get before the first address inside `function.dll` in the absence of the function data tables.
(What the debugger seems to do when the data is missing is to simply try the next few possible entries on the stack but it is very 
rarely successful in finding the next stack frame.)

Note that providing the PDB files in this case does not help to walk the stack correctly, as it is the DLL and EXE files
that contain the stack walking data. It therefore becomes important to ensure that you can easily obtain the **exact** versions of
the binary files that the target machine is using if you wish to successfully process the smaller format minidump files.

## Conclusion

The 64-bit Windows calling convention does seem to be an improvement on the 32-bit convention, although I personally wish that
a little more care had been taken to ensure that problem analysis was easier. As I have mentioned above, while the mechanism
does offer increased performance, it also decreases the likelihood of successful problem determination (especially with an optimised build.)

It is my hope that some understanding of how the 64-bit convention works will aid programmers as they migrate towards writing more 
64-bit programs for the Windows platform.

## Acknowledgements

Many thanks to Lee Benfield, Dan Azzopardi and the Overload reviewers for their suggestions and corrections which have helped to improve this article.

## Useful references

[Microsoft x64 Calling Convention](http://msdn.microsoft.com/en-us/library/9b372w95.aspx)

[Microsoft Macro Assembler Directives](http://msdn.microsoft.com/en-us/library/8t163bt0.aspx)

[Register Usage](http://msdn.microsoft.com/en-us/library/9z1stfyw.aspx)

<hr>
Copyright (c) Roger Orr 2021-07-06 (First published in Overload, Apr 2014)
