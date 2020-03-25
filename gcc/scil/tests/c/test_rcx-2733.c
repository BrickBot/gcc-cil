#include "Hardware.h"
#include "Rcx.h"
#include "sound.h"
#include "timer.h"

int
main (int argc, char **argv)
{
  Lego_Timer_Init ();
    
  Lego_Timer_Start ();
      
  unsigned int i = 0;

  for (;;)
  {
    _ZN5CliHw8Hardware3Cpu5SleepEv ();
    i++;
    switch (i)
    {
      case 100:
        Lego_Speaker_Off ();
        break;
      case 1000:
        Lego_Speaker_Sound (440);
        i = 0;
        break;
    }
  }
}
