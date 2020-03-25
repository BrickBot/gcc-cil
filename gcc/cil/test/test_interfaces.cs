
namespace InterfaceTest
{
    interface IA
    {
        void ia();
    }

    interface IB
    {
        void ib();
    }

    interface IC
    {
        void ic();
    }

    interface ID : IC
    {
        void id1();

        void id2();
    }

    class E : IA
    {
        public void ia()
        {
            System.Console.WriteLine("Method ia() from class E");
        }
        virtual public void ve1()
        {
            System.Console.WriteLine("Method ve1() from class E");
        }
        virtual public void ve2()
        {
            System.Console.WriteLine("Method ve2() from class E");
        }
        public void e3()
        {
            System.Console.WriteLine("Method e3() from class E");
        }
    }




    class F : E, IB, ID
    {
        public void ib()
        {
            System.Console.WriteLine("Method ib() from class F");
        }
        public void ic()
        {
            System.Console.WriteLine("Method ic() from class F");
        }
        public void id1()
        {
            System.Console.WriteLine("Method id1() from class F");
        }
        public void id2()
        {
            System.Console.WriteLine("Method id2() from class F");
        }
        override public void ve1()
        {
            System.Console.WriteLine("Method ve1() from class F");
        }
        virtual public void vf()
        {
            System.Console.WriteLine("Method vf() from class F");
        }
    }

    
    
    class Program
    {
        static void Main(string[] args)
        {
            E e = new E();
            IA a = e;
            a.ia();
            e.ve1();
            e.ve2();
            e.e3();

            F f = new F();
            a = f;
            IB b = f;
            IC c = f;
            ID d = f;
            a.ia();
            b.ib();
            c.ic();
            d.id1();
            d.id2();
            f.ve1();
            f.ve2();
            f.vf();
            System.Environment.Exit(0);
        }
    }
}
