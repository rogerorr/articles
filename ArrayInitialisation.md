# How many braces must a programmer write down?

## Introduction

In a recent (2021-05-27) posting to accu-general Andrew Marlow asked a good question:


<blockquote>
I am trying to learn about <code>std::array</code> and am struggling to find any example other than <code>std:array&lt;int, n></code>.
I have my own type and am trying to initialise an array where I declare it.  My type is a POD, it just has
a few `const char*` members and an integer. I don't mind if I have to define a ctor to give them all values
from ctor args but I was hoping I could use brace initialisation, like one would if one using an old style C array. I have:

```
std::array<MyType, 10> mine =
{
   { member values for first object },
        :
   {member values for last object }
};
```
This fails to compile. It says that there are too many initialisers. It says that no matter what value I put in for N.
I am stuck. Can someone help please?
</blockquote>

Jonathan Wakely (the first reply among others) provided a solution: add *another* pair of braces:

```
std::array<MyType, 10> mine =
{{
    { member values for first object },
        :
    {member values for last object }
}};

```
This is a common question and, as pointed out in the subsequent email discussion, it is not helped by the fact that the
commonly found online references for C++ seem to only show examples of an array of scalar values, typically `int`.
In this simpler case the naive initialisation of int values "just works" **without** needing two sets of braces.
So *why* is there this difference between a simple array of ints and an array of a user defined type?

Let us look at what is actually going on with initialising `std::array` and how it works with both an array of
`int` and the more complicated example of an array of a user-defined type.

As Andrew remarks in his initial email, he had hoped to use brace initialisation, as for a C-style array.
While `std::array` looks a lot like a C-style array it is not a replica -- and the difference is the root cause
of the need for the extra braces.

## `std::array` *wraps* an array

The first thing to note is that `std::array` is not a C-style array, it is a *wrapper* for a C-style array.
It is a template class and therefore only available in C++. However, to demonstrate the issues with aggregate
initialisation - even in C - we can emulate *instantiations* of `std::array` in plain C by creating a struct
with an array as a member. In fact, almost all of the issues of initialising `std::array` using aggregate
initialisation can be demonstrated using plain C structs as the presence of additional features in C++
(brace-init-lists and constructors, for example) doesn't materially affect the problem. Of course, we also
don't have templates in C, so I have arbitrarily picked an array of four integers and an array of UDTs as
my example types:

```
typedef struct udt { int i; int j; } udt;

typedef struct int_array {   int data[4]; } int_array; // like std::array<int,4>

typedef struct udt_array {   udt data[4]; } udt_array; // like std::array<udt,4>
```

Now we can examine aggregate initialisation "in plain sight".

## The most verbose syntax 

If we want to spell things out completely unambiguously then the full syntax is to use a pair of braces
for **each** level of nesting in the structure, until we reach the simple values applied to each scalar member.
So for the integer array we would, in full, use **two** braces as the scalar values are two levels down
(they are in the `data` member of the surrounding `int_array` struct):

```
int_array ia = { { 1, 2, 3, 4 } };
   'int_array' ^
        'data'   ^
         '[0]'     ^
```

For the UDT array we would, in full, use **three** levels of braces as the scalar values ('i' and 'j') are
members of the `udt` objects which are in turn inside the `data` member of the surrounding `udt_array` struct:

```
udt_array ua = { { { 1, 2 }, {3, 4}, {5, 6}, {7, 8} } };
   'udt_array' ^
        'data'   ^
         'udt'     ^
         'i'         ^
```

If we try to add **more** braces then the output gets noisier (with gcc and clang) or fails (with MSVC).
This is just the same with simple initialisation of an `int`, for example:

```
int main(void) {
  int i = 0; // Ok
  int j = {0}; // Ok
  int k = {{0}}; // warns with gcc/clang, errors with MSVC
  return i + j + k;
}
```
However, we are allowed to provide **fewer** braces, provided the initialisation that results is still valid.
When fewer braces are supplied than required initialisation simply populates the fields visible at the current
level in the target in order until it runs out of initialisers. So for example we could have something like this:

`typedef struct another_type {  int i; udt u; double d;} another_type;`

And initialise this either using the 'full' depth of braces:

```
another_type at = { 1, { 2, 3 }, 4 };
   'another_type' ^
              'i'   ^
              'u'      ^
              'j'        ^
              'k'           ^ 
              'd'                ^
```
or by using a 'flattened' list of initialisers:

```
another_type at = { 1, 2, 3, 4 };
   'another_type' ^
              'i'   ^
            'u.j'      ^
            'u.k'         ^
              'd'            ^
```
Where the use of `'u.j'` and `'u.k'` in the explanation is trying to describe how, for the purposes of initialisation,
`'u'` is 'transparent' and these nested fields are treated as if they were at the same level in the structure as `'i'` and `'d'`.

So, for the integer array case, we can remove the second level of braces and initialise the elements of the
struct -- which in this case consist solely of data[0] to data[3] -- by writing:

```
int_array ia = { 1, 2, 3, 4 };
         data[0] ^
         data[1]    ^
          etc...
```
The same can be done when using a user defined type, too (where here the scalar elements available are `data[0].i`,
`data[0].j`, `data[1[].i` through to `data[3].j`):

```
udt_array ua = { 1, 2, 3, 4, 5, 6, 7, 8 };
       data[0].i ^
       data[0].j    ^
       data[1].i       ^
       data[1].j          ^
          etc...
```

So an *alternative* solution to Andrew's problem is to remove a level of braces: it turns out that in his case
providing two levels of braces could be resolved *either* by adding *or* by removing braces!

However, while this is valid C (and C++) code, this initialisation generates a *warning*, with gcc and clang,
if `-Wmissing-braces` is enabled. The reason for the warning is that the code is fragile to changes -- if
for example an additional integer field 'k' is appended to the `udt` struct the initialisation of `ua`
above now has a very different meaning: the first *three* numbers would now go to the first `udt` , the
next three to the second, the next two (with a trailing 0) to the third and the fourth element is zeroed.

```
udt_array ua = { 1, 2, 3, 4, 5, 6, 7, 8 };
       data[0].i ^
       data[0].j    ^
       data[0].k       ^
       data[1].i          ^
          etc...
```

So, let us return to the original question where just **two** levels of braces were supplied in the initialisation.
As you can see from comparing the "full" initialisation to the "minimal" one, we have actually got **two** potential
ways of using the second level of braces:

```
udt_array ua1 = { { 1, 2, 3, 4, 5, 6, 7, 8 } };

udt_array ua2 = { { 1, 2 }, {3, 4}, {5, 6}, {7, 8} };
```

In the first case, the top level braces apply to the whole struct and the second level braces apply to its data
member, so the eight values within it are simply used consecutively to initialise `data[0].i`, `data[0].j`,
`data[1].i`, `data[1].j`, etc.
In the second case, the top level braces apply to the whole struct and the **first set** of second level
braces apply to the `data` element of the struct, so the **two** values within it would be used to initialise
`data[0].i` and `data[0].j`, with the rest of `data` set to zero. The subsequent sets of braced pairs of numbers
are **extra** - if there were further data members of the array then they would be available to populate them.
However, `data` is the *only* member of the struct and so we get an error (from gcc and msvc) that too many
initialisers were provided. Note that clang produces a warning, rather than an error, and treats the code as it if were simply:

```
udt_array ua2 = { { 1, 2 } };
```

## But wait, there's more

In all the examples so far I have provided initialisers for the user defined type that are simple scalar values.
Of course, you can also provide instances of the user defined type itself as the initialisers. In C++ we can do
this by using an explicit constructor call in the initialiser, but even in C we can do this by providing named objects.
For example:

```
udt u1 = { 1, 2 }, u2 = { 3, 4 }, u3 = { 5, 6 }, u4 = { 7, 8 };
```
We can initialise `udt_array` from these objects like this:
```
  udt_array ua = { u1, u2, u3, u4 };
     'udt_array' ^
       'data[0]'   ^        
```
or by providing braces for both "levels" of initialisation:

```
udt_array ua = { { u1, u2, u3, u4 } };
   'udt_array' ^
        'data'   ^
         '[0]'     ^
```
## Conclusion

I hope providing these examples will help to make what is going on "under the covers", so to speak, a little
clearer and thereby improve people's understanding of what can be quite a confusing part of the C and C++
initialisation syntax.

It is unfortunate that the rules for aggregate initialisation mean that initialising an array of a user
defined type requires more careful use of braces than for an array of simples types, such as `int`.

<hr>
Copyright (c) Roger Orr 2021-07-07 (First published in CVu, Jul 2021)
