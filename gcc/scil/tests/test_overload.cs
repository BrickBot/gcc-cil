public class MyClass
{
  public static void Main()
  {
    System.Console.WriteLine("Hello, world");
    Overload ();
    Overload (4+2);
    Overload (true);
    Overload (-2147483648, 2147483647);
    Overload ((1-9+9*8)/5, true);
    System.Environment.Exit(0);
  }

  public static void Overload ()
  {
    System.Console.WriteLine("overloaded without parameters");
  }
  
  public static void Overload (int x)
  {
    System.Console.WriteLine("overloaded with int");
    System.Console.WriteLine(x);
  }
  
  public static void Overload (bool y)
  {
    System.Console.WriteLine("overloaded with bool");
    System.Console.WriteLine(y);
  }

  public static void Overload (int x, int y)
  {
    System.Console.WriteLine("overloaded with int, int");
    System.Console.WriteLine(x);
    System.Console.WriteLine(y);
  }
  
  public static void Overload (int x, bool y)
  {
    System.Console.WriteLine("overloaded with int, bool");
    System.Console.WriteLine(x);
    System.Console.WriteLine(y);
  }
}
