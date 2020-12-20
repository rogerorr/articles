# CTAD - what is this new acronym all about?
Roger Orr

## Introduction

C++17 has now been shipped and the dust is settling. There are a number of new features in the language; one of the last to be added before the final cut goes by the snappy acronym of "CTAD". the full name is "Class Template Argument Deduction" which may not tell you a great deal more than the acronym does.

Example with `std::pair`

When using class templates you've always had to provide the template arguments even when their type was obvious from the use:

C++98 code
```c++
void test(int id, std::string const &name)
{
   std::pair<int, std::string> p(id, name);
   // ...
}
```
The template arguments have to be provided although it's pretty clear what they are.

Things changed slightly with the introduction of `auto` in C++11; it became possible to use the (pre-existing) helper function make_pair to create the variable and so avoid duplication of the types:

C++11 code
```c++
void test(int id, std::string const &name)
{
   auto p{std::make_pair(id, name)};
   // ...
}
```
However, this relies on the existence of the `make_pair` function template so if you wished to provide a similar facility for your own class template you had to ensure a helper function was available. It was simply an idiom to enable using the language rules which do allow template arguments to be deduced when calling function templates.
Class template argument deduction allows us to avoid duplicating type names even when using the constructor syntax:

C++17 code using CTAD
```c++
void test(int id, std::string const &name)
{
   std::pair p(id, name);
   // ...
}
```
The compiler detects that pair names a class template but no template arguments are supplied and deduces the arguments from the types used in the call of the constructor. (Hence the name of class template argument deduction.)

Use of CTAD makes the class type of the variable p explicit. It removes the need to define a helper function such as make_pair - and it is a better technique as the use of a helper function relies on a naming convention to specify the type to the reader of the code.

As the paper containing the wording [P0091R3](http://wg21.link/p0091r3) put it in the summary: 'If constructors could deduce their template arguments "like we expect from other functions and methods," ...' This is pretty much what CTAD does.

Since CTAD is now a standard language feature it is available for any existing class templates without any additional changes.
```c++
// Existing C++ class
template <typename T>
struct point
{
   T x;
   T y;
};

int main()
{
   // New C++17 use
   point pt{0L,0L};
}
```
Potential problems

First of all note that the addition of CTAD to C++17 does not break any existing code as it simply allows some formerly ill-formed code to become valid.
However, before you race to your code-base and remove all explicit specification of class template arguments on variable declarations there are a few corner cases that might be troublesome.

First of all, the deduction process will only work if there are constructors for the target type that use the template arguments; the process cannot magically guess the type from the arguments. So, for example, CTAD is of no use for the following class as the template argument is not part of the constructor signature:
```c++
template <typename T>
class collection
{
public:
  collection(std::size_t size);
  // ...
};
```
It can also be a problem when the constructor you desire to invoke uses types derived from the template arguments as there is no way by default to work 'backwards' to deduce the underlying template argument.
For example, if you wish to construct a vector from a pair of iterators and hence invoke the constructor:
```c++
template<class InputIterator>
vector(InputIterator first, InputIterator last, const Allocator& = Allocator());
```
Doing this directly is problematic as the template argument type for the target collection is implicit in the `value_type` of the supplied iterators. (We'll see below one work around.)

Secondly, if there are several constructors, the constructor you want may not be the one that the deduction will find. This may be a problem if you are trying to use CTAD with classes that were written before C++17 as designing in usable constructors will not have been necessary then. Even small details of the way classes are written can render CTAD inoperable. 

Thirdly, in some cases, you do want to use a helper function to create instances of the class for other reasons. One obvious example from the standard library is `std::make_shared`. As Scott Meyer's "Effective Modern C++" puts it in Item 21: "Prefer `std::make_unique` and `std::make_shared` to direct use of `new`."
Consider the following example:
```c++
#include <memory>
int main()
{
   std::shared_ptr<int> p(new int(10));
   auto q{std::make_shared<int>(10)};
   std::shared_ptr r(new int(10)); // C++17
}
```
The construction of `p` and `r` differ in whether the type is explicit or deduced, but in both cases the shared pointer will need to allocate an additional piece of data to manage the shared object. In the construction of `q` the target object and the associated management structure can be created using a single allocation.

## Helping choose the right constructor

The wording for class template argument deduction includes the option of providing explicit deduction guides.

The syntax is similar to that of a function template, except as the declaration is for a constructor-like entity there is no return type.

For example, as we saw above for the case of a vector, the language will not deduce a template argument for the vector constructor taking a pair of iterators. The following deduction guide was added to the standard library [P0433R2](http://wg21.link/p0433r2):
```c++
template<class InputIterator, class Allocator
 = allocator<typename iterator_traits<InputIterator>::value_type>>
vector(InputIterator, InputIterator, Allocator = Allocator())
-> vector<typename iterator_traits<InputIterator>::value_type, 
     Allocator>;
```
Omitting the (defaulted) allocator argument for ease of understanding we get:
```c++
template<class InputIterator>
vector(InputIterator, InputIterator)
-> vector<typename iterator_traits<InputIterator>::value_type>;
```
This instructs the compiler when it sees a constructor call taking a pair of iterators, to construct a vector using the `value_type` of the iterator type, such as:
```c++
void foo(std::set<int> const &c)
{
   std::vector v(c.begin(), c.end());
   // ...
```
and `v` will be deduced as `std::vector<int>`, as expected.

(Note that this is more restrictive than the range of options available using the full syntax, as in this case the vector to be constructed can, in general, be of any type that `int` can be converted to.)

## Helping prevent inappropriate choices

There are times when you may with to _prevent_ use of CTAD for a class, for example when the constructor chosen is unlikely to be the one that the user expected.

One way to remove the possibility of using class template argument deduction is to change the constructor to take a type derived from the template arguments:
```c++
template <typename T>
struct no_ctad { using type_t = T; };

template <typename T>
using no_ctad_t = typename no_ctad<T>::type_t;

template <typename T>
class test
{
public:
  test(no_ctad_t<T>);
};

int main()
{
   test t(1); // Error
}
```
The compiler is not allowed to 'work upstream' and deduce a possible value of `T` (i.e. `int`) that would make `no_ctad_t<T>` match the supplied argument. This is the same rule that already applies to regular template argument deduction on function calls.

(Note: Timur Doumler is proposing standardising a class similar to `no_ctad` - this use case is one of the motivating examples)

A second way, detailed in [P0091R4](http://wg21.link/p0091r4), would be to extend the usage of deleted functions ("=delete") to also allow for deleted deduction guides. This has not yet been adopted into the working paper, but approval for this direction has been given by the Evolution working group.

## Primary template and explicit specialisations

Class template argument deduction applies to the primary template. If an explicit specialization of this template defines different constructors these will not be found by CTAD.

For example:
```c++
template <typename T>
class myclass {};

template <>
class myclass<int>
{
public:
   myclass(int);
   // ...
};

int main()
{
   myclass m(1); // Error: primary template only has a default ctor
}
```

## Future directions

When CTAD was first proposed it was suggested that you could provide some template arguments and deduce the others. This is not currently in the working paper as it was felt safest to start with an 'all or nothing' approach where there are fewer opportunities for confusion or ambiguity and to consider a future extension if one is provided.

## Compiler support

CTAD is part of C++17 and so will eventually be provided by any compiler offering support for the current C++ standard.
However, at the time of writing (end of 2017) not all the mainstream compilers have yet released versions implementing this part of the language.

So for example while both gcc 7.1 and clang 5.0 support the feature, the latest version of MSVC at the time of writing did not [C++17 progress](https://blogs.msdn.microsoft.com/vcblog/2017/12/19/c17-progress-in-vs-2017-15-5-and-15-6/), nor does it appear that Intel's compiler does. The status may of course have changed by the time you read this article.

So your ability to make use of class template argument deduction in your code will depend on which compilers -- and which version of those compilers -- your project is targetting.

## Summary

Class template argument deduction does not let you write any new code that you couldn't already write, albeit with slightly more syntax.
However, the reduction in syntax does reduce the burden for the reader of the code and also ensures that, since the target types are deduced, the code automatically changes if the type of the supplied arguments change.
I consider CTAD a useful technique for reducing cognitive overhead and improving readability.

<hr>
Copyright (c) Roger Orr 2018-01-13 18:22:09Z
First published in Overload, 26(143), Feb 2018
