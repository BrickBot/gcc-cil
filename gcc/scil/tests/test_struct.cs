class test_struct
{
  struct struct_A
  {
    public int x;
    public int y;
    public int z;
    
    public struct_A (int x)
    {
      this.x = x;
      y = 98;
      z = 79;
    }
    public int X 
    {
      get { return x; }
      set { x = value; }
    }
  }

  struct struct_B
  {
    public struct_A a;
    public struct_B (struct_A a)
    {
      this.a = a;
    }
    public struct_A A
    {
      get { return a; }
      set { a = value; }
    }    
    public int X
    {
      get { return a.X; }
      set { a.X = value; }
    }
    public int AX
    {
      get { return A.X; }
    }
  }

  public static void Main ()
  {
    struct_A a = new struct_A (10);
    System.Console.WriteLine (a.x);
    a.x = 1;
    System.Console.WriteLine (a.X);
    a.X = 2;
  
    struct_B b = new struct_B (a);
    System.Console.WriteLine (b.a.x);
    b.a.x = 3;
    System.Console.WriteLine (b.a.X);
    b.a.X = 4;
    System.Console.WriteLine (b.A.x);
    System.Console.WriteLine (b.A.X);
    System.Console.WriteLine (b.X);
    b.X = 7;
    System.Console.WriteLine (b.AX);
  
    struct_A a1 = a;
    System.Console.WriteLine (a1.z);
  }
}