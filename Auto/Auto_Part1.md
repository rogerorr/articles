# Auto - a necessary evil?
(This article is based on the presentation of that title at ACCU 2013)

"*To have a right to do a thing is not at all the same as to be right in doing it.*"  - G.K.Chesterton.

## Introduction
## 
The keyword `auto` has a new use in C++11 - although the suggestion has been under discussion for a while, as we shall see. 
It was one of the early proposals for addition to what was then called C++0x and, since it was both useful and (relatively) 
non controversial, some compilers added support for it well before the completion of C++11. This does have the advantage that 
it has had 'field testing' by a large number of programmers and so the form of the feature in the new International Standard 
seems to be pretty solid.

The keyword `auto` now lets you declare variables where the *compiler* provides the actual type and the programmer is 
either unwilling or unable to *name* the actual type. The keyword can also be used in function definitions to let you 
provide the return type *after* the rest of the function declaration, which is useful when the return type depends on 
the type of the arguments.

As with any new keyword there are questions about usage -- at two levels. First of all, where and how are programmers 
*permitted* to use the new feature. Secondly, what guidance is there to sensible *adoption* of the new feature. I intend 
to start by answering the first question and then subsequently focus on the second.

## A bit of history
## 
The word `auto` has been **re-purposed** in C++11 - it was inherited from C where it has been a keyword since the 
first days of "The C Programming Language" by Kernighan and Ritchie.

The old meaning of `auto` was defined as follows: "Local objects explicitly declared `auto` or `register` or not 
explicitly declared `static` or `extern` have automatic storage duration. The storage for these objects lasts until 
the block in which they are created exits."

This meant that the keyword essentially added **nothing** over an implicit declaration:

```
  {
    auto int i; // explicitly automatic
    int j;      // implicitly automatic
    // ...
  } // end of life for both i and j
```
and so in practice `auto` was almost never used in production code.

When Bjarne Stroustrup started working on C++ his Cfront compiler originally allowed `auto` to be used for variable 
declarations in a very similar way to that now in C++11: "The auto feature has the distinction to be the earliest to 
be suggested and implemented: I had it working in my Cfront implementation in early 1984, but was forced to take it 
out because of C compatibility problems" [auto].

Many years later there was a discussion on the C++ committee email reflector about the difficulty of declaring 
variables resulting from complex template expressions. David Abrahams wrote (in ext-4278, 26 Oct 2001): 
"...the expression results in a very complicated nested template type which is difficult for a user to write down". 
At the time the best suggestion was to write such variable declarations as something like:

`  typeof(<expression>) x = <expression>;
`
(`typeof` was an early name for what eventually became `decltype` in C++11) 

This however meant that the (potentially rather complex) expression had to be written **twice**, for example in this 
simple case:

`  typeof(alpha*(u-v)*transpose(w)) x = alpha*(u-v)*transpose(w);`

which made the code harder to read - and was also a good source of bugs if and when the expression was changed.

He suggested this form of declaration could be replaced with something like:

`  template <class T> T x = <expression>;`

The C++ template argument deduction rules could then come into play to work out the actual compile-time type of '`x`'.

In the subsequent discussion Andy Koenig wrote: "I would also like to see something like

`  auto x = <expression>;`

I know we can't use `auto`, but you get the idea."

However, various people picked up on his, probably throwaway, suggestion and the idea gained momentum. Of course, a big 
concern was whether this change of use for the `auto` keyword would break a lot of code; the standards committee is 
understandably very reluctant to break existing valid code. A number of people spent some time searching internal company 
code bases they had access to and also using the now defunct Google Code Search. Daveed Vandevoorde reported that "Google 
Code Search finds less than 50 uses of `auto` in C++ code."

It turned out that most existing uses of `auto` were in test code (verifying that compilers, parsers or other tools 
handling C++ code correctly processed the keyword) and that a number of the remaining uses were in fact incorrect! 
The research gave the committee confidence that repurposing the keyword would not be a major problem. This confidence 
seems to have been well-founded.

The first formal paper for C++0x was N1478 (Apr 2003) [N1478]. The emphasis of this paper was in providing ways to make 
*generic* programming easier - the draft of this proposal (ext-5364) begins: "Proposal for "auto" and "typeof" 
to simplify the writing of templates".

The paper also proposed another new keyword, `fun`, which was used for declaring function return types. Over time this 
was replaced by an overloaded use for `auto` (and jokes about how we lost the fun.) I do sometimes wonder whether 
`auto` is in danger of gaining multiple meanings in the same way that the keyword `static` has!

It is worth keeping this history in mind when looking at the use of `auto` as it might help distinguish the two main 
uses (one for variables and one for functions). It is also instructive to compare the original target design space -- 
templates -- with the range of uses finally allowed. It isn't the first time that a feature in C++ has had its use 
broadened well beyond the original expectations.

## So what did we end up with?

`auto` is repurposed and can be used in a variety of ways, such as:

- a placeholder for the type in a simple variable declaration:
  ```
  auto x = 5; // 'auto' is here equivalent to 'int'
  ```

- to declare a variable referring to a lambda:
  ```
  auto lambda1 = [](int i){ return i * i; };
  ```

- in a `new` expression:
  ```
  new auto(1.0); // ` 'auto' is here equivalent to 'double'
  ```
- in function declarations (and definitions) allowing the return type to be specified at the end:
  ```
  auto f()->int(*)[4]; 
  ```

- in function template declarations:
  ```
  template <class T, class U>
   auto add(T t, U u) -> decltype(t + u);
   ```

where this is considerably simpler than the equivalent *without* `auto`:

  ```
  template <class T, class U>
  decltype((*(T*)0) + (*(U*)0)) add(T t, U u);
  ```

In each case `auto` is a place holder for a *specific* compile time type - this type is 'baked in' 
by the compiler. This is worth highlighting, especially for those used to languages with dynamic types; 
there is no runtime overhead in using `auto`. Also note that the use of `auto` does not change the *meaning* 
of the code - it means exactly the same as the equivalent code with the deduced type written in full.

Once formally adopted into the working paper `auto` became available for use in several compilers. Scott 
Meyer's list [Meyer] of C++11 support shows `auto` was available in

   - gcc 4.4 (formal release Apr '09)
   - MSVC 10 (formal release Apr '10)

and the examples given above all do compile successfully with both gcc and MSVC.

As the wording for `auto` was being polished for inclusion in C++11 (and as additional papers were written 
adding further new features to the language) there was a keen interest in avoiding any "special cases" for 
`auto`. The committee followed the general principle of trying to make use of `auto` orthogonal to other 
choices: so for example `auto` for function return types is not restricted to function templates but can 
also be used for non-template functions.

## Interactions with other items

### r-value references

One of the new items added to C++11 was r-value references (designated with &&). As many of you will already 
be well aware this was principally added to support 'move semantics' which enables significant performance 
improvements when copying data out of temporary objects.

```
  auto var1 = <expression>;
  auto & var2 = <expression>;
  auto && var3 = <expression>;
```
These are all valid (subject to constraints on the actual expression).
Note though the last in particular may not do *quite* what you expect, I will say more about this in the 
second article. (Scott Meyers covered this in his article on "Universal References in C++" [Universal].)

### Lambda

The addition of lambda expressions to C++ was one of the motivating cases for `auto`. Passing a lambda to a 
function template works easily - for example:

```
  template <typename T> void invoke(T t); 
  ...
  invoke([](int i){ return i; });
  ```

The call to `invoke` passes a (trivial in this example) lambda that takes an int and returns it. The compiler 
deals with instantiation of the correct template and so the programmer neither knows nor cares what the actual 
type of the lambda is.

But what if you want to hold the lambda in a variable?

```
  <type> square = [](int i){ return i * i; }; 
  int j = square(7);
```
The $64,000 question is: "What should replace `<type>` ?" The answer is '`auto`'.

### NSDMI (non-static data member initialisers)

In C++11 values can be provided for non-static data members that will be used to provide the initial value 
(unless one is supplied in the initialisation list of the constructor.)
For example:

```
  class x {
    int i = 128;
    double d = 2.71828;
  };
```
Could you instead write:

```
  class x {
    auto i = 128;
    auto d = 2.71828;
  };
```
Short answer: no. This was rejected ... see "Where *can't* you use it?" below for a bit more detail about the 
reasons for this.

**Range-based for

**C++11 added syntactic sugar to support simple syntax for iteration over containers, for example:

```
  for (std::string x : container) {
    // do something with 'x'
  }
```
which is a simpler and safer way to write:
```
  for (std::vector<std::string>::const_iterator it =
       container.begin(); it != container.end(); ++it) {
    std::string x = *it;
    // do something with 'x'
  }
```
The `auto` keyword is allowed in this context too, so you can write:

```
  for (auto x : container) {
    ...
  }
```
and the compiler will deduce the correct type for `x` to match the elements in the container.

The use of references and const allows more control over whether the loop variable is a value or a reference 
and whether or not it is constant:

```
for (auto & x : container) {
   x += ...
}
```
Or

```
for (auto const & x : container) {
   ...
}
```
(Note that in the first example the type of `x` is already a `const` reference if the container is `const`.)

## Specification note

You may or may not care that range-based for is actually specified in terms of `auto`:

```
{
  auto && __range = range-init;
  for ( auto __begin = begin-expr,
             __end = end-expr;
        __begin != __end;
        ++__begin ) {
    for-range-declaration = *__begin;
    statement
  }
}
```
### The `decltype` keyword

The keyword `decltype` obtains the type of an expression. In C++03 there was no easy way to do this and various 
tricks were invented to provide various derived types - for example by using nested `typedef`s or associated 
traits classes. While `auto` allows you to declare a variable of the same type as an expression `decltype` provides 
a more general technique. For example, declaring a variable without an initial value:

```
  std::vector<int> vec;
  decltype(vec.begin()) iter;
```
There are some subtle differences declaring a variable with `decltype` and with `auto`, which I will touch on later.

## Where *must* you use it?
 
The basic principle behind `auto` is that the compiler knows the type - but you either can't describe it 
or don't want to. There is one primary use-case where you *cannot* name the type - with lambdas. Lambdas are most
 often used as arguments to other functions. However, if you want one as a local variable, the standard states
 (5.1.2p3) that the type of the lambda-expression "is a *unique*, *unnamed* nonunion class type - called
 the closure type" (my italics)

What this means is you the programmer **cannot** name the type (as the type is unnamed), nor can you even use 
`decltype` to declare a variable to hold the lambda (as the type is unique so the type in the decltype *won't* 
match the actual type of the expression).

Side note:
A small number of types in the standard are specified as *unspecified* so you cannot name them portably. `auto` 
gives you a way to create variables of those types; however this is almost never a genuine problem as the number 
of use cases when you genuinely need to do this is vanishingly small!

## What *is* the actual type of a lambda variable?

Here is a simple example of a variable holding a lambda.

```
int main()
{
   auto **sum** = [] (int x, int y)
   { return x + y; }; 

   int i(1);
   int j(2);
   // ...
   std::cout << i << "+" << j << "=" 
     << **sum**(i, j) << std::endl;
}
```
In this contrived example the lambda is created and assigned to `sum` at the start of `main` and then invoked at the 
end of main in the output operation. But, if we are curious, we may be wondering what actually *is* the type of the 
variable holding the lambda.

We cannot *name* it in our code, but we are allowed to perform some other operations on the type.

We may for instance try to get some information by using `typeinfo`, for example with: `typeid(sum).name()`
The actual output is implementation specified, I obtain this with MSVC:

  `class <lambda_8f4bf0680d354484748e55d11883b00a>`

and this with gcc:

  `Z4mainEUliiE_` (this name demangles to `main::{lambda(int, int)#1}`)

This gives some hint about possible implementation strategies in each case, but obviously code like this is of very 
limited practical utility.

## An alternative solution

Very commonly of course we are not interested in the *precise* type of the variable but more in what we can *do* 
with it.  We could then make use of the C++ `function` class to hold the variable:

`   std::function<int(int, int)> **sum** = [](int i, int j) ...`
This technique employs *type erasure* behind the scenes - the actual lambda type is hidden inside the std::function 
object at the cost of a small runtime penalty. (`auto` avoids this penalty.)

This looks very similar to the following C# code:

`Func<int, int, int> sum = (int x, int y) => { ... }`

## Are lambdas the only place to use auto?

Declaring variables to hold lambda expressions is, I believe, the only time `auto` is **mandatory** in your code. 
However most people recommend you use `auto` in (at least some of) the cases where giving the name of type yourself 
is a valid option.

Herb Sutter, for example, wrote: "For example, virtually every five-line modern C++ code example will say "auto" 
somewhere." [Herb]

As the quotation from G.K.Chesterton implies, being *allowed* to use `auto` does not mean this is always the right 
thing to do. In the subsequent article I will look at some of the forces involved in deciding when to use (and when 
not to use) the `auto` keyword.

## Where *can't* you use it?

In C++11 you *cannot* use `auto`:

- As the type of lambda **arguments**:

`   auto sum = [] (auto x, auto y) // not legal in C++11
     { /*...*/ }
`
This however was voted into the next release - C++14 - at this April's WG21 meeting; and is already in some recent 
versions of gcc.
What this generates is a lambda which can take different argument types - a sort of "lambda template". This has 
been named "polymorphic lambda" and you may well have heard some of the discussion about this feature, which is 
one of the most common requests people make for extensions to lambda.

- To declare function return types without a trailing-return-type declaration

`   auto func() { return 42; } // not legal in C++11
`
This also will was voted into C++14 - compilers will be able to deduce the return type of `func()` from the type 
of the returned expression (or expressions, if they are of equivalent type).

- To declare member data

```
   class X {
    auto field = 42; // error
   // ...
  };`
```
As mentioned earlier, this idea was floated during the discussions about `auto` for C++11, but there were concerns 
over whether this change might make the parsing of class definitions too complex and also over violations of the 
ODR (one definition rule) if the type of the initialisation expression was *different* in two different translation 
units.

Discussion on supporting this one has resurfaced recently and it is possible there will be a proposal to add it 
to the language.
I note that C#, where the `var` keyword has much the same purpose as `auto` for C++, also disallows fields being 
declared with `var`. Perhaps this common choice indicates some deeper problems with what at first sight seems 
to be a relatively straightforward extension.

- To declare function arguments

`   void foo(auto i) { /*...*/ } // error
`
The idea here is that this declares a function `foo` that behaves like a template and instantiates itself according 
to the type of argument provided - the code above would be effectively equivalent to:

`   template <typename __T1>
   void foo(__T1 i) { /* ... */ }`

However, we *already* have function templates to do this job, and the use of explicitly named template arguments 
rather than `auto` allows you to express constraints between the arguments types more easily. However, there is 
some interest in supporting this syntax and so it may possibly be standardised at some time in the future, but 
is not currently in scope. It may be introduced as part of the 'concepts lite' development that is being formalised 
as a Technical Specification since this may provide the vocabulary to express constraints between the arguments.

## Conclusion

C++11 contains a number of new features, some of which are somewhat complicated or obscure. The `auto` keyword 
though seems to be relatively safe and easy to use and allows complicated variable declarations to be greatly 
simplified. When used in conjunction with the range-based for loop the resultant code, to my mind at least, 
expresses intent much more clearly than the equivalent C++03 code and with very few downsides.

However the use of `auto` is not always so cut and dried -- and there are also some subtle interactions with 
const and r-value references. In the next article I will explore in more detail when you might wish to use 
`auto` and when you might prefer *not* to use it (and why). I will also cover some of the cases where `auto` 
produces different behaviour from what you might expect.

## Acknowledgements

Many thanks to Christof Meerwald, Irfan Butt, Sam Saariste, Rai Sarich and the Overload reviewers for their 
suggestions and corrections which have helped to improve this article.

## References

[auto]      http://www.stroustrup.com/C++11FAQ.html#auto

[N1478]    http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2003/n1478.pdf

[Meyer]    http://www.aristeia.com/C++11/C++11FeatureAvailability.htm

[Universal]    "Universal References in C++", Scott Meyers (Overload 111)

[Herb]      http://herbsutter.com/elements-of-modern-c-style/

<hr>
Copyright (c) Roger Orr (first published in Overload, Jun 2013.)
 