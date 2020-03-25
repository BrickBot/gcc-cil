using Hardware;

namespace Renesas
{
    public struct SerialCommunication
    {
        [MemoryAlias (0x00)]
        public sbyte SerialModeRegister;

        [MemoryAlias (0x01)]
        public byte BitRateRegister;

        [MemoryAlias (0x02)]
        public sbyte SerialControlRegister;

        [MemoryAlias (0x03)]
        public sbyte TransmitDataRegister;

        [MemoryAlias (0x04)]
        public sbyte SerialStatusRegister;

        [MemoryAlias (0x05)]
        public sbyte ReceiveDataRegister;

        public struct VectorTable
        {
            [MemoryAlias (0x00)]
            public InterruptHandler ReceiveError;

            [MemoryAlias (0x02)]
            public InterruptHandler ReceiveEnd;

            [MemoryAlias (0x04)]
            public InterruptHandler TdrEmpty;

            [MemoryAlias (0x06)]
            public InterruptHandler TsrEmpty;
        }
    }
}
