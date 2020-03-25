#include "Rcx.h"
#include "sound.h"

/* Turns on the speaker on frequency (in Hertz, and not too precise) */
void 
Lego_Speaker_Sound (unsigned int frequency)
{
    if (frequency < 31) 
    {
        Lego_Speaker_Off ();
        return;
    }

    /* Select output inversion on compare-match A and 
       clear status flags_ */
    Renesas_H8_3297_Board_Timer8Bit_Channel0_ControlStatusRegister = 0x03;

    /* We must invert the signal after 1/frequency seconds */
    if (frequency <= 487)
    {
        Renesas_H8_3297_Board_Timer8Bit_Channel0_ConstantRegisterA = 7812 / frequency - 1;
        
        /* Select internal clock/1024 and clearing on compare-match A */
        Renesas_H8_3297_Board_Timer8Bit_Channel0_ControlRegister = 0x0B;
    }
    else if (frequency <= 3906)
    {
        Renesas_H8_3297_Board_Timer8Bit_Channel0_ConstantRegisterA = 124992 / frequency - 1;
        
        /* Select internal clock/64 and clearing on compare-match A */
        Renesas_H8_3297_Board_Timer8Bit_Channel0_ControlRegister = 0x0A;
    } 
    else 
    {
        Renesas_H8_3297_Board_Timer8Bit_Channel0_ConstantRegisterA = 999936 / frequency - 1;
        
        /* Select internal clock/8 and clearing on compare-match A */
        Renesas_H8_3297_Board_Timer8Bit_Channel0_ControlRegister = 0x09;
    }
}

void 
Lego_Speaker_Off ()
{
    Renesas_H8_3297_Board_Timer8Bit_Channel0_ControlRegister = 0x00;
}
