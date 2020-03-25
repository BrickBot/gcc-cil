class Hello
{
  static Hello h;
  static int x;

  public void hello (int y)
  {
    x = y;
  }

  public static void Main()
  {
    x = 35;
    System.Console.WriteLine(x);
  
    h = new Hello();
    h.hello(47);
  
    System.Console.WriteLine(x);    
  
    System.Environment.Exit(0);
  }
}
