# Are the Old Ways Sometimes the Best?

_Comparing the 'classic C++' and 'modern' ways to solve various programming tasks_


## Introduction

This article is based on the first part of my ACCU 2023 presentation of the same name.

All living programming languages change over time; sometimes these changes give new ways of doing old things. There are various places in C++ where this has occurred since its inception and looking at a few examples might be instructive. There are two main questions to keep in mind: what might guide us in choosing between idioms and are there any lessons for code we ourselves produce for others to consume?

In this case I will look at some of the many different ways a simple <code>for</code> loop from 'vintage' C++98 could be re-written using various facilities of the releases of C++ over the last 25 years.

The code is deliberately kept minimal so the examples are clearer; in an actual code base they would typically span more lines.

### The <code>for</code> loop

Let us start with a very simple construct: the <code>for</code> loop. The desired task is to calculate the product of a vector of integers.

1. Here's how it might have looked in C++98:
```cpp
#include <vector>

int product(const std::vector<int>& items) {
  int result = 1;
  for (int i = 0; i < items.size(); i++) {
    result *= items[i];
  }
  return result;
}
```
The loop is simple, and is accessible to many readers who are not C++ programmers. Since it uses a common C++ idiom, any practiced C++ programmer immediately understands the semantics of the loop without needing to spend very long working through the code.

There is a possible problem with this code, if the product **overflows** the maximum value that can be held in an <code>int</code>. I'm not going to look further at ways of resolving that particular problem in this article.

The loop as it stands has one issue: <code>items.size()</code> is of an unsigned integral type (often <code>std::size_t</code> is used) and this may cover a larger range than an <code>int</code> can hold. This was rarely an issue back in C++98 as it was quite rare to encounter such large collections in the majority of the places where C++ was being used. In this specific use case, unless the majority of the values in the vector are 1, attempting to calculate the product of the elements will not work anyway because of the overflow issue mentioned earlier.

2. The "raw" loop.

While I don't think anyone would recommend it, we can replace the loop using explicit control flow with <code>goto</code>:
```cpp
#include <vector>

int product_raw(const std::vector<int>& items) {
  int result = 1;
  int i = 0;
  goto end;
loop:
  result *= items[i];
  ++i;
end:
  if (i < items.size()) goto loop;
  return result;
}
```
Checking with a couple of compilers this code produced **exactly the same** compiled output as example 1. But source code is not written for computers to understand as much as for other people to understand.

So what have we lost in the rewrite? We have lost the block structure, and the idiom, which means most programmers will need to read the code through carefully before they understand what it is doing.
I doubt anyone would prefer the explicit control flow code over the simple for loop.

3. Some more modern idioms in the for loop

Since C++98 we have gained so-called uniform initialization, so many programmers make regular use of brace initialization. Computers in many environments now have a lot more memory, and the use of <code>int</code> for a loop index is becoming increasingly problematic - especially since in many 64-bit environments an <code>int</code> is only 32-bit.

So updating the old-style for loop to use contemporary idioms we might end up with something more like this:
```cpp
#include <vector>

int product(const std::vector<int>& items) {
  int result{1};
  for (std::size_t idx{}; idx != items.size(); ++idx) {
    result *= items[idx];
  }
  return result;
}
```
We have used:
- brace initialization for both <code>result</code> and <code>idx</code>.
- preferred <code>size_t</code> for the loop index. (Technically we might need to use the implementation-defined type <code>std::vector<int>::size_type</code>.)
- used an inequality not a comparison check on the loop variable (this allows for loops where equality is supported for the loop variable but comparisons are not).
- pre-incremented the loop variable (this makes no difference here with a primitive type, but can avoid unwanted creation of temporaries with more complicated loop variables, such as iterators)

The code is now slightly more idiomatic C++ - but this does mean it may be less easy for readers from <i>other</i> programming languages to read as, for example, they may be less familiar with brace initialization.

4. Almost Always Auto

Proponents of the 'almost always auto' style of programming may well prefer removing the explicit naming of types and match the type of the initializer.
```cpp
#include <vector>

int product(const std::vector<int>& items) {
  auto result{1};
  for (auto idx{0uz}; idx != items.size(); ++idx) {
    result *= items[idx];
  }
  return result;
}
```
(Using P0330R8 "Literal Suffix for (signed) size_t" from C++23, in gcc, clang, & edg to allow <code>0uz</code>)

Again, some people prefer this, others do not. It depends on the individual's exposure to the various alternatives. I personally don't think it is a matter of critical importance which one a team wishes to use; but I do think trying to have a consistent preference within a single code base is sensible.

5. Non-standard looping

There is nothing to stop us re-writing the code to iterate the other way round the collection.
```cpp
int product(const std::vector<int>& items) {
  auto result{1};
  for (auto idx{items.size()}; idx--; ) {
    result *= items[idx];
  }
  return result;
}
```
We no longer need to use <code>0uz</code> to get the correct type for idx as we initialize from <code>size()</code> and we can also combine the modification of the loop variable with the condition being tested.

However, we've used <code>idx--</code> in the loop condition, should we have used <code>--idx</code>? I suspect most people will need to think carefully about boundary conditions to be sure, and getting this wrong could produce some quite subtle bugs.

There may be times when you do need to iterate in reverse, but be aware that using a non-idiomatic loop construct makes the code harder to read so only do it if you really need to. Additionally, optimisers are written to recognise common code patterns, so it is possible that unusual loops may not be optimised as well as more idiomatic ones.

6. Saving a multiply

The original loop starts with 1 and multiplies this by every number in the collection. We could start with the <i>first</i> number and multiply by the others in turn, reducing the number of multiplies by one. This is potentially a (small) performance benefit.
```cpp
int product(const std::vector<int>& items) {
  if (items.empty()) return 1;
  auto result{items[0]};
  for (std::size_t idx{1}; idx != items.size(); ++idx) {
    result *= items[idx];
  }
  return result;
}
```
Now of course there needs to be a check for the degenerate case of an empty collection and the "non-standard" iteration range does make the code slightly harder to read. It may be worth it if the multiplies are expensive enough and the collection is typically quite small; but I would want to see a performance measurement to justify making such a change.

7. Using iterators

Some would prefer an iterator solution. This of course is nothing new, and iterators have been available in all versions of ISO C++. Here's a C++98 style solution:
```cpp
int product(const std::vector<int>& items) {
  int result = 1;
  for (std::vector<int>::const_iterator it = items.begin();
       it != items.end(); ++it) {
    result *= *it;
  }
  return result;
}
```
One obvious disadvantage of this code is that the readability of the code is decreased by the long type name "<code>std::vector&lt;int>::const_iterator</code>". On the other hand though, using <code>const_iterator</code> rather than <code>iterator</code> expresses that we are only reading from the vector and not modifying it, and also means than any accidental attempt to modify the vector using the iterator will fail to compile. The rest of the for loop is idiomatic C++ and clearly indicates that we're covering the whole vector from beginning to end.

8. Using a more modern iterator idiom

Here is what a more modern writer might use:
```cpp
int product(const std::vector<int>& items) {
  auto result = 1;
  for (auto it = items.cbegin(); it != items.cend(); ++it) {
    result *= *it;
  }
  return result;
}
```
(cbegin/cend added in C++11)

This is one of the cases where the majority of programmers are happy to use <code>auto</code>: no-one&ast; really cares what the actual iterator type is, the concern in most programmers minds is simply the type produced when it is dereferenced.

One of the reasons for introducing <code>cbegin</code>/<code>cend</code> was to support using <code>auto</code> in more cases. When using <code>auto</code> the intent is signalled in the <i>calls</i> being made: <code>cbegin</code> not <code>begin</code> where in the C++98 example the use of the <i>type</i> <code>std::vector<int>::const_iterator</code> expressed the intent.

(&ast; I'm wrong, of course)

9. Or use free functions?

There is also the option of using the free function forms of <code>cbegin</code> and <code>cend</code>: these delegate to the container's <code>cbegin</code> and <code>cend</code> when available, but also support inbuilt array types.
```cpp
int product(const std::vector<int>& items) {
  auto result = 1;
  for (auto it = **cbegin(items); it != **cend(items); ++it) {
    result *= *it;
  }
  return result;
}
```
(cbegin/cend free functions added in C++14)

Note however that to use the free forms of <code>cbegin</code> and <code>cend</code> with an inbuilt array type you need to ensure name lookup will find it, for example by adding:
```cpp
using std::cbegin;
using std::cend;
```
The example above works without this since argument dependent lookup on <code>items</code> will also look in the <code>std</code> namespace as this is where <code>vector</code> is from.

I personally tend to avoid use of inbuilt arrays, and so prefer using the member functions over the free functions even in function templates, as I'm not expecting these templates to be instantiated with array types. Your use cases may be different!

10. Generalising an iterator solution

One advantage of using iterators is that they represent a slightly higher level of abstraction; which means that an iterator solution can be generalised to handle a wider range of container types:
```cpp
template <typename Coll>
int product(const Coll & items) {
  int result = 1;
  for (auto it = items.begin(); it != items.cend(); ++it) {
    result *= *it;
  }
  return result;
}
```
This function is now usable is cases where, for example you have an <code>std::list&lt;int></code> rather than an <code>std::vector&lt;int></code>. However, since it is now a function template, the definition of the template must be available to the users of the function which typically moves code from implementation files into headers.

Function templates are usually slightly harder to write and debug than non-template functions, and some thought should be taken to balance this against a potentially wider applicability.

11. Using range for

C++11 added range for into the language, allowing us to hide the loop mechanics completely with a language solution:
```cpp
int product(const std::vector<int>& items) {
  int result = 1;
  for (auto item : items) {
    result *= item;
  }
  return result;
}
```
There are two main advantages for those reading the code of using range for. The first is that the loop automatically iterates over the whole collection, and since the loop variable is not public it cannot be modified in the body of the loop - whether deliberately or accidentally. The second is that the dereference of the iterator is done implicitly, so the loop variable contains the **value** of the item in the collection.

But what declared type should you use for <code>item</code>? I've used <code>auto</code> here, which means we are taking a **copy** of each element in the collection. (For integer values this seems fine, for some other data types making the copy can be expensive or even impossible). Other possible options here include <code>auto&</code>, <code>const auto&</code>, <code>auto&&</code>, or use of the actual type (<code>int</code>, <code>int&</code>, <code>const int&</code>, or <code>int&&</code>). Each option has reasons and contexts where it might be the best option.

One view held by many was that <code>auto&&</code> is the best thing to use by default. In this context this behaves much like a forwarding reference, and normally decays to <code>auto&</code> or <code>const auto&</code> depending on whether the collection being iterated over is <code>const</code> or not. Indeed, there was a proposal (N3994) to allow the type to be omitted in range for and for this to mean an implied use of <code>auto&&</code>:
```cpp
int product(const std::vector<int>& items) {
  int result = 1;
  for (item : items) { // (Not accepted into C++)
    result *= item;
  }
  return result;
}
```
This further simplification didn't achieve consensus, however - N3994 was rejected in plenary. There were enough people who <i>didn't</i> agree that the choice of <code>auto&&</code> was the best choice as the default type.

Unfortunately this is one of the cases where the naive use of plain <code>auto</code> is common, but a poor default. The creation of a copy means potential expense at runtime, and also may produce bugs where the user assigns to the loop variable thinking it will modify the underlying collection.

12. "No raw loops"

Sean Parent has a phrase "No raw loops". He says in [https://sean-parent.stlab.cc/presentations/2013-09-11-cpp-seasoning/cpp-seasoning.pdf]:
- Use an existing algorithm
   - Prefer standard algorithms if available
- Implement a known algorithm as a general function
   - Contribute it to a library
   - Preferably open source
- Invent a new algorithm
   - Write a paper
   - Give talks
   - Become famous!

So, here is a simple way to use an algorithm - does this comply with his dictum?
```cpp
#include <algorithm>

int product(const std::vector<int>& items) {
  int result = 1;
  std::for_each(items.begin(), items.end(),
    [&](int item) { result *= item; } );
  return result;
}
```

This is rather tongue-in-cheek: we're simply using an algorithm to write a loop for us. With range for in the C++ programmer's toolkit since C++11 I'm not sure what good use cases remain for using <code>std::for_each</code> in modern C++.

13. Using accumulate

We can use better algorithms than <code>for_each</code>. For example, we could write our function in terms of <code>accumulate</code>:
```cpp
#include <functional>
#include <numeric>

int product(const std::vector<int>& items) {
  return std::accumulate(items.begin(), items.end(), 1,
    std::multiplies());
}
```

(pre C++17 (P0091R3 "Class template argument deduction"), std::multiplies<>()
 pre C++14 (N3421 "Making Operator Functors greater<>"), std::multiplies<int>)

I believe <code>accumulate</code> suffers slightly from its name; which reflects the default behaviour - addition - when no operation is provided. It is in fact useful in many cases where you want to combine together the elements of a collection, but I think it is often overlooked by programmers as a possible part of their solution.

14. Or ranges::fold

The ranges library, added in C++20, is an extension and generalization of the existing C++ iterators and algorithms. In C++23 fold operations were added to the library, so we can now write:
```cpp
#include <algorithm>

int product(const std::vector<int>& items) {
  return std::ranges::fold_left(items, 1, std::multiplies());
}
```
(Using P2322R6 "ranges::fold" in C++23, implemented only in MSVC at the original time of writing)

The use of the ranges library means we can simply provide the collection object, and no longer need to invoke <code>begin</code> and <code>end</code> against the collection ourselves.

Additionally, you may find the name <code>fold_left</code> is a clearer indication of the purpose of the algorithm. Or you may not - when presenting this to a live audience some preferred <code>fold_left</code> and others preferred <code>accumulate</code> with no clear winner. I suspect it depends on people's previous history in computing.

### Conclusion

The humble <code>for</code> loop can be re-written in a **lot** of different ways. I'm sure you could come up with a few other ways yourself! One important question is, <i>which</i> way best expresses **intent** - to the actual audience of your code. This answer to this question is not an absolute but is dependent on the people who make up this audience.

How can you learn what ways are in fact clearer to your audience? Or can you improve the code reading skills of your audience so different constructs become easier for them to understand? In my experience code reviews are one such way where less experienced programmers can be exposed to more idiomatic constructs and have them explained clearly.

Whichever looping idioms are in common use in your code base, <i>non</i>-idiomatic loop constructs are going to be harder to reason about. Spending time having to work out whether a particular loop construct behaves correctly, especially at boundaries, makes understanding code more difficult.

If we are able to use higher level constructs, such as standard algorithms, to hide the loop completely rather than writing it out explicitly then we can perhaps avoid having to think about it at all. The reader's attention can then focus on what is being done rather than how it is being done.

<hr>
Copyright (c) Roger Orr 2024-04-07 (First published in CVu, Sep 2023)
