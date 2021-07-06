<HTML>
<HEAD>
<link type="text/css" rel="stylesheet" href="../or2.css">
<TITLE>
Windows 64-bit calling conventions - DRAFT 5
</TITLE>
<style type="text/css">
table,th,td
{
  border: 1px solid black;
}
table
{
  border-spacing: 0
}
th,td
{
  padding: 4px 7px 4px 7px;
}
td.alt
{
  border: 0px;
  padding: 4px 7px 4px 7px;
}
</style>
</HEAD>
<BODY style="font-family:arial">
<H1>Windows 64-bit calling conventions - DRAFT 5</H1>

<H2>
Introduction
</H2>
There are many layers of technology in computing: we even use the term "technology stack" when trying to name the set of components used
in the development of a given application. While it may not be necessary to understand all the layers to make use of them a little
comprehension of what's going on can improve our overall grasp of the environment and, sometimes help us to work with,
rather than against, the underlying technology.
<p>
Besides, it's interesting!
<p>
Many of us write programs that run on the Windows operating system and increasingly many of these programs are running as 64-bit applications.
While Microsoft have done a fairly good job at hiding the differences between the 32-bit and 64-bit windows environments there are 
differences and some of the things we may have learnt in the 32-bit world are no longer true, or at least have changed subtly.
<p>
In this article I will cover how the calling convention has changed for 64-bit Windows. Note that while this is very <i>similar</i> to
the 64-bit calling conventions used in other environments, notably Linux, on the same 64-bit hardware I'm not going to specifically
address other environments (other than in passing.) I am also only targeting the "x86-64" architecture (also known as "AMD 64")
and I'm not going to refer to the Intel "IA-64" architecture. I think Intel have lost that battle.
<p>
I will be looking at a small number of assembler instructions; but you shouldn't need to understand much assembler to make sense of
the principles of what the code is doing. I am also using the 32-bit calling conventions as something to contrast the 64-bit ones with;
but again, I am not assuming that you are already familiar with these.
<H2>
A simple model of stack frames
</H2
Let's start by looking in principle at the stack usage of a typical program.
<p>
The basic principle of a stack frame is that each function call operates against a 'frame' of data held on the stack that includes all the directly
visible function arguments and local variables.
When a second function is called from the first, the call sets up a new frame further into the stack
(confusingly the new frame is sometimes described as "further up" the stack and sometimes as "further down").
<p>
This design allows for good encapsulation: each function deals with a well-defined set of variables and, in general, you do not 
need to concern yourself with variables outside the current stack frame in order to fully understand the behaviour and semantics of 
a specific function. Global variables, pointers and references make things a little more complicated in practice.
<p>
The "base" address of the frame is not necessarily the lowest address in the frame, and so some items in the frame may have a higher
address than the frame pointer (+ve offset) and some may have a lower address (-ve offset).
<p>
In the 32-bit world the <code>esp</code> register (the sp stands for "stack pointer") holds the current value of the stack pointer and
the <code>ebp</code> register (the bp stands for "base pointer") is used <b>by default</b> as the pointer to the
base address of the stack frame (the use of 'frame pointer optimisation' (FPO) - also called 'frame pointer omission' - can repurpose this register.)
Later on we'll see what happens in 64-bits.
<p>
Let's consider a simple example function <code>foo</code>:
<p>
<pre>void foo(int i, char ch)
{
   double k;
   // ...
}
</pre>

Here is how the stack frame might look when compiled as part of a 32-bit program:
<p>

<table>
<tr> <td class="alt"/> <th>Offset</th> <th>Size</th> <th>Contents</th></tr>
<tr> <td class="alt"><i>High mem</i></td> <td>+16</td><td>&nbsp;</td> <td><i>Top of frame</i></td></tr>
<tr style="color:#606060"> <td class="alt"/> <td>+13</td><td>3 bytes</td> <td><i>padding</i></td></tr>
<tr> <td class="alt"/> <td>+12</td> <td>1 byte</td> <td><code>char ch</code> <i>(arguments pushed R to L)</i></td></tr>
<tr> <td class="alt"/> <td>+8</td> <td>4 bytes</td> <td><code>int i</code></td></tr>
<tr> <td class="alt"/> <td>+4</td> <td>4 bytes</td> <td><i>return address</i></code></td></tr>
<tr> <td class="alt" style="text-align:right"><code>ebp</code>-&gt;</td> <td>+0</td><td>4 bytes</td> <td><i>previous frame base</i></td></tr>
<tr> <td class="alt" style="text-align:right"><code>esp</code>-&gt;*</td> <td>-8</td> <td>8 bytes</td> <td><code>double k </code><i>(and other local variables)</i></td></tr>
<tr> <td class="alt"><i>Low mem</i></td> <td>&nbsp;</td><td>&nbsp;</td> <td>&nbsp;</td></tr>
</table>
(* <code>esp</code> starts here but takes values lower in memory as the function executes.)
<p>
Note: the assembler listing output that can be obtained from the MSVC compiler (<code>/FAsc</code>) handily displays many of these offsets, for example:
<pre><code>
_k$ = -8   ; size = 8
_i$ = 8    ; size = 4
_ch$ = 12  ; size = 1
</code></pre>
<p>
This is conceptually quite simple and, at least without optimisation, the actual implementation of the 
program in terms of the underlying machine code and use of memory may well match this model.
This is the model that many programmers are used to
and they may even implicitly translate the source code into something like the above memory layout.
<p>
The total size of the stack frame is 24 bytes: there are 21 bytes in use (the contiguous range from -8 to +13) but the frame top is
rounded up to the next 4 byte boundary.
<p>
You can demonstrate the stack frame size in several ways; one way is by calling a function that takes the address of a local variable
before and during calling <code>foo</code>
(although note that this simple-minded technique may not work as-is when aggressive optimisation is enabled).
For example:
<code><pre>
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
</pre></code>

In the caller:
<code><pre>
  check();
  foo(12, 'c');
</pre></code>

In the function:
<code><pre>
  void foo(int i, char ch)
  {
    check();
    double k;
    // ...
</pre></code>

<p>
The 32-bit stack frame for <code>foo</code> may be built up like this when it is called:
<p>

<table>
<tr> <th style="text-align:left" colspan="2">In the caller</th> </tr>
<tr> <td><code>push 99 </code></td> <td>set <code>ch</code> in what will become <code>foo</code>'s frame to 'c'</td></tr>
<tr> <td><code>push 12 </code></td> <td>set up <code>i</code> in <code>foo</code>'s frame</td></tr>
<tr> <td><code>call foo</code></td> <td>enter <code>foo</code>, return address now in place</td></tr>

<tr> <th style="text-align:left" colspan="2">In <code>foo</code></th> </tr>
<tr> <td style="text-align:left" colspan="2"><i>Function prolog</i></td></tr>
<tr> <td><code>push ebp</code></td> <td>save the previous frame register </td></tr>
<tr> <td><code>mov ebp, esp</code></td> <td>set register <code>ebp</code> to point to the frame base</td></tr>
<tr> <td><code>sub esp, 8</code></td> <td> reserve space in the stack for the variable <code>k</code> </td></tr>

<tr> <td style="text-align:left" colspan="2"><i>Function body starts</i></td></tr>
<tr> <td> ... </td> <td>&nbsp;</td></tr>
<tr> <td> <code>mov eax, dword ptr [ebp+8]</code></td> <td>example of accessing the argument <code>i</code> </td></tr>
<tr> <td> <code>movsd qword ptr [ebp-8], xmm0</code></td> <td>example of accessing the variable <code>k</code> </td></tr>
<tr> <td> ... </td> <td>&nbsp;</td></tr>

<tr> <td style="text-align:left" colspan="2"><i>Function epilog starts</i></td></tr>
<tr> <td><code>mov esp,ebp</code> </td> <td>reset the stack pointer to a known value</td></tr>
<tr> <td><code>pop ebp</code></td> <td> restore the previous frame pointer </td></tr>
<tr> <td><code>ret</code></td> <td> return to the caller </td><tr>

<tr> <th style="text-align:left" colspan="2">Back in the caller</th> </tr>
<tr> <td><code>add esp,8</code></td> <td>restore the stack pointer </td></tr>
</table>

<p>
While the code might be changed in various ways when, for example, optimisation is applied or a different calling convention
is used there is still a reasonable correlation between the resultant code and this model.
<p>
<ul>
<li>Optimisers often re-use memory addresses for various different purposes and may make
extensive use of registers to avoid having to read and write values to the stack.
<li>The "stdcall" calling convention used for the Win32 API itself slightly changes the function return: the called
function is responsible for popping the arguments off the stack, but the basic principles are unchanged.
<li>The 'fastcall' convention passes one or two arguments in registers, rather than on the stack.
<li>'frame pointer optimisation' re-purposes the <code>ebp</code> register as a general purpose register and uses the <code>esp</code> register,
suitably biased by its current offset, as the frame pointer register.
</ul>
<p>
In the 64-bit world while what happens from the programmer's view will be identical, the underlying implementation has some differences.

Here is a summary of how the stack frame for <code>foo</code> might look when compiled as part of a 64-bit program:
<p>

<table>
<tr> <th>Offset</th> <th>Size</th> <th>Contents</th></tr>
<tr> <td>+72</td> <td>1 byte</td> <td><code>char ch</code></td></tr>
<tr> <td>+64</td> <td>4 bytes</td> <td><code>int i</code></tr>
<tr> <td>+56</td> <td>8 bytes</td> <td><i>return address</i></code></tr>
<tr> <td>+32</td> <td>8 bytes</td> <td><code>double k</code></tr>
</table>

<p>
(Again, offsets can be taken from the assembler output). Notice that all the offsets are positive, and the smallest offset is 32.
Perhaps even more surprising is that the stack frame size (from a debugger, the assembler output or by adding calls to a checking function) is <b>64</b> bytes;
more than double the 24 bytes used in the 32-bit case. Why would this be the case -- we might expect <i>some</i> of the items to double in size to match the word
size but something else is going on here.
<p>
In the 64-bit calling convention the caller passes the first <b>four</b> arguments in registers but must reserve space on the stack for them.
This provides a well-defined location for the called program to save and restore
the corresponding argument if the value passed in the register would otherwise be overwritten.
<p>
Additionally space for four arguments is <i>always</i> reserved even when the function takes fewer than that.
(These 32 bytes just above the return address on each function call are sometimes called the "home space".)
So in our example, although we only have two arguments (<code>i</code> and <code>ch</code>) our caller will have reserved space for two other
(unused) arguments. The full stack frame for <code>foo</code> can therefore be written like this:
<p>

<table>
<tr> <td class="alt"/> <th>Offset</th><th>Size</th><th>Contents</th></tr>
<tr> <td class="alt"><i>High mem</i></td> <td>+96</td> <td>&nbsp;</td> <td><i>top of frame</i></td></tr>
<tr style="color:#606060"> <td class="alt"/> <td>+88</td> <td>8 bytes</td> <td><i>reserved for 4th argument</i></td></tr>
<tr style="color:#606060"> <td class="alt"/> <td>+80</td> <td>8 bytes</td> <td><i>reserved for 3rd argument</i></td></tr>
<tr style="color:#606060"> <td class="alt"/> <td>+73</td> <td>7 bytes</td> <td><i>padding</i></tr>
<tr> <td class="alt"/> <td>+72</td> <td>1 byte</td> <td><code>char ch</code></td></tr>
<tr style="color:#606060"> <td class="alt"/> <td>+68</td> <td>4 bytes</td> <td><i>padding</i></tr>
<tr> <td class="alt"/> <td>+64</td> <td>4 bytes</td> <td><code>int i</code></tr>
<tr> <td class="alt"/> <td>+56</td> <td>8 bytes</td> <td><i>return address</i></code></tr>
<tr style="color:#606060"> <td class="alt"/> <td>+40</td> <td>16 bytes</td> <td><i>padding</i></tr>
<tr> <td class="alt"></td> <td>+32</td> <td>8 bytes</td> <td><code>double k</code></tr>
<tr> <td class="alt" style="text-align:right"><code>rsp</code>-&gt;*</td> <td style="color:#606060">+0</td> <td style="color:#606060">32 bytes</td> <td style="color:#606060"><i>argument space for child functions</i></td></tr>
<tr> <td class="alt"><i>Low mem</i></td> <td>&nbsp;</td><td>&nbsp;</td> <td>&nbsp;</td></tr>
</table>
(* <code>rsp</code> normally remains here for the duration of the function.)
<p>
The first offset in the stack frame is +32 because this function will in turn need to reserve stack space for up to four arguments when it 
calls another function. So the function presets the stack pointer just below these four words to avoid having to modify the stack pointer
when making function calls - it can just make the call.
<p>
The actual size of the first offset can be greater than 32 if, for example, more than four arguments are passed to a child function;
but it can only be <i>less</i> if the function itself does not call any other functions.
<p> 
Note that although we're only using 21 bytes of memory the stack frame is 64 bytes in size: that's over twice as much being wasted as being used. The 64-bit calling convention does, in general,
seem to increase the stack consumption of the program. However there are a couple of things that help to reduce the stack consumption. 
<ul>
<li>Firstly the 64-bit architecture has more registers (eight more general-purpose registers <code>r8</code> - <code>r15</code>).
This allows more temporary results (or local variables) to be held in registers without requiring stack space to be reserved.
<li>Secondly, the uniform stack frame convention increases the number of places where a nested function call at the end of a function
can be replaced with a jump. This technique, known as "tail call elimination", allows the called function to 'take over' the current 
stack frame without requiring additional stack usage.
</ul>
<p>
However, it still seems odd (at least to me) that Microsoft did not change the default stack size for applications when compiled as 64-bits: 
by default both 32-bit and 64-bit applications are given a 1Mb stack.
<p>
If your existing 32-bit program gets anywhere near this stack limit you may find the 64-bit equivalent needs a bigger stack 
(obviously this is very dependent on the exact call pattern of your program).  This can be set, if necessary, by using the
<code>/stack</code> linker option when the program is created - or even after the program has been linked using the same
<code>/stack</code> option with the <code>editbin</code> program provided with Visual Studio.

<p>
This is a possible sequence of instructions for setting up the stack frame when <code>foo</code> is called in a 64-bit application:
<p>

<table>
<tr> <th style="text-align:left" colspan="2">In the caller</th> </tr>
<tr> <td><code>mov dl, 'c' </code> </td> <td> set up low 8 bits of the <code>rdx</code> register with 'ch'</td></tr>
<tr> <td><code>mov ecx, 12 </code> </td> <td> set up low 32 bits of the <code>rcx</code> register with 'i'</td></tr>
<tr> <td><code>call foo</code></td> <td>enter <code>foo</code>, return address now in place</td></tr>

<tr> <th style="text-align:left" colspan="2">In <code>foo</code></th> </tr>
<tr> <td style="text-align:left" colspan="2"><i>Function prolog starts</i></td></tr>
<tr> <td><code>mov byte ptr [rsp+16], dl</code></td> <td>save the second argument in the stack frame</td</tr>
<tr> <td><code>mov dword ptr [rsp+8], ecx</code></td> <td>save the first argument in the stack frame</td</tr>
<tr> <td><code>sub rsp,56</code></td> <td>reserve space for the local variables,<br>
 and 32 bytes for when <code>foo</code> calls a further function</td></tr>

<tr> <td style="text-align:left" colspan="2"><i>Function body starts </i></td></tr>
<tr> <td> ... </td> <td>&nbsp;</td></tr>
<tr> <td> <code>mov eax, ecx</code></td> <td>example of accessing the argument <code>i</code> </td></tr>
<tr> <td> <code>movsdx qword ptr [rsp+32], xmm0</code></td> <td>example of accessing the variable <code>k</code> </td></tr>
<tr> <td> ... </td> <td>&nbsp;</td></tr>

<tr> <td style="text-align:left" colspan="2"><i>Function epilog starts</i></td></tr>
<tr> <td><code>add rsp, 56</code> </td> <td>reset the stack pointer</td></tr>
<tr> <td><code>ret</code></td> <td> return to the caller </td><tr>

<tr> <th style="text-align:left">Back in the caller</th> <td><i>nothing to see here ... move along </i></td></tr>
</table>

<p>
As you can see the 64-bit code is simpler than the 32-bit code because most things are done with the 'mov' instruction rather than using 'push' and 'pop'.
Since the stack pointer register <code>rsp</code> does not change once the prolog is completed it can be used as the pointer to the stack frame,
which releases the <code>rbp</code> register to be a general-purpose register.
<p>
Note too that the 64-bit code only updates the relevant part of the register and memory location. This has the unfortunate effect that, if you
are writing tools to analyse a running program or are debugging code to which you do not have the complete source, you cannot as easily tell the
actual value of function arguments as the complete value in the 64-bit register or memory location may include artifacts from earlier.
In the 32-bit case, when an 8bit char was pushed into the stack, the high 24bits of the 32-bit value were set to zero.
<p>
One other change in the 64-bit convention is that the stack pointer must (outside the function prolog and epilog) <b>always</b> be aligned to
a multiple of 16 bytes (not, as you might at first expect, 8 bytes to match the word size).
This helps to optimise use of the various instructions that read multiple words of memory at once, without requiring each function
to align the stack dynamically.
<p>
Finally note that the 64-bit convention means that the called function returns with the stack restored to its value on entry. 
This means function calls can be made with a variable number of arguments and the caller will ensure the stack is managed correctly.
<p>
<i>Note that in Visual Studio 2013 Microsoft have added a second (explicit) calling convention for <b>both</b> 32-bit and 64-bit programs,
the <code>__vectorcall</code> convention. This passes up to six 128bit or 256bit values using the SSE2 registers
<code>xmm</code> and <code>ymm</code>. I'm not discussing this convention further - interested readers
can investigate this by looking up the keyword on MSDN.</i>
<p>
<H2>
More on passing variables
</H2>
The 64-bit bit convention dictates that the first four arguments are passed in fixed registers. These registers, for integral and pointer values are
<code>rcx</code>, <code>rdx</code>, <code>r8</code> and <code>r9</code>.
For floating point values the arguments are passed in <code>xmm0</code> - <code>xmm3</code>.
(The older x87 FPU registers are not used to pass floating point values in 64-bit mode.)
<br>
If there is a mix of integral and floating point arguments the unused registers are normally simply skipped, for example passing a 
<code>long</code>, a <code>double</code> and an <code>int</code>
would use <code>rcx</code>, <code>xmm1</code> and <code>r8</code>.
<p>
However, when the function prototype uses ellipses (i.e. it takes a variable number of arguments), any floating pointing values are placed in
<b>both</b> the integral <b>and</b> the corresponding xmm register since the caller does not know the argument type expected by the called function.
<br>
For example, <code>printf("%lf\n", 1.0);</code> will pass the 64-bit value representing 1.0 in both the <code>xmm1</code> and <code>rdx</code> registers.
<p>
When a member function is called, the <code>this</code> pointer is passed as an implicit argument; it is always the first argument and hence
is passed in <code>rcx</code>.
<p>
The overall register conventions in the x64 world are quite clearly defined. The documentation [Register Usage] describes how each register is used
and lists which register values must be retained over a function call and which ones might be destroyed.
<p>
<b>Bigger (or odder) values</b>
<p>

Another change in the 64-bit calling convention is how larger variables (those too big for a single 64-bit register) or "odd" sized variables
(those that are not <b>exactly</b> the size of a char, short, int or full register) are passed.
In the 32-bit world arguments passed by value were simply copied onto the stack, taking up as many complete 32-bit words of stack space as required.
The resulting temporary variable (and any padding bytes) would be contiguous in memory with the other function arguments.
<p>
In the 64-bit world any argument that isn't 8, 16 32 or 64-bits in size is passed by <b>reference</b> - the caller is responsible for 
making a copy of the argument on the stack and passing the address of this temporary as the appropriate argument. (Note that this passing by
reference is transparent to the source code.)
Additionally the caller must ensure that any temporary so allocated is on a 16byte aligned address.
This means that the temporary variables themselves will not necessarily be contiguous in memory - the 'over-sized' variables will be located
in working storage above the stack frame for the target function. While this should very rarely affect any code it is something to be aware 
of when working with code that tries to play 'clever tricks' with its function arguments.
<p>
<b>Local variables</b>
<p>
The compiler will reserve stack space for local variables (whether named or temporary) unless they can be held in registers.
However it will re-order the variables for various reasons - for example to pack two <code>int</code> values next to each other.
Additionally arrays are normally placed together at one end to try and reduce the damage that can be done by a buffer overrun.
<p>
<b>Return values</b>
<p>
Integer (or pointer) values up to 64-bits in size are returned from a function using the <code>rax</code> register, and
floating point values are returned in <code>xmm0</code>.
Values other than these are constructed in memory at an address specified by the caller.
The pointer to this memory is passed as a hidden first argument to the function (or a hidden second argument,
following the <code>this</code> pointer, when calling a member function)
and then returned in <code>rax</code>.
<H2>
Debugging
</H2>
In this example the first two instructions in the prolog save the argument values, passed in as register values, in the stack frame.
When optimisation is turned on this step is typically omitted and the register values are simply used directly.
This saves at least one memory access for each argument and so improves performance.
<p>
The compiler can now use the stack space for saving intermediate results (which reduces its need for other stack space) knowing
there will always be space for four 64-bit values on the stack.
<p>
Unfortunately this performance benefit comes at the price of making debugging much harder: since the function arguments are now only held in (volatile)
registers it can become hard to determine their values in a debugger (or when examining a dump). In many cases the value has simply been lost and you have
to work back up the call stack to try and identify what the value <i>might</i> have been on entry to the function.
<p>
While this sort of optimisation is common in 32-bits for <i>local</i> variables the fact the arguments are (usually) passed on the stack does increase their
longevity. For example, consider this simple function:
<pre><code>
void test(int a, int b, int c, int d, int e)
{
  printf("sum: %i\n", a + b + c + d + e);
}
</code></pre>
<p>
If I build a program with an optimised build of this function and breakpoint on the printf statement, in a 32-bit application I can still see the values of the arguments (when using the default calling convention, 
unless the optimiser has made the entire function inlined). If I try the same thing with a 64-bit optimised build I get, for example:
<p>

<table>
<tr><th> Argument </th><th> Actual value passed </th><th> Value displayed in the debugger </th></tr>
<tr><td> a </td><td> 11111 </td><td> 1 </td></tr>
<tr><td> b </td><td> 22222 </td><td> 1067332614 </td></tr>
<tr><td> c </td><td> 33333 </td><td> 1 </td></tr>
<tr><td> d </td><td> 44444 </td><td> 4058400 </td></tr>
<tr><td> e </td><td> 55555 </td><td> 55555 </td></tr>
</table>

<p>
As you can see, the fifth argument is displayed correctly because, as described above, only the first four arguments (in Windows 64-bit) are passed in registers.
In general the debugger cannot locate the values from the register (even assuming the value is still there!) and so it simply displays what is in the stack frame
location reserved for that argument; but as the argument has not been persisted to the stack frame in the release build the contents are arbitrary.
<p>
In some cases, for example where the argument is a pointer value, the arbitrary value can even break the debugger (at least in VS 2012).
<p>
This can make it significantly harder to identify the reason for a failure in a build of an application where some or all of the code is compiled with optimisation.
This is a particular problem when you get a dump of a production system, where reproducing the fault yourself on a non-optimised build may be hard.
<H2>
Stack walking
</H2>
The other main area where the 64-bit calling convention differs from the 32-bit one is when walking the stack.
There are two main cases when the stack is walked.
<ul>
<li>Handling an exception
<li>In various debugging scenarios
</ul>
In the 32-bit world these two cases were handled very differently. 
<p>
For exceptions, each thread in the Win32 subsystem contained a singly-linked list of exception handlers, maintained in the stack 
with the address of the first handler held in the thread environment block.
<br>
Additionally the MSVC compiler maintained a simple state machine for each function containing exception handling logic (either implicit or explicit) 
and used the state variable, which was also held in the stack frame, during unwinding of the stack when handling an exception.
<p>
This state variable was used by the exception handling logic, in conjunction with some other tables built into the binary image by the compiler,
to find the catch handler, if any, for the thrown exception and also to identify the completely constructed objects on the stack that should be destroyed
when unwinding the stack.
<p>
There were at least three main problems with this approach.
<ul>
<li>The exception code was fragile under accidental or malicious stack overwrites
<li>Management of the exception chain had a measurable performance impact
<li>The stack frame was larger, again impacting performance (among other things)
</ul>
<p>
For the various debugging scenarios the simplest approach was to follow the chained base pointers: the value of the <code>ebp</code> register
provides the address of the current frame. Each frame was expected to have the entry at +0 containing the previous base pointer and the entry
at +4 to contain the return address.
<p>
This mechanism was, like the exception chain, quite fragile and was also complicated by the 'frame pointer optimisation'. The MSVC toolchain
would add additional data to the debugging files (with the 'pdb' extension) for each module containing information to enable a debugger to
locate the stack frame even when this optimisation was enabled, but this required the PDB files to be present and accessible to allow the stack
to be reliably walked. The tables were also quite slow to access, meaning their use was unfeasible in some of the places where
stack walking at runtime was used (such as tracking memory allocations and deallocations).
<p>
As you may know, although the frame pointer optimisation does produce a performance benefit (normally a low single digit percentage),
Microsoft have disabled it in their operating system builds since Windows XP service pack 2 as they considered the increased ability
to debug production problems was more significant than the loss of performance.
<p>
The 64-bit convention uses a pure table-based system that is linked into the binary image and is used to walk the stack in <b>both</b> the cases above.
This has several benefits. Firstly, there is improved robustness and security since the data structures are in read-only memory rather than created
on the stack. Secondly, there is a small performance improvement as the tables are fixed and only accessed if and when stack walking is
required. Thirdly, since the data structures are held in the binary itself, stack walking is reliable even if the PDB file is not present.
<p>
The instruction pointer is used to find the currently executing image and the offset into that image. This offset is then used to look up the
correct entry for the current function in the tables for the current module. The table entries contain, among other things, a description
of the stack frame for each function: in particular which register contains the base address and what the vital offset values are.
This information allows the stack walker to reliably walk up the list of stack frames to identify the calling functions and/or
find the correct exception handler without relying on data tables held in the stack itself.
<p>
While not quite as fast a simply chaining up a linked list of exception records or frame pointers the mechanism is fast enough to be
used at runtime. The Win32 API exposes a method, <code>CaptureStackBackTrace</code> that understands the data structures involved
and can be used by application programs to capture the address of the functions in the call stack.
<p>
There are further functions providing support for this stack walking: such as <code>RtlLookupFunctionEntry</code> which obtains
a pointer to the relevant data for a specific address; but I recommend that you use the supplied stack capture function: as while 
the data structures are (at least partly) documented making correct use of them is not for the faint hearted.
<p>
If you need to write 64-bit <i>assembler</i> code then you need to ensure that these tables are correctly built. There are a number
of restrictions as to the instructions that can be used in the function prolog and epilog; and additional assembler directives
must be written to ensure the assembler generates the correct data structures.
<p>
An even more complicated activity is when you need to generate executable code on the fly at runtime. The Win32 API provides 
a function <code>RtlAddFunctionTable</code> that you can use to dynamically add function table entries to the running module.
Unfortunately there do not seem to be any helper functions to facilitate building up the required data structures.
<p>
Additionally, it can be hard to verify that the data structures are in fact correct - the first indication that they are incorrect 
may occur when the system is trying to handle an exception as, if it is unable to correctly process the function table entry
for your dynamically created code this will almost certainly result in unexpected program termination.
<p>
However, since the same control structures are used for walking the call stack in the debugger as are used when
an exception is thrown, some checking can be done by verifying the call stack displays correctly in a debugger
as you step through the generated code.
<H2>
Dump busting
</H2>
While in general the data tables in the binary images do provide a very reliable way to walk the stack, the technique can fail when processing
a dump if the actual binaries are not accessible to the debugger that is reading the dump file.
<p>
As an example, consider a very simple function that throws an exception:<p>
<pre><code>
void func()
{
  throw 27;
}
</code></pre>
If we package this function in a DLL, and call this DLL from a main program, the exception handling logic walks up the chain from the
site of the exception (which is actually the <code>RaiseException</code> function inside kernelbase.dll) to the handler, if any, in the 
calling function. At the time of the exception all these tables are present in the executing binaries; but if a minidump is taken then
the code modules may well not be included in the dump. This will depend on which options are used when the minidump is created,
but space is often at a premium and so a complete memory dump may not be realistic.
<p>
Let us look at what happens when the resulting dump is processed on a <i>different</i> machine where not all the binaries
are present: in this example we have access to a copy of the <code>main.exe</code> program but not of the <code>function.dll</code>.
In the 32-bit world the absence of the binaries simply means the function names are missing, but the stack walk itself
can complete (I've disabled FPO in this example).
<p>
<b>The 32-bit exception in a debugger</b>
<ul><code>
<li> 	KERNELBASE!RaiseException+0x58
<li>	function.dll!_CxxThrowException(void * pExceptionObject, const _s__ThrowInfo * pThrowInfo) Line 152
<li> 	function.dll!func() Line 4
<li> 	main.exe!main() Line 12
<li> 	main.exe!__tmainCRTStartup() Line 241
<li> 	kernel32.dll!@BaseThreadInitThunk@12()
<li> 	ntdll.dll!___RtlUserThreadStart@8()
<li> 	ntdll.dll!__RtlUserThreadStart@8()</code>
</ul>
<b>The same exception, examined from a minidump on another machine</b>
<ul><code>
<li> 	KERNELBASE!RaiseException+0x58
<li> 	function+0x108b
<li> 	function+0x1029
<li> 	main.exe!main() Line 12
<li> 	main.exe!__tmainCRTStartup() Line 241
<li> 	kernel32.dll!@BaseThreadInitThunk@12()
<li> 	ntdll.dll!___RtlUserThreadStart@8()
<li> 	ntdll.dll!__RtlUserThreadStart@8()</code>
</ul>
<p>
If the same operations are performed with a 64-bit build of the same program the results when reading the dump are quite different.
<p>
<b>The 64-bit exception in a debugger</b>
<ul><code>
<li> 	KernelBase.dll!RaiseException()
<li>	function.dll!_CxxThrowException(void * pExceptionObject, const _s__ThrowInfo * pThrowInfo) Line 152
<li> 	function.dll!func() Line 4
<li> 	main.exe!main() Line 11
<li> 	main.exe!__tmainCRTStartup() Line 241
<li> 	kernel32.dll!BaseThreadInitThunk()
<li> 	ntdll.dll!RtlUserThreadStart()</code>
</ul>
(Note that debuggers sometimes display slightly different line numbers - the entry for <code>main.exe!main()</code> from which <code>func</code> was
called is shown as Line 11 in the 64-bit debugger (correctly) but as the <i>next</i> line, Line 12, in the 32-bit debugger.)
<p>
<b>The same exception, examined from a minidump on another machine</b>
<ul><code>
<li>	KERNELBASE!RaiseException+0x39
<li> 	function+0x1110
<li> 	function+0x2bd90
<li> 	0x2af870
<li> 	0x1
<li> 	function+0xf0
<li> 	0x00000001`e06d7363</code>
</ul>
The stack walk is unable to get before the first address inside <code>function.dll</code> in the absence of the function data tables.
(What the debugger seems to do when the data is missing is to simply try the next few possible entries on the stack but it is very 
rarely successful in finding the next stack frame.)
<p>
Note that providing the PDB files in this case does not help to walk the stack correctly, as it is the DLL and EXE files
that contain the stack walking data. It therefore becomes important to ensure that you can easily obtain the <b>exact</b> versions of
the binary files that the target machine is using if you wish to successfully process the smaller format minidump files.
<H2>
Conclusion
</H2>
The 64-bit Windows calling convention does seem to be an improvement on the 32-bit convention, although I personally wish that
a little more care had been taken to ensure that problem analysis was easier. As I have mentioned above, while the mechanism
does offer increased performance, it also decreases the likelihood of successful problem determination (especially with an optimised build.)
<p>
It is my hope that some understanding of how the 64-bit convention works will aid programmers as they migrate towards writing more 
64-bit programs for the Windows platform.

<H2>
Acknowledgements
</H2>

Many thanks to Lee Benfield, Dan Azzopardi and the Overload reviewers for their suggestions and corrections which have helped to improve this article.

<H2>
Useful references
</H2>
<table>
<tr>
  <td>Microsoft x64 Calling Convention</td>
  <td><A href="http://msdn.microsoft.com/en-us/library/9b372w95.aspx">http://msdn.microsoft.com/en-us/library/9b372w95.aspx</A></td>
</tr>
<tr>
  <td>Microsoft Macro Assembler Directives</td>
  <td><A href="http://msdn.microsoft.com/en-us/library/8t163bt0.aspx">http://msdn.microsoft.com/en-us/library/8t163bt0.aspx</A></td>
</tr>
<tr>
  <td>Register Usage</td>
  <td><A href="http://msdn.microsoft.com/en-us/library/9z1stfyw.aspx">http://msdn.microsoft.com/en-us/library/9z1stfyw.aspx</A></td>
</tr>
</table>

<HR>
Roger Orr <BR>
To be published in Overload <BR>
$Id: 64b-Windows-Calling-Conventions.html 188 2014-03-11 23:06:29Z Roger $

</BODY>
</HTML>
