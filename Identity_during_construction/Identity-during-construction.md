# Identity During Construction

## Introduction

C++, Java and C# are related languages that share a lot of behaviour.
However, one place where there are differences is in the finer details of how objects 
are constructed.

While this is not often an issue in practice, it can matter a lot and, when it matters, our often unconscious
assumptions about "how it works" can let us down.

To illustrate the main differences I'll work through basically the same example in C++, Java and C# so
we can see how the behaviour changes.

## C++

What would you expect to see from this code that creates a single object of type `Derived`?

```c++
#include <iostream>
#include <typeinfo>

int value()
{
static int seed;
  std::cout << "value()\n";
  return ++seed;
}

class Base
{
private:
  int i = value();
public:
  Base()
  {
    std::cout << "Base ctor\n";
    std::cout << "Type=" << typeid(*this).name() << "\n";
    std::cout << "Base::i=" << i << "\n";
    init();
  }

  virtual void init()
  {
    std::cout << "Base::init\n";
    std::cout << "Base::i=" << i << "\n";
  }
};

class Derived: public Base
{
private:
  int j = value();
public:
  Derived()
  {
    std::cout << "Derived ctor\n";
    std::cout << "Type=" << typeid(*this).name() << "\n";
    std::cout << "Derived::j=" << j << "\n";
  }

  void init()
  {
    std::cout << "Derived::init\n";
    std::cout << "Base::j=" << j << "\n";
  }
};


int main()
{
  Derived d;
}
```
The way it works in C++ is that the object is constructed from the inside out.

First the `Base` sub-object is created; starting with the field initialisers and then calling the constructor.
Note that while this constructor runs the object is of runtime type `Base` and consequently,
the virtual call to `init()` calls the implementation in the `Base` class.

Then the `Derived` object is constructed, after starting with its field initialisers and then calling the
constructor for `Derived` to complete the creation of the total object.

The full output is:
```
value()
Base ctor
Type=class Base
Base::i=1
Base::init
Base::i=1
value()
Derived ctor
Type=class Derived
Derived::j=2
```

Note too that the two calls to `value()` made to initialise `i` and `j` are each made
just before the constructor body of the relevant class is executed.

The main advantage of this mechanism is that each 'layer' of the object can be created in turn and each one is completely constructed before the construction of the next layer starts.
However the consequential disadvantage is that a base class constructor (and correspondingly the destructor) has no knowledge of the existence (or type) of the complete object.
This can be a nuisance when, for example, the values of fields in the base class depend on attributes of the complete object or 
when trying to display information about object lifecycle in a common base class.

A common workaround for the first issue is so-called 'two phase construction' when the object is constructed as usual and then
a virtual method is invoked *after* the constructors
have all finished to complete the initialisation of the object.

## Java

Re-writing this example in Java produces these two classes:

```java
public class Base
{
  private static int seed;

  public static int value()
  {
    System.out.println("value()");
    return ++seed;
  }

  private int i = value();

  public Base()
  {
    System.out.println("Base ctor");
    System.out.println(String.format("Type=%s", getClass().getName()));
    System.out.println(String.format("Base.i=%s", i));
    init();
  }

  public void init()
  {
    System.out.println("Base.init");
    System.out.println(String.format("Base.i=%s", i));
  }
}
```

and the derived class:

```java
public class Derived extends Base
{
  private int j = Base.value();

  public Derived()
  {
    System.out.println("Derived ctor");
    System.out.println(String.format("Type=%s", getClass().getName()));
    System.out.println(String.format("Derived.j=%s", j));
  }

  public void init()
  {
    System.out.println("Derived.init");
    System.out.println(String.format("Derived.j=%s", j));
  }


  public static void main(String[] args)
  {
    Derived d = new Derived();
  }
}
```

The rules in Java are slightly different - in particular there is not the same rigid inside-out construction order and the object 
is of a consistent type throughout construction even though, when in the base constructor, the output object is incomplete.

The complete output from this example is
```
value()
Base ctor
Type=Derived
Base.i=1
Derived.init
Derived.j=0
value()
Derived ctor
Type=Derived
Derived.j=2
```

The main difference from the C++ example is that when `init` is called in the `Base` class 
constructor the call goes to the method in the `Derived` class - even though the fields of this class
are not yet populated (as shown by the line containing `Derived.j=0`).

Additionally, if we change the code in the `init` method to **modify** the value of `j` 
the value written will be overwritten by the subsequent initialisation!

However, while perhaps not ideal, the behaviour is **well defined** since in Java (as in C#) all the fields in the
object are zero-initialised at the start and hence the value of any field will be precisely specified.

If C++ allowed similar behaviour it would be very hard to avoid undefined behaviour as the fields in the derived
object would be uninitialised (and typically unspecified) before their initialisation occurred.

## C&#35;

The initialisation of objects in C# is, unsurprisingly, a lot like that in Java but there are still a couple
of interesting differences that can trap the unwary.

Here is a C# equivalent to the earlier examples (I've not followed the C# capitalisation conventions to try and make the code
easier to compare with the examples from the other two languages):
```csharp
public class Base
{
  private static int seed;

  public static int value()
  {
    System.Console.WriteLine("value()");
    return ++seed;
  }

  private int i = value();

  public Base()
  {
    System.Console.WriteLine("Base ctor");
    init();
    System.Console.WriteLine("Type={0}", GetType().ToString());
    System.Console.WriteLine("Base.i={0}", i);
  }

  public virtual void init()
  {
    System.Console.WriteLine("Base.init");
    System.Console.WriteLine("Base.i={0}", i);
  }
}
```

and the derived class of:

```csharp
public class Derived : Base
{
  private int j = Base.value();

  public Derived()
  {
    System.Console.WriteLine("Derived ctor");
    System.Console.WriteLine("Type={0}", GetType().ToString());
    System.Console.WriteLine("Derived.j={0}", j);
  }

  public override void init()
  {
    System.Console.WriteLine("Derived.init");
    System.Console.WriteLine("Derived.j={0}", j);
  }

  public static void Main()
  {
    Derived d = new Derived();
  }
}
```

The initialisation in C# follows Java in that the object type remains the same throughout the calls to both the `Base`
and the `Derived` constructor (and hence, as in the Java case, the call to `init()` in the `Base` constructor is 
made to the implementation in the `Derived` class.)

Where it differs is that the field initialisation occurs up-front before *either* constructor runs; and additionally
the construction of `j` in the `Derived` class comes **before** the construction of `i`
in the `Base` class - hence in this example the values of `i` and `j` are *reversed*
from the values in both the previous languages.

## Conclusion


The initialisation of objects (and their fields) is subtly different between C++, Java and C#.
It is easy to make a mistake if you don't realise what the differences are, especially if you are converting
code written in one language to run in another one.

The problem is that often, while the resultant code will **compile** and **execute** without reporting any problems,
the **behaviour** may not be what is expected.

I hope this simple example of the basic behaviour may help people avoid making this category of mistake.

---
Copyright(c) Roger Orr 2014-04-19 10:31:49 (First published in CVu)