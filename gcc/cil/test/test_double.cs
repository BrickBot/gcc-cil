using System;

public class Test_double
{
    public static void testDoubleConversion()
    {
        double d = 1;
        int i = 1;
        byte b = 1;
        short s = 1;
        i = (int)d;
        s = (short)d;
        b = (byte)d;
        d = (double)i;
        d = (double)s;
        d = (double)b;
        if(d!=1.0) System.Environment.Exit(1);
        d = (double)(i+b);
        d = d + b;
		double d1=4.3;
		double d2=4.4;
		double d3=-d2;
		if(d2>d1) 
			Console.WriteLine("Double success...");
		else 
			System.Environment.Exit(6);
    }
    public static void testDoubleArrays()
    {
        double[] array = new double[100];
        array[10]=10.0;        
        double d = 13.0;
        array[11]=d;
        if(array[10]+array[11]!=23) System.Environment.Exit(7);
    }
    public static void Main()
    {
        double a = 3.1415;
        long l = 2;
        double b = a;
        if(a<3) System.Environment.Exit(1);
        if(b<3) System.Environment.Exit(2);
        if((a+b)<4) System.Environment.Exit(3);
        if((a-b)!=0) System.Environment.Exit(4);
        if(l>a) System.Environment.Exit(5);
		testDoubleConversion();
		testDoubleArrays();
        System.Environment.Exit(0);        
    }    
}