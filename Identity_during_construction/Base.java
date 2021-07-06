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
