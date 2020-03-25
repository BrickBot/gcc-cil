using Hardware;
using Renesas;

namespace TestRcx
{
  public class MainClass
  {
    [InterruptHandler]
    static void Handler ()
    {
      H8_3297.Board.SerialTimerControlRegister = 123;
    }
  
    public static void Main ()
    {     
      H8_3297.Board.VectorTable.NonMaskableInterrupt = Handler;
    }
  }
}
