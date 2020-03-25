#include "Rcx.h"
#include "Timer.h"

static void HandlerA (void);
static void HandlerB (void);

void 
Lego_Timer_Init ()
{
    Lego_Timer_Stop ();
    
    Lego_Rcx_VectorTable_Timer16Bit_OutputCompareA = HandlerA;
    Lego_Rcx_VectorTable_Timer16Bit_OutputCompareB = HandlerB;
            
    Renesas_H8_3297_Board_Timer16Bit_ControlStatusRegister = 0x01;
    Renesas_H8_3297_Board_Timer16Bit_ControlRegister = 0x02;

    Renesas_H8_3297_Board_Timer16Bit_set_CompareMatchA (500);
    Renesas_H8_3297_Board_Timer16Bit_set_CompareMatchB (100);
}
      
void 
Lego_Timer_Start ()
{
    /* Enable interrupt when timer equals comparator A or B_ */
    Renesas_H8_3297_Board_Timer16Bit_InterruptEnableRegister |= 0x08 /* 0x0C */;
}

void 
Lego_Timer_Stop ()
{
    /* Disable interrupt when timer equals comparator A or B_ */
    Renesas_H8_3297_Board_Timer16Bit_InterruptEnableRegister &= 0xF3;
}

void  __attribute__ ((saveall))
HandlerA ()
{
    Renesas_H8_3297_Board_Timer16Bit_ControlStatusRegister &= 0xF7;
}

void  __attribute__ ((saveall))
HandlerB ()
{
    Renesas_H8_3297_Board_Timer16Bit_ControlStatusRegister &= 0xFB;
}
