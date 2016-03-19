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
