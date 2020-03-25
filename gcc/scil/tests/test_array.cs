class Hello{
  static void int_array(){
    int[] x = new int[3];
    x[0] = 10;
    x[1] = 20;
    x[2] = 30;
    int i, total;
    total = 0;
    for(i=0;i<x.Length;i++){
      total += x[i];
    }
    if(total!=60){
      System.Console.WriteLine("int_array failed");
      System.Environment.Exit(1);
    }
  } 

  static void string_array(){
    string[] a = new string[4];
    a[0] = "Hello";
    a[1] = a[0];
    if (a.Length != 4){
      System.Console.WriteLine("string_array failed");
      System.Environment.Exit(1);
    }
  }
      
      
  public static void Main(){
   int_array();
   string_array();
   System.Environment.Exit(0);
  }
}
