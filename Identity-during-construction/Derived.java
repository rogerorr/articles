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
