using Hardware;
using Renesas;
using Lego;

class Scheduler
{  
  static bool flag = false;
  static ushort count = 0;

  public static Cpu.Stack DetermineNext (Cpu.Stack stack)
  { 
  
    if (count++ == 1000)
      {
        count = 0;
        flag = !flag;
      
        if (flag)
        {
          Thread.Thread1.stack = stack;
          return Thread.Thread2.stack;
        } 
        else 
        {
          Thread.Thread2.stack = stack;
          return Thread.Thread1.stack;
        }
      }
    return stack;
  }

}

[StaticDelegate]
delegate void ThreadStart ();

struct Thread
{
  public Cpu.Stack stack;

  public static Thread Thread1;
  public static Thread Thread2;

  static Thread ()
  {
    Thread2.stack = Cpu.InitStack (ThreadCode2, 0xE000);
  }

  static ushort frequency = 440;

  public static void ThreadCode1 ()
  {
    bool up = true;
    while (true)
    {
      if (frequency > 2200)
        up = false;
      else if (frequency < 220)
        up = true;
    
      if (up) 
        frequency++; 
      else 
        frequency--;
    }
  }

  static void ThreadCode2 ()
  {
    while (true)
      {
        Speaker.Sound (frequency);
        Cpu.Sleep ();
      }
  }
}

public class MainClass
{
  public static void Main ()
  {
    Timer.Init (); 
    Timer.Scheduler = Scheduler.DetermineNext;
    Timer.Start ();
    Thread.ThreadCode1 ();
  }
}

