SECTIONS {
  .onchipregisters : {
    /* starts at ff88 */		
    
    _Renesas_H8_3297_Board_Timer16Bit_InterruptEnableRegister       = 0x08;
    _Renesas_H8_3297_Board_Timer16Bit_ControlStatusRegister         = 0x09;
    _Renesas_H8_3297_Board_Timer16Bit_FreeRunningCounter            = 0x0A;
    _Renesas_H8_3297_Board_Timer16Bit_OutputCompareRegister         = 0x0C;
    _Renesas_H8_3297_Board_Timer16Bit_ControlRegister                = 0x0E;
    _Renesas_H8_3297_Board_Timer16Bit_OutputCompareControlRegister = 0x0F;
    _Renesas_H8_3297_Board_Timer16Bit_InputCaptureRegisterA        = 0x10;
    _Renesas_H8_3297_Board_Timer16Bit_InputCaptureRegisterB        = 0x12;
    _Renesas_H8_3297_Board_Timer16Bit_InputCaptureRegisterC        = 0x14;
    _Renesas_H8_3297_Board_Timer16Bit_InputCaptureRegisterD        = 0x16;
                    
    _Renesas_H8_3297_Board_WatchdogTimer_WordAccess               = 0x20;
    _Renesas_H8_3297_Board_WatchdogTimer_ControlStatusRegister    = 0x20;
    _Renesas_H8_3297_Board_WatchdogTimer_Counter                  = 0x21;
  
    _Renesas_H8_3297_Board_Port1_DataDirectionRegister               = 0x28;
    _Renesas_H8_3297_Board_Port2_DataDirectionRegister               = 0x29;
    _Renesas_H8_3297_Board_Port1_DataRegister                         = 0x2A;
    _Renesas_H8_3297_Board_Port2_DataRegister                         = 0x2B;
    _Renesas_H8_3297_Board_Port3_DataDirectionRegister               = 0x2C;
    _Renesas_H8_3297_Board_Port4_DataDirectionRegister               = 0x2D;
    _Renesas_H8_3297_Board_Port3_DataRegister                         = 0x2E;
    _Renesas_H8_3297_Board_Port4_DataRegister                         = 0x2F;
    _Renesas_H8_3297_Board_Port5_DataDirectionRegister               = 0x30;
    _Renesas_H8_3297_Board_Port6_DataDirectionRegister               = 0x31;
    _Renesas_H8_3297_Board_Port5_DataRegister                         = 0x32;
    _Renesas_H8_3297_Board_Port6_DataRegister                         = 0x33;
    
    _Renesas_H8_3297_Board_WaitStateControlRegister                 = 0x3A;
    _Renesas_H8_3297_Board_SerialTimerControlRegister               = 0x3B;
    _Renesas_H8_3297_Board_SystemControlRegister                     = 0x3C;
    _Renesas_H8_3297_Board_ModeControlRegister                       = 0x3D;
    
    _Renesas_H8_3297_Board_IrqSenseControlRegister                    = 0x3E;
    _Renesas_H8_3297_Board_IrqEnableRegister                          = 0x3F;
  
    _Renesas_H8_3297_Board_Timer8Bit_Channel0_ControlRegister               = 0x40;
    _Renesas_H8_3297_Board_Timer8Bit_Channel0_ControlStatusRegister        = 0x41;
    _Renesas_H8_3297_Board_Timer8Bit_Channel0_ConstantRegisterA            = 0x42;
    _Renesas_H8_3297_Board_Timer8Bit_Channel0_ConstantRegisterB            = 0x43;
    _Renesas_H8_3297_Board_Timer8Bit_Channel0_Counter                        = 0x44;
    
    _Renesas_H8_3297_Board_Timer8Bit_Channel1_ControlRegister               = 0x48;
    _Renesas_H8_3297_Board_Timer8Bit_Channel1_ControlStatusRegister        = 0x49;
    _Renesas_H8_3297_Board_Timer8Bit_Channel1_ConstantRegisterA            = 0x4A;
    _Renesas_H8_3297_Board_Timer8Bit_Channel1_ConstantRegisterB            = 0x4B;
    _Renesas_H8_3297_Board_Timer8Bit_Channel1_Counter                        = 0x4C;
    
    _Renesas_H8_3297_Board_SerialCommunication_SerialModeRegister                        = 0x50;
    _Renesas_H8_3297_Board_SerialCommunication_BitRateRegister                           = 0x51;
    _Renesas_H8_3297_Board_SerialCommunication_SerialControlRegister                     = 0x52;
    _Renesas_H8_3297_Board_SerialCommunication_TransmitDataRegister                      = 0x53;
    _Renesas_H8_3297_Board_SerialCommunication_SerialStatusRegister                      = 0x54;
    _Renesas_H8_3297_Board_SerialCommunication_ReceiveDataRegister                       = 0x55;
  
    _Renesas_H8_3297_Board_ADConverter_DataRegisterA                          = 0x58;
    _Renesas_H8_3297_Board_ADConverter_DataRegisterB                          = 0x5A;
    _Renesas_H8_3297_Board_ADConverter_DataRegisterC                          = 0x5C;
    _Renesas_H8_3297_Board_ADConverter_ControlStatusRegister                  = 0x60;
    _Renesas_H8_3297_Board_ADConverter_ControlRegister                         = 0x61;
  } > ocreg
}
