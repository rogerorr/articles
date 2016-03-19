# One Definition Rule
Roger Orr

## Introduction

C++ allows a lot of flexibility over the physical arrangement of source code. Almost all C++ programmers have a concept of "source file" and "header file" and some views about what should be placed into each file. What does this mean in practice?

There is no single simple answer in modern C++.

Traditionally people have put function declarations, class definitions, and constants into header files and placed function and member function implementations into source files. However, the use of templates and the presence of inline functions, where the full definitions are normally required whenever they are referenced, has blurred the distinction between headers and source files.

Conversely, many tool chains now offer some sort of 'whole program' optimisation which means that the linker has an overview of all the source code in the program and again this reduces the clear distinction between separate source files since cross-source optimization is now possible.

The "One Definition Rule" ("ODR") is attempting to ensure that there is at most one definition of the various entities (classes, functions, etc.) in each **source** file and also ensure that there is a single unambiguous definition in the resulting whole **program**. While both of these are covered by the ODR the first case is much easier for the compiler (and the programmer) to detect than the other. It's the second case, that of the same entity being defined differently in two different components of the same program, that can cause very hard to diagnose problems.

The language standard itself speaks in terms of 'translation units' by which it means a source file, together with all the lines of code *in* headers and other source files `#include`d by it, excluding any lines skipped by any of the conditional inclusion preprocessing symbols. This *textual* inclusion model means that the same lines of text from the same ("header") file can be included in multiple translation units ("TUs") but each is compiled separately and the context for compilation will be provided by the previous lines of text, the preprocessing symbols defined, and any additional switches provided to the compiler.

This is a very different model from many languages where inclusion is a reference to the *compiled* output from another source file. In these cases there is no ambiguity about where things are defined nor about what the context was for their compilation. While it is still possible to cause problems, for example by using a different version of the included file during compilation from that used at runtime, the problems are greatly reduced.

The ODR refers to the "same sequence of tokens" which is not necessarily equivalent to "from the same (header) file" and there are additional constraints basically trying to ensure that if the same thing is defined in two different translation units the two different compilation contexts have not affected the meaning of the entity being defined.

## Demonstration of the problem

Source file 1
```c++
  struct Data
  {
    int id;
    std::string first;
    std::string second;
    std::string last;
  };

  extern Data getData(int employeeId)
  {
    // Implementation details omitted...
  }
```
Source file 2
```c++
  struct Data
  {
    int id;
    std::string first;
    std::string last;
  };

  extern Data getData(int employeeId);

  int main()
  {
    int id;
    std::cout << "Enter employee Id: ";
    std::cin >> id;
    Data const details( getData(id) );
    std::cout << "employee " << id << " is " << details.first << std::endl;
  }
```
Here we can see the same named structure, `Data` , is used in both source files but with a different number of fields in the two cases. The behaviour of the resultant program if source file 1 and 2 are linked together will be **undefined** .

I tried two different compilers and in both cases I got output something like this:
```
Enter employee Id: 1
employee 17370356 is Roger
```
While the actual number printed varied in neither case did I get any warnings nor any crashes, but the employee Id printed out *wasn't* the value that was supplied.

With a third compiler I got this:
```
Enter employee Id: 1
employee 1 is Roger
```
followed by a crash of the program.

What has occurred in the first two cases is that call to `getData` **corrupted** the value held in `id` . This is because the data structure `details` has enough space for two strings, but the function in source file 1 is populating the structure with three strings, and this is overwriting the id value as it just happens to be the next address in memory beyond the end of the `details` structure.

In the last case it seems that the corruption is still occurring, but the memory layout in this case is different and the corrupted value is not `id` but something else, such as the return address, causing the crash when `main` ends.

In this example it is easy to see what the problem is; in a real-world example it can be extremely hard to detect the problem.

## How can we prevent this happening?

The problem above was two different definitions of the same data type. While ODR violations affect many different entities in C++ one of the most troublesome is when the layout of a class changes as the consequences of this can be quite unexpected and hard to isolate. ODR violations with other entities still result in undefined behaviour, but in my experience the actual consequences at runtime tend to be easier to understand.

We can reduce the likelihood of the definitions for the data type differing by placing *one* definition inside a single file and including this header file in both the source files.

That is fairly obvious; but it is hard to enforce. It is quite common to have multiple **copies** of the same header file in the source tree for a large project - perhaps caused by laziness or short-term desire for "simplicity". Unfortunately, as we all know, once you have multiple copies they tend to diverge and then you have *different* definitions in the various files.
One common cause of this problem is when two different versions of a 3rd party library are used in the same project - perhaps the project includes a library that was implemented with one version of Boost and the project code accidentally uses a different version.

It is also common for people to create various helper classes and functions for use in the *implementation* of a class. These entities are defined in the source file itself and are not visible outside it. However, if another source file happens to define the an entity with the same name then we have an ODR violation. While many of these are benign (as the code is localised and often gets completely inlined) the other cases can cause nasty bugs.

Placing helper classes and functions into the anonymous namespace avoids ODR violations like these and should be encouraged. In my experience people are becoming fairly good at putting helper functions into an anonymous namespace but for some reason less commonly place helper classes into a namespace.

However, even with a class defined in a single header file in one location there are still a number of ways that differences can creep in.

In my experience the commonest problems are caused by:
1. preprocessor symbols
2. packing
3. compiler flags

## Different preprocessor symbols.

It is common to see header files set up to support a number of different compilation modes. For example, some data structures may need locking with a mutex in a multi-threaded program but do not need this protection a single-threaded one. One common implementation technique is to conditionally define the mutex, and the associated code using it, based on a preprocessor symbol that the user of the header file defines if required.
A sample data structure might look like this:
```c++
  class Data
  {
  public:
    // Methods ...
  private:
    int id;
    std::string first;
    std::string second;
    std::string last;
  #ifdef MULTI_THREADED
    std::mutex mutex;
  #endif;
  };
```
The danger here is that if *some* of the source files in the program that include this header file have `MULTI_THREADED` defined and *some* do not then we have an ODR violation because of the different number of fields and the resulting program may not behave as we would like.
However, as in the example I started with, the actual consequences of the undefined behaviour are very hard to predict and are likely vary between compilers. 

## Different packing

Compilers align fields on a 'natural' address to fit in with the addressing modes of the underlying architecture. So for example, on a 64bit platform the first couple of fields in the data structure above might be laid out as:
```
  int id; // offset 0, length 4
  std::string first; // offset 8, length 32
```
However, many compilers allow the programmer to use a pragma or attribute to override this natural alignment and to pack the fields as close together as possible. This typically results in a reduction in memory and may also result in a reduction in performance, depending on a number of factors. Packing is also used when ensuring a C++ data structure matches some extenally defined data structure, perhaps from a serialisation format or network protocol.

The way to do this in Microsoft Visual C++ is to use the `#pragma pack` directive, for example:
```
  #pragma pack(2) // 2 byte aligned
  #include "data.h"
  #pragma pack() // back to the default
```
with these packing instructions the fields might now be laid out as:
```
  int id; // offset 0, length 4
  std::string first; // offset 4, length 32
```
However, mostly to provide compatability with MSVC, other compilers such as gcc also support the same pragma and it is quite common to see the pragam used even in code used cross-platform.

Once again, as long as *every* source file in the program uses the same packing value we are fine, but if just one file does not set the same packing then we cause an ODR violation.

This problem is much less likely to occur with the idiomatic style provided by gcc which uses attributes attached to the *specific* data structure(s) they are applied to.
```
  class Data \{
    // ...
  } <b>__attribute__ ((packed))<b> ;
```
One further trouble with the example shown above of using `#pragma pack` is that we don't know what *other* header files might be included by the line `#include "data.h"` . If, as is quite likely, `data.h` in turn `#include` s `<string>` then by setting the packing while including `data.h` we may have inadvertently caused the standard string class to be packed too.

I would recommended if at all possible that you ensure that `#pragma pack` directives are very carefully scoped to ensure that no header files are `#include` d within the scope of the directive.

## Different compiler flags

Depending on the compiler being used there may be various flags which can be set during compilation which change the layout of the class. Often these flags are simply another way of generating a different set of preprocessor symbols, or different packing, but there are some other cases too - for example specifying the size of the `long double` type or whether the `char` type is `signed` or `unsigned`.

It is important to ensure the set of flags used in building a single program are consistent where the use of different settings could result in ODR violations. 

This relates to build system discipline where it is advisable to set program-wide settings at the top level of the build system rather than individually specifying them for each component or even for each file.

## And more...

An additional problem is when the meaning of **types** change between source files.
For example, a structure is defined with a field `char buffer[BUFSIZE];` but the value of `BUFSIZE` varies between source files.
More subtle problems can occur when, for instance, a type defined in one namespace masks a type defined in an outer namespace so a different type is used for the field in two different files.

## Can the compiler help?

If the ODR violation occurs inside a single translation unit then we do expect, rightly, that the compiler will error on ODR violations. The standard is quite clear: "No translation unit shall contain more than one definition of any variable, function, class type, enumeration type, or template."

In the cases of ODR violation between TUs it is much harder to see how the compiler -- which compiles each TU independently -- can assist us. However, in some cases the use of name mangling can help, for example to avoid linking functions with mismatched calling conventions together.

## -Wodr to the rescue

Recent versions of gcc support link-time optimisation ("lto") with the `-flto` flag which allows the linker access to the internal bytecode generated by the separate compilations. This allows the linker to perform additional optimisations, such as inlining a function call from one source file to another.

The mechanism has been slightly extended in gcc 5.x to include support for checking for some of the ODR violations covered above for data types; the byte code generated by each source file compiled with lto support will contain additional information about the data structures involved and this can be checked at link time for consistency.

If I compile on the example I started with using gcc 5.2 and `-flto` I get the following error at **link** time:
```bash
  $ g++ -Wall -Wextra **-flto** -o SourceFile1.o SourceFile1.cpp -c
  $ g++ -Wall -Wextra **-flto** -o SourceFile2.o SourceFile2.cpp -c
  $ g++ -o Program SourceFile1.o SourceFile2.o
  SourceFile1.cpp:2:8: warning: type 'struct Data' violates one definition rule [-Wodr]
    struct Data
           ^
  SourceFile2.cpp:4:8: note: a different type is defined in another translation unit
    struct Data
           ^
  SourceFile1.cpp:5:17: note: the first difference of corresponding definitions is field 'second'
       std::string second;
                   ^
  SourceFile2.cpp:7:16: note: a field with different name is defined in another translation unit
      std::string last;
                  ^
```
(Note that for -Wodr to work **both** the .o files with the differing type definitions must be compiled with -flto.)

This is extremely useful and has helped me to locate several places in a large code base where there were previously unidentified ODR violations, at least one of which had led to large memory leaks in the past which had been worked round without actually finding the root cause of the problem.

## Modules

As many of the readers may already know, there is active work towards a 'modules' system for C++ which among other things should make it easier to avoid ODR violations, especially with internal 'implementation only' entities used purely inside the library. However, a lot will depend both on the final form of the proposals and also on the details of the implementation strategies adopted by the compiler or compilers you use.

# Summary

ODR violations can be very hard to spot. Good programming discipline in

- naming things
- avoiding copy-paste (see the DRY principle)
- using namespaces
- using anonymous namespaces in implementation files
- build system settings

all help to reduce the likelihood of experiencing ODR violations.

The gcc `-Wodr` detection included in link-time optimisation is highly recommended (at least in gcc 5.2 and above - there did appear to be some occasional problems in 5.1) and it might be worth using this as a static analysis tool even if the primary build doesn't use link time optimisation (or even uses a different compiler).

---
Copyright (c) Roger Orr 2015-10-04 14:40:59
 