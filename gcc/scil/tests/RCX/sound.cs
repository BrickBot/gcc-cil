using Hardware;
using Renesas;

namespace Lego
{ 
    public class Speaker
    {    
        /* Turns on the speaker on frequency (in Hertz, and not too precise) */
        public static void Sound (ushort frequency)
        {
            if (frequency < 31)
            {
		Off ();
                // TODO: throw ArgumentOutOfRangeException
                return;
            }

            /* Select output inversion on compare-match A and 
               clear status flags. */
            H8_3297.Board.Timer8Bit.Channel0.ControlStatusRegister = 0x03;

            /* We must invert the signal after 1/frequency seconds */
            if (frequency <= 122)
            {
                H8_3297.Board.Timer8Bit.Channel0.ConstantRegisterA = 
                    (byte) ( (short) 7813 / frequency);
                
                /* Select internal clock/1024 and clearing on compare-match A */
                H8_3297.Board.SerialTimerControlRegister &= ~0x01;
                H8_3297.Board.Timer8Bit.Channel0.ControlRegister = 0x0B;
            }
            else if (frequency <= 488)
            {
                H8_3297.Board.Timer8Bit.Channel0.ConstantRegisterA = 
                    (byte) (31250 / frequency);
                
                /* Select internal clock/256 and clearing on compare-match A */
                H8_3297.Board.SerialTimerControlRegister |= 0x01;
                H8_3297.Board.Timer8Bit.Channel0.ControlRegister = 0x0B;
            } 
            else if (frequency <= 976)
            {
                H8_3297.Board.Timer8Bit.Channel0.ConstantRegisterA = 
                    (byte) (125000 / frequency);
                
                /* Select internal clock/64 and clearing on compare-match A */
                H8_3297.Board.SerialTimerControlRegister &= ~0x01;
                H8_3297.Board.Timer8Bit.Channel0.ControlRegister = 0x0A;
            }
            else if (frequency <= 3906)
            {
                H8_3297.Board.Timer8Bit.Channel0.ConstantRegisterA = 
                    (byte) (250000/ frequency);
                
                /* Select internal clock/32 and clearing on compare-match A */
                H8_3297.Board.SerialTimerControlRegister |= 0x01;
                H8_3297.Board.Timer8Bit.Channel0.ControlRegister = 0x0A;
            }
            else if (frequency <= 15625)
            {
                H8_3297.Board.Timer8Bit.Channel0.ConstantRegisterA = 
                    (byte) (1000000 / frequency);
                
                /* Select internal clock/8 and clearing on compare-match A */
                H8_3297.Board.SerialTimerControlRegister &= ~0x01;
                H8_3297.Board.Timer8Bit.Channel0.ControlRegister = 0x09;
            }           
            else
            {
                H8_3297.Board.Timer8Bit.Channel0.ConstantRegisterA = 
                    (byte) (4000000 / frequency);
                
                /* Select internal clock/2 and clearing on compare-match A */
                H8_3297.Board.SerialTimerControlRegister |= 0x01;
                H8_3297.Board.Timer8Bit.Channel0.ControlRegister = 0x09;
            }
        }
      
        public static void Off ()
        {
            H8_3297.Board.Timer8Bit.Channel0.ControlRegister = 0x00;
        }
    }
}
