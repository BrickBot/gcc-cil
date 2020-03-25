SECTIONS {
  .text : {
  } > ram1
	 
  .data : {
  } > ram1
	
  .bss : {
  } > ram1
  
  .ocram : {
    _Lego_Rcx_VectorTable_AfterReset = 0x10;
    _Lego_Rcx_VectorTable_NonMaskableInterrupt = 0x12;
    _Lego_Rcx_VectorTable_Irq0 = 0x14;
    _Lego_Rcx_VectorTable_Irq1 = 0x16;
    _Lego_Rcx_VectorTable_Irq2 = 0x18;
    _Lego_Rcx_VectorTable_Timer16Bit_InputCaptureA = 0x1A;
    _Lego_Rcx_VectorTable_Timer16Bit_InputCaptureB = 0x1C;
    _Lego_Rcx_VectorTable_Timer16Bit_InputCaptureC = 0x1E;
    _Lego_Rcx_VectorTable_Timer16Bit_InputCaptureD = 0x20;
    _Lego_Rcx_VectorTable_Timer16Bit_OutputCompareA = 0x22;
    _Lego_Rcx_VectorTable_Timer16Bit_OutputCompareB = 0x24;
    _Lego_Rcx_VectorTable_Timer16Bit_Overflow = 0x26;
    _Lego_Rcx_VectorTable_Timer8Bit_Channel0_OutputCompareA = 0x28;
    _Lego_Rcx_VectorTable_Timer8Bit_Channel0_OutputCompareB = 0x2A;
    _Lego_Rcx_VectorTable_TimerBit_Channel0_Overflow = 0x2C;
    _Lego_Rcx_VectorTable_Timer8Bit_Channel1_OutputCompareA = 0x2E;
    _Lego_Rcx_VectorTable_Timer8Bit_Channel1_OutputCompareB = 0x30;
    _Lego_Rcx_VectorTable_TimerBit_Channel1_Overflow = 0x32;
    _Lego_Rcx_VectorTable_SerialCommunication_ReceiveError = 0x34;
    _Lego_Rcx_VectorTable_SerialCommunication_ReceiveEnd = 0x36;
    _Lego_Rcx_VectorTable_SerialCommunication_TdrEmpty = 0x38;
    _Lego_Rcx_VectorTable_SerialCommunication_TsrEmpty = 0x3A;    
    _Lego_Rcx_VectorTable_ADConverter_ConversionEnd = 0x3C;    
    _Lego_Rcx_VectorTable_WatchdogTimer_Overflow = 0x3E;
  } > ocram
}
