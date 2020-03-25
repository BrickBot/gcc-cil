extern unsigned int Renesas_H8_3297_Board_WatchdogTimer_WordAccess;
extern char Renesas_H8_3297_Board_WatchdogTimer_ControlStatusRegister;
extern char Renesas_H8_3297_Board_WatchdogTimer_Counter;

static inline void
Renesas_H8_3297_Board_WatchdogTimer_set_Counter (char value)
{
  Renesas_H8_3297_Board_WatchdogTimer_WordAccess = 0x5A00 | value;
}

static inline char
Renesas_H8_3297_Board_WatchdogTimer_get_Counter (void)
{
  return Renesas_H8_3297_Board_WatchdogTimer_Counter;
}

static inline void
Renesas_H8_3297_Board_WatchdogTimer_set_ControlStatusRegister (char value)
{
  Renesas_H8_3297_Board_WatchdogTimer_WordAccess = 0xA500 | value;
}

static inline char
Renesas_H8_3297_Board_WatchdogTimer_get_ControlStatusRegister (void)
{
  return Renesas_H8_3297_Board_WatchdogTimer_ControlStatusRegister;
}
