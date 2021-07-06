# Auto - a necessary evil? (part 2)
(This article is based on the presentation of that title at ACCU 2013)

"*To have a right to do a thing is not at all the same as to be right in doing it.*"  - G.K.Chesterton.

## Introduction

In the first article we covered the rules governing the `auto` keyword that was added to the
language in C++11 (or added back, if your memory of C++ goes back far enough!)

It is important with a feature like `auto` not only to know the rules about
what is permitted by the language - and the meaning of the consequent code  - but also to be able
to decide **when** the use of the feature is appropriate and what design forces need to be considered
when taking such decisions.

In this article we look in more detail at some uses of `auto` with the intent of identifying
some of these issues.

## A "Complex type" example

One of the main motivations for `auto` was to simplify the declaration of variables with 'complicated' types.
One such example is in the use of iterators over standard library containers in cases such as:

```
std::vector<std::set<int>> setcoll;
std::vector<std::set<int>>::const_iterator it = setcoll.cbegin();
```

*Note* `cbegin` is another C++11 addition: it explicitly returns a `const` iterator
even from a mutable container.

Many programmers were put off using the STL because of the verbosity of the variable declarations.
With C++03 one recommendation was to use a typedef - and this approach remains valid in C++11:

```
typedef std::vector<std::set<int> > collType; // C++03 code still works fine
collType setcoll;
collType::const_iterator it = setcoll.begin();
```

With the addition of `auto` to the language the code can be shortened considerably:

```
std::vector<std::set<int>> setcoll;
auto it = setcoll.cbegin();
```

But is it **better**?

To help answer that question let us consider the alternatives in more detail.

The original code is often seen as hard to read because the length of the variable declaration dwarfs the name itself. Many
programmers dislike the way that the meaning of the code is masked by the scaffolding required to get the variable type correct.

Additionally, the code is fragile in the face of change. The type of the iterator is heavily dependent on the type of
the underlying container so the two declarations (for `setcoll` and `it`) must remain in step if the
type of one changes.

The second code, using a `typedef`, improves both the readability of the code and also the maintainability as, should
the type of the container change, the nested type `const_iterator` governed by the `typedef` will change too.
However having to pick a type name adds to the cognitive overhead; additionally good names are notoriously hard to pin down.

In the final code the use of `auto` further helps readability by focussing the attention on the expression used to
initialise `it` as this defines the type that `auto` will resolve to. Given this, code maintainability
is improved as the type of `it` will track the type required by the initialising expression.

We retain the type safety of the language - the variable is still strongly typed - but implicitly not explicitly.
The main downside of the final version of the code is that if you *do* need to know the precise type of the variable
then you have to deduce it from the expression, to do which also means knowing the type of the container.
On the other hand, it can be argued that to understand the semantics of the line of code you already have to know this
information, so the new style has not in practice made understanding the code any more difficult.

In this case I am inclined to agree with this view and I can see little downside to the use of `auto` to
declare variables for iterators and other such entities.

So:
- the code is quicker and easier to write and, arguably, to read
- the purpose is not lost in the syntax
- code generated is identical to the explicit type
- the variable automatically changes type if the collection type changes

However, the last point can be reworded as the variable ~~automatically~~ **silently** changes type if the collection type changes.
In particular this can be an issue with the difference between a `const` and non-`const` container. Note that the C++11 code
uses cbegin():
```
auto it = setcoll.cbegin();
```
If we'd retained the used of `begin()` we would have got a **modifiable** iterator from a non-`const` collection.
The C++03 code makes it explicit by using the actual type name:
```
std::vector<std::set<int>>::const_iterator it;
```
The stress is slightly different and may mean making some small changes to some class interfaces, as with the addition of cbegin().

## DRY example

`auto` allows you to specify the type name once. Consider this code:
```
std::shared_ptr<std::string> str =
    std::make_shared<std::string>("Test");
```
- We've repeated the `std::string`
- `make_shared` exists solely to create `std::shared_ptr` objects

We can write it more simply as:
```
auto str = std::make_shared<std::string>("Test");
```

The resulting code is just over half as long to write (and read) and I don't think we've lost any information.
Additionally the code is easier to change.

Using `auto` rather than repeating the type is indicated most strongly when:
- the type names are long or complex
- the types are identical or closely related

`auto` is less useful when:
- the type name is simple - or important
- the cognitive overhead on the reader of the code is higher

So I think `auto` may be less useful in an example like this:
```
// in some header
struct X {
 int *mem_var;
 void aMethod();
};

// in a cpp file
void X::aMethod() {
  auto val = *mem_var; // what type is val?
  ...
```

YMMV (Your mileage may vary) – opinions differ here. The ease of answering the question about the type of `val`
may also depend on whether you are using an IDE with type info.

For example, with Microsoft Visual Studio you get the type displayed in the mouse-over for the example above:

![Microsoft IDE](Microsoft-IDE.png)

## Dependent return type example

`auto` can simplify member function definitions. Consider this class and member function definition:
```
class Example
{
public:
  typedef int Result;

  Result getResult();
};

Example::Result Example::getResult()
{ return ...; }
```

We have to use the prefix of `Example::` for the return type `Result` as at this point in the definition
the scope does not include `Example`. `auto` allows the removal of the class name from the return type.

The syntax is to place the `auto` where the return type would otherwise go, then follow the function prototype
with `->` and the actual return type:
```
auto Example::getResult() -> Result
{ return ...; }
```
Whether or not this makes the code clearer depends on factors including:
- familiarity
- consistent use of this style

Personally, I still can't decide on this one. I think the new style is an improvement over the old one, but until use of
C++11 is sufficiently widespread trying to use the style may simply result in a mix of the old and new styles being used.
I do not think this would be a great step forward for existing code bases, but might be worth trying out for new ones.

## Polymorphism?

One problem with `auto` is the temptation to code to the *implementation* rather than to the *interface*.
If we imagine a class hierarchy with an abstract base class `Shape` and various concrete implementations such as
`Circle` and `Ellipse`. We might write code like this:
```
auto shape = make_shared<Ellipse>(2, 5);
...
shape->minor_axis(3);
```
The use of `auto` has made the generic variable `shape` to be of the explicit type shared pointer to Ellipse.
This makes it too easy to call methods -- such as `minor_axis` above -- that are not part of the interface but
of the implementation.

When the type of shape is 'shared pointer to the abstract base class' you can't make this mistake.
(Aside: I think this is a bigger problem with `var` in C# than with `auto` in C++
but your experience may be different.) The trouble is that `auto` is too “plastic” – it fits the exact type that matches
whereas *without* `auto` the author needs to make a decision about the most appropriate type to use.
This doesn't only affect polymorphism: `const`, signed/unsigned integer types and sizes are other possible pinch points
where the deduction of the type done by `auto` is not the best choice.

## What type *is* it?

It is possible to go to the extreme of making everything in the program use `auto`, but I'm not convinced
this is a good idea. For example, what does this program do:
```
auto main() -> int {
  auto i = '1';
  auto j = i * 'd';
  auto k = j * 100l;
  auto l = k * 100.;
  return l;
}
```
It is all too easy to assume the `auto` types are all the same – miss the promotion, the 'l' or the '.'.
Opinions also vary on whether writing `main` using `auto` aids readability - I
am not at all sure it does, especially given the large amount of existing code predating this use
of `auto`.

You can use the `auto` rules (on some compilers) to tell you the type. For example,
if we want to find out the actual type of `j` we could write this code:
```
auto main() -> int {
  auto i = '1';
  auto j = i * 'd', x = "x";
  ...
```
When compiled this will error as the type deduction for `auto` for the variables `j` and `x`
produces inconsistent types. A possible error message is:
```
     error: inconsistent deduction for 'auto':
  'int' and then 'const char*'
```
You may also be able to get the compiler to tell you the type by using template argument deduction, for example:
```
  template <typename T>
  void test() { T::dummy(); }

  auto val = '1';
  test<decltype(val)>();
```
This generates an error and the error text (depending on the compiler) is likely to include text such as:
`"see reference to function template instantiation 'void test<char>(void)' being compiled"`

### What are the actual rules?

The meaning of an `auto` variable declaration follows the rules for template argument deduction.

We can consider the invented function template
```
  template <typename T>
  void f(T t) {}
```
and then in the expression `auto val = '1';` the type of `val` is the same as that deduced for
`T` in the call `f('1')`.

This meaning was picked for good reason - type deduction can be rather hard to understand and it was
felt that having a subtly different set of rules for `auto` from existing places where types are
deduced would be a bad mistake.
However, this does mean that the type deduced when using `auto` differs from a (naïve) use of `decltype`:
```
  const int ci;
  auto val1 = ci;
  decltype(ci) val2 = ci;
```
`val1` is of type `int` as the rules for template argument deduction will drop the top-level `const`;
but the type of `val2` will be `const int` as that is the declared type of `ci`.

## Adding modifiers to `auto`

Variables declared using `auto` can be declared with various combinations of `const` and various sorts of references.
So what's the difference?
```
     auto          i   = <expr>;
     auto const    ci  = <expr>;
     auto       &  ri  = <expr>;
     auto const &  cri = <expr>;
     auto       && rri = <expr>;
```
As above, `auto` uses the same rules as template argument deduction so we can ask the equivalent question
about what type is deduced for the following uses of a function template:

```
template <typename T>;
     void f(T          i);
     void f(T const    ci);
     void f(T       &  ri);
     void f(T const &  cri);
     void f(T       && rri);
```
The answer to the question is, of course, "it depends" ... especially for the && case
(which is an example of what Scott Meyers has named the “Universal Reference”).

###`const` inference (values)

Let us start by looking at a few examples of using `auto` together with `const` for simple value declarations.
```
     int i(0); int const ci(0);

     auto       v0 = 0;
     auto const v1 = 0;
     auto       v2 = i;
     auto const v3 = i;
     auto       v4 = ci;
     auto const v5 = ci;
```
This is the easiest case and, as in the earlier discussion of the difference between `auto` and `decltype`,
v0 is of type `int` and v1 is of type `int const` (you may be more used to calling it `const int`.)
Similarly v2 and v4 are of type `int` and v3 and v5 are of type `int const`.


In general, with simple variable declarations, I prefer using `auto const` by default as the reader knows the value will remain
fixed. This means if they see a use of the variable later in the block they do not have to scan the intervening code to check whether or
not the value has been modified.

### `const` inference (references)

Let's take the previous example but make each variable an l-value reference:
```
     int i(0); int const ci(0);

     auto       & v0 = 0; // Error
     auto const & v1 = 0;
     auto       & v2 = i;
     auto const & v3 = i;
     auto       & v4 = ci;
     auto const & v5 = ci;
```
The first one **fails** as you may not form an l-value reference to a temporary value. However, you **are** allowed
to form a `const` reference to a temporary and so v1 is valid (and of type `int const &`.)

v2 is valid and is of type `int &` and the three remaining variables are all of type `int const &`.
Notice that the `const` for v4 is not removed, unlike in the previous example, as it is not a *top-level* use of `const`.

### Reference collapsing and `auto`

Things get slightly more complicated again when we use the (new) r-value reference in conjunction with `auto`.

```
    int i(0); int const ci(0);

    auto       && v0 = 0;
    auto const && v1 = 0;
    auto       && v2 = i;
    auto const && v3 = i; // Error
    auto       && v4 = ci;
    auto const && v5 = ci; // Error
```
The first variable, v0, becomes an r-value reference to the temporary 0 (type `int &&`) and the second,
v1, is the `const` equivalent (`int const &&`).
When it comes to v2, however, the reference type "collapses" to an l-value reference and so the type of v2 is simply `int &`.
v3 is invalid as the presence of the `const` suppresses the reference collapsing and you are not allowed to bind an
r-value reference to an l-value.
v4 reference-collapses to `int const &` and the declaration of v5 is an error for the same reason as for v3.

So this is the complicated one: ` auto && var = <expr>; ` as, depending on the expression, `var` could be
- `T       &`
- `T       &&`
- `T const &`
- `T const &&`

Deducing the last case is a little more obscure - you need to bind to a `const` temporary that is of class type.
Here's an example of deducing `const &&`:
```
class T{};
const T x() { return T(); }

auto && var = x(); // var is of type T const &&
```
Note that non-class types, like `int`, decay to &&. This changed during the development of C++11 and
at one point Microsoft's compiler and the Intellisense disagreed over the right answer!

![Const&&](constrefref.png)

(The compiler in the Visual Studio 2013 preview edition does now get this right.)

## More dubious cases

`auto` does not work well with initializer lists as the somewhat complicated rules for parsing these results in
behaviour, when used with `auto`, that may not be what you expect:
```
int main() {
  int var1{1};
  auto var2{1};
```
You might expect var1 and var2 to have the same type.
Sadly the C++ rules have introduced a new 'vexing parse' into the language. The type of var2 is
`std::initializer_list<int>`. There is a proposal to make this invalid as almost
everyone who stumbles over this behaviour finds it unexpected.

A mix of signed and unsigned integers – or integers of different sizes – can cause problems with `auto`.
In many cases the compiler generates a warning, if you set the appropriate flag(s), and if you heed the warning
you can resolve possible problems. But not in all cases ....
```
std::vector<int> v;
...
for (int i = v.size() - 1; i > 0; i -= 2)
{
  process(v[i], v[i-1]);
}
```
If you change `int` to `auto` then the code breaks. The trouble here is that v.size() returns
`std::vector::size_type` which is an *unsigned* integer value.
The rules for integer promotions means that `i` is also an unsigned integer value.
If it starts out odd it will decrease by 2 round the loop as far as 1, then the next subtraction will wrap around
-- to a large **positive** value. Of course, care must be taken to ensure that an `int` will be large
enough for all possible values of size() that the program might encounter.

I'm less convinced by the use of `auto` for variables defined by the results of arithmetic expressions
as the correct choice of variable type may be necessary to ensure the desired behaviour of the program.

## Conclusion

`auto` is a very useful tool in the programmer's armoury as it allows you to retain type safety
without needing to write out the explicit types of the variables. I expect that use of `auto`
will become fairly widespread once use of pre-C++11 compilers becomes less common.

However, I do have a concern that thoughtless use of `auto` may result in code that does not behave
as expected, especially when the data type chosen implicitly is not the one the reader of the code anticipates.

Please don't use `auto` without thought simply to save typing, but make sure you use it by conscious
choice and being aware of the potential issues and possible alternatives.

## Acknowledgements

Many thanks to Rai Sarich and the Overload reviewers for their suggestions and corrections which have helped to improve this article.

<HR>
Copyright (c) Roger Orr 2021-07-06 (First published in Overload Aug 2013)
