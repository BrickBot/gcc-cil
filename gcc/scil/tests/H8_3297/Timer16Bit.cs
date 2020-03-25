using Hardware;
using Hardware.BigEndian;

namespace Renesas
{
    public struct Timer16Bit
    {
        [MemoryAlias (0x00)]
        public sbyte InterruptEnableRegister;

        [MemoryAlias (0x01)]
        public sbyte ControlStatusRegister;

        [MemoryAlias (0x02)]
        public ushort FreeRunningCounter;

        [MemoryAlias (0x04)]
        public ushort OutputCompareRegister;

        [MemoryAlias (0x06)]
        public sbyte ControlRegister;

        [MemoryAlias (0x07)]
        public sbyte OutputCompareControlRegister;

        [MemoryAlias (0x08)]
        public ushort InputCaptureRegisterA;

        [MemoryAlias (0x0A)]
        public ushort InputCaptureRegisterB;

        [MemoryAlias (0x0C)]
        public ushort InputCaptureRegisterC;

        [MemoryAlias (0x0E)]
        public ushort InputCaptureRegisterD;

        public struct VectorTable
        {
            [MemoryAlias (0x00)]
            public InterruptHandler InputCaptureA;

            [MemoryAlias (0x02)]
            public InterruptHandler InputCaptureB;

            [MemoryAlias (0x04)]
            public InterruptHandler InputCaptureC;

            [MemoryAlias (0x06)]
            public InterruptHandler InputCaptureD;

            [MemoryAlias (0x08)]
            public InterruptHandler OutputCompareA;

            [MemoryAlias (0x0A)]
            public InterruptHandler OutputCompareB;

            [MemoryAlias (0x0C)]
            public InterruptHandler Overflow;
        }
      
        public ushort CompareMatchA
        {     
            set 
            { 
                H8_3297.Board.Timer16Bit.OutputCompareControlRegister &= ~0x10;
                H8_3297.Board.Timer16Bit.OutputCompareRegister = value; 
            }
        }
      
        public ushort CompareMatchB
        {
            set 
            { 
                H8_3297.Board.Timer16Bit.OutputCompareControlRegister |= 0x10;
                H8_3297.Board.Timer16Bit.OutputCompareRegister = value; 
            }
        }      
    }
}
