public class SimpleExample
{
    public static void Main()
    {
        try
        {
            SimpleComExample.TestClass tc = new SimpleComExample.TestClass();
            System.Console.WriteLine(tc.GetMessage());
            System.Console.WriteLine("Press Enter...");
            System.Console.ReadLine();
        }
        catch (System.Exception ex)
        {
            System.Console.WriteLine("Unexpected exception {0}", ex);
        }
    }
}
