class Hello
{
  public static void Main()
  {
    System.Console.WriteLine("Hello, world");
    int x = -1;
    if (x > 0)
    {
      System.Console.WriteLine("Hello, universe");
    } 
    else 
    {
      System.Console.WriteLine("Hello, *");
    }
  
    while (x < 5)
    {
      System.Console.WriteLine(x++);
    }
  
    System.Console.WriteLine(x < 5 ? 25 : 33);
  
    System.Environment.Exit(0);
  }
}
