using Hardware;
using Renesas;
using Lego;

public class Timer
{
  public static void Init ()
  {
    Stop ();

    Rcx.VectorTable.Timer16Bit.OutputCompareA = HandlerA;

    H8_3297.Board.Timer16Bit.ControlStatusRegister = 0x01;
    H8_3297.Board.Timer16Bit.ControlRegister = 0x02;
    H8_3297.Board.Timer16Bit.CompareMatchA = 500;
  }

  public static void Start ()
  {
    H8_3297.Board.Timer16Bit.InterruptEnableRegister |= 0x08;
  }

  public static void Stop ()
  {
    H8_3297.Board.Timer16Bit.InterruptEnableRegister &= ~0x08;
  }
      
  private static bool flag = true;    
  
  [InterruptHandler]
  private static void HandlerA ()
  {
    H8_3297.Board.Timer16Bit.ControlStatusRegister &= ~0x08;

  if (MainClass.Frequency > 2200)
      flag = false;
    else if (MainClass.Frequency < 220)
      flag = true;

    if (flag) 
      MainClass.Frequency++; 
    else 
      MainClass.Frequency--;
  }
}

public class MainClass
{
  public static ushort Frequency = 220;

  public static void Main ()
  {
    Cpu.Scheduler = null;
   
    Timer.Init (); 
    Timer.Start ();
    
    while (true) 
    {
      Speaker.Sound (Frequency);
      Cpu.Sleep ();
    }
  }
}

