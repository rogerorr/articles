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
