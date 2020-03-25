
class Hello
{
    
    public void sayHello()
    {
        System.Console.WriteLine("Hello!");
    }

    public static void Main()
    {
        Hello hello = new Hello();
        hello.sayHello();
        System.Environment.Exit(0);
    }
}
