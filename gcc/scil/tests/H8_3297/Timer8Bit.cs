using Hardware;
using Hardware.BigEndian;

namespace Renesas
{
    public struct Timer8Bit
    {
        [MemoryAlias (0x00)]
        public Channel Channel0;

        [MemoryAlias (0x08)]
        public Channel Channel1;

        public struct VectorTable
        {
            [MemoryAlias (0x00)]
            public Channel.VectorTable Channel0;

            [MemoryAlias (0x06)]
            public Channel.VectorTable Channel1;
        }

        public struct Channel
        {
            [MemoryAlias (0x00)]
            public sbyte ControlRegister;

            [MemoryAlias (0x01)]
            public sbyte ControlStatusRegister;

            [MemoryAlias (0x02)]
            public byte ConstantRegisterA;

            [MemoryAlias (0x03)]
            public byte ConstantRegisterB;

            [MemoryAlias (0x04)]
            public byte Counter;

            public struct VectorTable
            {
                [MemoryAlias (0x00)]
                public InterruptHandler CompareMatchA;

                [MemoryAlias (0x02)]
                public InterruptHandler CompareMatchB;

                [MemoryAlias (0x04)]
                public InterruptHandler Overflow;
            }
        }
    }
}
