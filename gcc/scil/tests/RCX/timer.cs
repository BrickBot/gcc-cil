using Hardware;
using Renesas;

namespace Lego
{ 
    public class Timer
    {
        public static Cpu.SchedulerDelegate Scheduler;
    
        public static void Init ()
        {
            Stop ();
            
            Cpu.Scheduler = HandlerA;
            Rcx.VectorTable.Timer16Bit.OutputCompareA = Cpu.InvokeScheduler;
            Rcx.VectorTable.Timer16Bit.OutputCompareB = HandlerB;
          
            H8_3297.Board.Timer16Bit.ControlStatusRegister = 0x01;
            H8_3297.Board.Timer16Bit.ControlRegister = 0x02;

            H8_3297.Board.Timer16Bit.CompareMatchA = 500;
            H8_3297.Board.Timer16Bit.CompareMatchB = 100;
        }
      
        public static void Start ()
        {
            /* Enable interrupt when timer equals comparator A or B. */
            H8_3297.Board.Timer16Bit.InterruptEnableRegister |= 0x08 /* 0x0C */;
        }
      
        public static void Stop ()
        {
            /* Disable interrupt when timer equals comparator A or B. */
            H8_3297.Board.Timer16Bit.InterruptEnableRegister &= ~0x0C;
        }
                  
        public static Cpu.Stack HandlerA (Cpu.Stack stack)
        {
            H8_3297.Board.Timer16Bit.ControlStatusRegister &= ~0x08;
            return Scheduler (stack);
        }

        private static void HandlerB ()
        {
            H8_3297.Board.Timer16Bit.ControlStatusRegister &= ~0x04;
        }
    }
}
