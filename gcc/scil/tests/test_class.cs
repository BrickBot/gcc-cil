public class Base{
  public int x;
  public int y;

  public Base(int a, int b){
    x=a;
    y=b;
  }

  public virtual int square(){
    return x*x+y*y;
  }
}

class Derived:Base{
  int z;

  Derived(int a,int b,int c):base(a,b){
    z=c;
  }

  public override int square(){
    return base.square()+z*z;
  }

  static void test_base(){
    Base b = new Base(3,4);
    if (b.square() != 25) {
      System.Console.WriteLine("test_base failed");
      System.Console.WriteLine(b.square());
      System.Environment.Exit(1);
    }
    else
    {
      System.Console.WriteLine("test_base succeeded");
      System.Console.WriteLine(b.square());
    }
  }

  static void test_derived()
  {
    Base b = new Derived(3,4,5);
    if (b.square() != 50) {
      System.Console.WriteLine("test_derived failed");
      System.Console.WriteLine(b.square());
      System.Environment.Exit(1);
    }
    else
    {
      System.Console.WriteLine("test_derived succeeded");
      System.Console.WriteLine(b.square());
    }
  }

  public static void Main(){
   test_base();
   test_derived();
   System.Environment.Exit(0);
  }
}
