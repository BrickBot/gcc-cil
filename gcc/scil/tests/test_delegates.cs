public delegate void print(string s);

public class A
{
	public int x = 0;
	public A(){}
	public A(print p)
	{
		invokeDelegate(p);
	}
	public void print(string s)
	{
		x++;
	}
	public void invokeDelegate(print p)
	{
		p("");
	}
}

public class Test_delegate
{
	static int y = 0;
	public static void printMethod(string s)
	{
		y++;
	}
	
	public static void Main()
	{
		A a = new A();
		A a2 = new A(new print(a.print));
		a.invokeDelegate(new print(printMethod));
		a.invokeDelegate(new print(a.print));
		for(int i=0;i<10;i++)
		{
			print p = new print(a.print);
			a.invokeDelegate(p);
		}
		if (y != 1)
			System.Environment.Exit(1);
		if (a.x != 12)
			System.Environment.Exit(1);
		System.Environment.Exit(0);	
	}
}
