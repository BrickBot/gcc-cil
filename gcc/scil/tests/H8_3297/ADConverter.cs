using Hardware;
using Hardware.BigEndian;

namespace Renesas
{
    public class ADConverter
    {
        [MemoryAlias (0x00)]
        public TwoBytes DataRegisterA;

        [MemoryAlias (0x02)]
        public TwoBytes DataRegisterB;

        [MemoryAlias (0x04)]
        public TwoBytes DataRegisterC;

        [MemoryAlias (0x06)]
        public TwoBytes DataRegisterD;

        [MemoryAlias (0x08)]
        public sbyte ControlStatusRegister;

        [MemoryAlias (0x09)]
        public sbyte ControlRegister;

        public class VectorTable
        {
            [MemoryAlias (0x00)]
            public InterruptHandler ConversionEnd;
        }
    }
}
