using Hardware;
using Renesas;
using Lego;

namespace TestRcx
{
  public class MainClass
  {
    public static void Main ()
    {
      Timer.Init ();
     
      Timer.Start ();
      
      ushort i = 0;

      while (true)
      {
        Cpu.Sleep ();
        i++;
        switch (i)
        {
          case 100:
            Speaker.Off();
            break;
          case 1000:
            Speaker.Sound(440);
            i = 0;
            break;
        }
      }
    }
  }
}
