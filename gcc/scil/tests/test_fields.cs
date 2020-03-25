public class A
{
	public bool b;
	protected bool d;
}

public class B:A
{
	public bool c;
	// check access to derived fields
	public bool Check()
	{
		return d;
	}
}

public class FieldTest
{
	static int si;
	static A sa;
	int i;
	A a;

	public void run()
	{
		// instance variables
		i=3;
		si = si+i;
		a=new A();
		a.b=true;
	}

	public static void Main(string[] args)
	{
		// static variables
		si = 3;
		sa = new A();
		sa.b = false;
		sa.b = sa.b | false;
		System.Environment.Exit(0);
	}
}
