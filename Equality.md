# Some objects are more equal than others

## *The many meanings of equality, value and identity*

## Roger Orr and Steve Love
(This article is based on our presentation at ACCU 2011)

## Equality in programming

Testing for equality is an important concern in a lot of programming tasks and is often used for control flow: equality
 is one of the commonest expressions used in `if`, `for` and `while` statements. However despite being 
 something that is covered in almost any introduction to a programming language the concept and implementation of equality
 can be quite complicated.

## Possible meanings of "equality"

There are a wide variety of meanings to the use of 'equality' in a programming language. The list of possible meanings includes:

   # Refer to the same memory location
   # Have the same value
   # Behave the same way

This article explores some of the details and pitfalls with equality in terms of just the first two items on this list.
 We found it was a harder task than it appears at first glance to get it right (for some definition of right), even
 ignoring the third item on our list or looking further afield for other meanings.

The first item in the list is often described as "identity comparison" and the second one as "value comparison", and
 we make use of these terms below. Note that value comparison usually refers to the *perceived* value for users 
 of the object and fields that don't affect this (for example internally cached values) are usually not included in 
 the comparison code.

We are further restricting the subject to focus primarily on only three languages: C++, C# and Java. Despite their common
 heritage and obvious similarities there are many differences in the sort of problems equality raises in each language:
 even at the basic level of language syntax we see:

In Java: `a == b` always does something for all variables `a` and `b` of the same type (and compiles in
 some cases when they are of different types) and you cannot change what it does.

In Java & C#: `anobject.[eE]quals(another)` always does something (we write `[eE]quals` because the method is 
spelled `equals` in Java but `Equals` in C#)

In C++ & C# you can overload the meaning of `==` and in C# & Java you can override `[eE]quals` to customize behaviour.

Let's start with the language construct form of equality "`==`" on the grounds that this must be a pretty fundamental
 definition to have been enshrined in the syntax of the programming language, What does each language provide for this 
 operator 'out of the box'?

In C++ "`==`" is predefined (as a value comparison) for all built-ins and the subset of the library types for which
 equality makes sense (e.g. `std::string`), but is not automatically provided for custom types defined in a program.
 However you can provide your own definitions of `operator==` as long as at least one argument is a custom type:
 and you can also specify your own return type for the operator (although returning anything but `bool` is usually a bad decision.)

In Java "`==`" is predefined for primitive built-ins and does a straightforward value comparison. For object types
 "`==`" performs identity comparison between the two objects supplied. You cannot change this behaviour.

In C# "`==`" is predefined, or overridden, for all built-ins and library types (whether these types are
 *reference* [`class`] or *value* [`struct`] types). It is not automatically provided for custom value types
 and performs identity comparison for custom reference types. C# lets you define "`==`" for any custom type,
 but you must additionally provide an implementation of "`!=`".

**Object Comparisons
**For Java and C# the presence of a single root class for all object types allows for a sensible definition
 of an equality method in this base class which takes an argument of the base class. In both languages the default
 implementation of this method, on custom types, performs identity comparison.

Java overrides equals() for some of the predefined types, such as `Integer`. However there is some confusing behaviour
 as this example demonstrates:

```
public class IntegerEquals
{
  public static void main(String[] args)
  {
    test(10);
    test(1000);
  }
  
  public static void test(int value)
  {
    System.out.println("Testing " + value);
    Object obj = value;
    Object obj2 = value;
    if (obj.equals(obj2)) System.out.println("Equals");
    if (obj == obj2) System.out.println ("==");
  }
}
```
If you compile and run this simple program you might be surprised:

```
Testing 10
Equals
==
Testing 1000
Equals
```

The two objects `obj` and `obj2` compare the same using the `equals` method  (as the overridden method in Integer
 compares values not identity) when executed with `value` set to 10 or 1000 as expected. However, on most implementations
 of Java, `obj` compares the **same** as `obj2` using == when `value` is set to 10 but they compare **different**
 when the value is set to 1000. What is happening here?

This is a consequence of an optimisation in the Java code that boxes primitive data into Integer objects. The compiler
 implements "`obj == value`' by calling `Integer.valueOf(value)` and this method caches "commonly used values" such as 
 10 [1]. Hence in the first case the compiler is performing identity comparison on two references to the same, cached,
 `Integer `with value 10 and in the second case the compiler is performing identity comparison on references to two
 *separate *temporary `Integer `objects with value 1000.

[footnote 1: See http://download.oracle.com/javase/6/docs/api/java/lang/Integer.html#valueOf%28int%29 ]

There is a similar problem with intern'ed strings (strings held in a shared pool of unique strings 
normally accessed using the String.intern() method) as demonstrated here:
```
public class Intern
{
  private static final String s1 = "Something";
  private static final String s2 = "Some";
  private static final String s3 = "thing";
  public static void main( String [ ] args )
  {
    if (s1 == s2 + s3) System.out.println("match!");
  }
}
```
This program prints `match!` when executed as the compiler ensures that strings with the same compile time value generate
 references to a single object. This is perfectly safe since strings in Java are immutable, but can cause some confusion.
 In general checking strings for equality in Java with `==` is unsafe and some tools provide a warning for attempts to do so.
 The danger is that compile time strings, which are interned, are treated differently from any runtime strings (which typically aren't).
The classic case where this causes problems is that unit tests, which typically use compile time strings, will pass most tests
 successfully whether you use the `equals `method or the `==` operator; but in actual use with runtime generated strings
 (such as those read from a file) the behaviour is different.

C# implicitly provides some implementation assistance with the `Equals` method for value types, but it's more
 complicated than it might appear at first sight.

```
struct Easy
{
  int X;
  int Y[ 100 ] ;
}
struct Hard
{
  int X;
  MyType Y;
}
```
Both these structures will have an Equals method synthesised by the compiler. The first class ("Easy") only contains basic 
scalar members and the Equals method will perform a bit-wise check on the two values (using the total size of the object),
 which is often exactly the desired behaviour (and is fast). In the case of the second class ("Hard") the presence of the 
 custom type 'MyType' means that the synthesised Equals method performs reflection on the class at run time to identify the
 fields and then does a member-wise comparison of all the members (including the basic scalar `int` member `X`).
 While this produces the correct answer the performance is likely to be significantly worse than an explicit implementation
 of equality.

Finally in both C# and Java thought needs to be given to ensure the primary object reference is non-null. This simple example
 demonstrates the problem and also a way (in C# only) to avoid it:

```
public class NullEquals
{
  public static void Main()
  {
    object a = null;
    object b = new object();
    if (a.Equals( b ))
      Console.WriteLine("Now there's a thing");
    if (object.Equals(a, b))
      Console.WriteLine("This should be safe enough");
  }
}
```
This program fails with a `NullReferenceException `as 'a' is null in the first call to `Equals` and you cannot call a method
 on a `null` object. The second call, using the static method taking two arguments. does not throw such an exception when 
 supplied with null references (and returns `false` if either a or b is null and `true` if they both are).

C++ does not have a single object root and so it doesn't really make sense to have an equals method, but it **does** have 
templates and to help with programming the STL there is `std::equal_to`, which by default performs ==. You can specialise 
it for your own type to pass your own types to methods and classes implemented in terms of `equal_to` such as `std::unordered_map`
**Example:**

```
#include <functional>
#include <iostream>
int main( )
{
  std::cout << "std::equal_to<int>()(10,10): "
    << std::equal_to<int>()(10,10) << std::endl;
}
```
The full story for C# is even more complex as there is a long list of equality measures, which have been added to as various
 new versions of the .Net framework have been released. The list includes

   * `object.Equals` (we've already seen both flavours of this one)
   * `object.ReferenceEquals`
   * `IEquatable<T>`
   * `IEqualityComparer`
   * `IEqualityComparer<T>`
   * `EqualityComparer<T>`
   * `IStructuralEquatable`
   * `StringComparer`

...and others we've probably missed...

The second element of this list, the `ReferenceEquals` method, is used to perform the identity check: that two references 
refer to the same object. The method is needed because ==, which performs this check by default, can be overridden. (Since 
Java does not allow operator overloading it has no need for such a method.)

However, when used in conjunction with object boxing, `object.ReferenceEquals` has some interesting behaviour:

```
public class RefEqual
{
  public static void Main()
  {
    int ten = 10;
    System.Console.WriteLine(object.ReferenceEquals(ten, ten));
  }
}
```
This program prints `False` because the two temporary boxed integer objects created to pass into the `ReferenceEquals` method
 are distinct, and hence different, objects. This is a related problem to the one shown above using the Java `Integer` class.

## Over*load*ing Equality

Both Java and C# allow the programmer the freedom to overload the [eE]quals method to take an argument of a different type. 
Here is an example in Java that shows the problems of a naive implementation:

```
public class OverloadingEquals
{
  private int value;
  public OverloadingEquals(int initValue)
  {
    value = initValue;
  }
  public boolean equals(OverloadingEquals oe)
  {
    return oe != null && oe.value == value;
  }
  public static void main(String[] args)
  {
    OverloadingEquals oe1 = new OverloadingEquals(10);
    OverloadingEquals oe2 = new OverloadingEquals(10);
    Object obj1 = oe1;
    Object obj2 = oe2;
    System.out.println("oe1.equals(oe2): " + oe1.equals(oe2));
    System.out.println("oe1.equals(obj2): " + oe1.equals(obj2));
    System.out.println("obj1.equals(oe2): " + obj1.equals(oe2));
    System.out.println("obj1.equals(obj2): " + obj1.equals(obj2));
  }
}
```
This program prints:

```
oe1.equals(oe2): true
oe1.equals(obj2): false
obj1.equals(oe2): false
obj1.equals(obj2): false
```
even though the **same** objects are being compared in each case. The trouble is that the overloaded method is called based
 on the **compile** time type of both the primary object and the argument. What you probably want in this case is logic based 
 on the **runtime** type.

There are some principles from the mathematics of "equivalence relations" that, if adhered to, result in a consistent use of
 the concept of equality. They are that equality is...

**Reflexive**  a==a is always true
**Commutative** - if a==b then b==a
**Transitive** - if a==b and b==c then a==c
**Reliable** - Never throws. (This means checking for null!)

These rules are listed out in fuller detail in the language references for both C# [C# equals] and Java [Java equals].
 The wording from the C++ standard is short enough to quote in full: "(5.10p4) Each of the operators shall yield true if 
 the specified relationship is true and false if it is false." There you have it: succinct at any rate!

Now let us try and apply these rules when considering polymorphic equality. Consider a two-dimensional coordinate class in C#:

```
class Coordinate
{
  public double X { get; set; }
  public double Y { get; set; }
  public override int GetHashCode()
  {
    // ... 
  }
  public override bool Equals(object other)
  {
    var right = other as Coordinate;
    if (right != null)
      return X == right.X && Y == right.Y;
    return false;
  }
}
```
We might extend this class to support a three-dimensional coordinate system like this:

```
class Coordinate3d : Coordinate
{
  public double Z { get; set; }
  public override int GetHashCode()
  {
    // ...
  }
  public override bool Equals(object other)
  {
    var right = other as Coordinate3d;
    if (right != null)
      return base.Equals( other ) &&
        Z == right.Z;
    return false ;
  }
}
```
How does this polymorphic equality fare when checked against our four relationships for equality?

```
var p1 = new Coordinate { X = 2.3, Y = 5.6 };
var p2 = new Coordinate3d { X = 2.3, Y = 5.6, Z = 10.11 };

p1.Equals( p2 ) is True
p2.Equals( p1 ) is False
```
Oops. The equality relationship fails the **commutative** requirement. We can improve our conformance to this
 requirement in C# by implementing `IEquatable<T>` - which enforces implementation of an override of Equals
 taking T - for both classes. This provides the symmetry for p1 and p2 but is still not a complete solution
 to the problem as this code fragment shows:

```
object o1 = p1;
Console.WriteLine("p1.Equals(o2) {0}, o2.Equals(p1) {1}",
  p1.Equals(o2), o2.Equals(p1));
```
However, even if we fix the commutative relation by making our equality test more complex we *still* have a problem. Let's add this variable:

```
var p3 = new Coordinate3d { X = 2.3, Y = 5.6, Z = 1.22 };
```
Now p1 will be equal to p3 (for the same reason it is equal to p2), but p2 and p3 will **not** compare equal. We have broken
 the **transitivity** requirement. How can we resolve this? Should we even try?

Let's consider *why* we have the problems we see. Our problems are mostly caused by attempting to define equality in a class
 hierarchy. What sense is there to try and compare a two-dimensional and three-dimensional object? They are not the same class. 
 The first solution is to change our design so that two and three dimensional classes are **not** related: we might use
 composition in preference to inheritance if we do wish to use some of the implementation of `Coordinate2d` in the 
 implementation of `Coordinate3d`.
When inheritance **is** needed  a good solution to the problematic elements of value equality is to allow comparison to 
succeed only if the actual run-time class types are the same, which can be implemented simply enough in C# by comparing
 the results of calling `GetType()` on each object.

## Incidental and intentional equality

Avoid defining equality just so it can be used in conjunction with something that requires it, e.g. hashed containers. 
While it may make the initial implementation simpler to define 'just enough' equality to be able to use the type in this 
way, such partial implementations of equality have a nasty habit of causing more serious problems later on as the code evolves.

Suppose for example that you have a C# class and wish to create a `HashSet` of objects from this class. It can be tempting to
 define an `equals()` method on the class that fulfils just the checks necessary for this usage. However the equality used for
 a comparison in this context might be very different from one used elsewhere: perhaps only certain key fields are relevant. 
 In this case an alternative way of solving the problem exists as the C# `HashSet` can use a pluggable equality comparer 
 (`IEqualityComparer<T>`) instead of using `equals()`. This also provides a clearer way of stating the intent than 
 implementing the equality operator just for using in the hash set. In C++ the `unordered_set` can be given its own equality comparer; however in the standard Java collection classes `HashSet` can only use `object.equals()`, so you're stuck with it. 

Within a single application, *both *meanings of equality might be required: for example in an application for playing card 
games do you need **the** Ace of Spaces or **an** Ace of Spades? In Java and C#, override `[Ee]quals` for a value-check and 
leave `==` well alone to perform its default action of an identity check. In C++, which allows access to the address of an 
object, you can explicitly compare addresses (for identity) or contents (for value). Unless of course someone has defined 
`operator&` for one of the types...

## Hashcodes

There is a close relationship between equality and hashing. For example the C# documentation states that "classes [..] must [...] 
guarantee that two objects considered equal have the same hash code". Java imposes a similar rule for `Object.hashCode()`.
The reason is simple: when hashing functions are used with collections of objects the hash code is used first as a coarse filter
 to partition objects into buckets with the same, or related, hash codes. If you implement a hash code function that means two 
 objects comparing equal have a different hash code then the two objects may end up in different buckets and the code won't ever 
 get to the point of testing for equality.
 
Hash codes for objects that can mutate are another problem, for example:

```
public static class Bogus
{
  public String Value;
  @Override public int hashCode()
  {
    return Value.hashCode();
  }
  @Override public boolean equals( Object other )
  {
    return ((Bogus)other).Value.equals( Value );
  }
}
```
Consider what happens if Value changes after inserting into a hashed container...  if the object's hash code changes after
 being added to a hashed container, subsequent attempts to look for the object in the container will be accessing the wrong bucket.

The default implementation of `GetHashCode()` in C# for a value type is the hash code of the first field - this is rarely
 the best implementation for most value types. While we were investigating hash code behaviour in C# we found an interesting
 'feature' of the Microsoft C# runtime: the hash code for a `boolean `value is constant! The following program demonstrates
 both these behaviours by printing `True` both times when compiled and run using Microsoft's implementation.

```
using System;
static class Program
{
  struct HashTest
  {
    public bool Enabled;
    public string Value;
  }
  public static void Main()
  {
    var h1 = new HashTest{ Enabled = true, Value = "Great!"};
    var h2 = new HashTest{ Enabled = false , Value = "Great!"};
    Console.WriteLine( h1.GetHashCode() == h2.GetHashCode());
    h1.Value = "Rubbish!";
    Console.WriteLine( h1.GetHashCode() == h2.GetHashCode());
  }
}
```
Using Visual Studio this program prints:
```
True
True
```

## Collections

Another set of issues is raised by considering equality on container types. When are two collections of things 
equal? Is it enough that the two containers have the same items or do they need to be in the same order? (As a 
side note, we can add to the C# list of equality checks with `SequenceEqual`, which insists on the same items, 
in the same order).

This is a question that has performance implications too: comparing two sets are equal when permutations are allowed 
has a higher complexity measure than the case when the ordering must match.

A further question that may need addressing with containers is whether you want a value or reference comparison: do 
two containers match if they contain the identical objects or if they contain objects with identical values?

Note that this is a case where polymorphic equality makes a lot of sense: two collections are equal when they contain 
the same objects. You are not usually interested in whether they are from the same class (or even whether the internal
 states are the same); the important thing for equality is the objects they contain.

**Conclusion**
Equality is hard to define simply even for a single language. It is easy to implement if you stick to a small set of
 common sense rules; more complicated implementations are possible but not in general recommended.

One key distinction is between *values* and *references*. You should know the difference between (polymorphic)
 reference types and value types in all languages and avoiding treating the two the same way! Equality for references 
 is a check for *identity* but equality for value types is a check for equal values of all (significant) fields.

Making use of immutability for value types has many benefits, far beyond equality. In the case of equality though it 
allows for the possibility of caching of objects and/or values and it also removes the class of problems exemplified 
by the example of modifying an object while it is held in a collection.

Using value equality in a class hierarchy rarely makes sense and should be avoided. It is often better to avoid inheritance
 in the sort of cases where equality might make sense and use composition instead. Classes can also be made `final`
 (or `sealed`) to prevent unwanted inheritance but this can be an annoyance when a user of the class has a valid reason
 for wanting to extend your class. 

## Further Reading
C# in a Nutshell has a deep exploration of equality in C#. For more about equality in Java see http://www.javapractices.com,
 and follow links through Overriding Object methods to implementing equals.
Angelika Langer and Klaus Kreft wrote a pair of articles on the subject [Langer]. While the target of their article is 
Java many of the points apply to C# as well.

## References

[C# Equals] - http://msdn.microsoft.com/en-us/library/bsc2ak47%28v=VS.100%29.aspx

[Java Equals] - http://download.oracle.com/javase/6/docs/api/java/lang/Object.html#equals%28java.lang.Object

[Langer] - http://www.angelikalanger.com/Articles/JavaSolutions/SecretsOfEquals/Equals.html 

<hr>
Copyright (c) Roger Orr and Steve Love 2021-07-05 (First published in Overload, June 2011)
