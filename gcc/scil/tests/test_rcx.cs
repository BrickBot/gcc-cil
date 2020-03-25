using Hardware;
using Renesas;
using Lego;

namespace TestRcx
{
  public class MainClass
  {
    static Cpu.Context savedContext;
  
    static void Switch ()
    {
/*      Cpu.Context h = savedContext;
      savedContext = Cpu.CurrentContext;
      Cpu.CurrentContext = h;
*/    }

    static ushort i = 0;
  
    static void RunScheduler ()
    {
      switch (++i)
      {
          case 100:
            Switch ();
            break;
          case 1000:
            i = 0;
            Switch ();
            break;
      }
    }

    static void Thread1 ()
    {
      while (true)
        {
          Speaker.Sound (440);
          Cpu.Sleep ();
        }
    }
  
    static void Thread2 ()
    {
      while (true)
        {
          Speaker.Off ();
          Cpu.Sleep ();
        }
    }
  
    public static void Main ()
    {     
/*      savedContext = Cpu.CurrentContext;     
      savedContext.ProgramCounter = Thread1;
*/    
      Timer.Init ();
      Timer.PeriodicEvent = RunScheduler;
      Timer.Start ();

      Thread2 ();
    }
  }
}
