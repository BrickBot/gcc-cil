using Hardware;

namespace Renesas
{
    public struct WatchdogTimer
    {
#pragma warning disable 0649
        [MemoryAlias (0x00)]
        private sbyte controlStatusRegister;

        [MemoryAlias (0x01)]
        private byte counter;
#pragma warning restore 0649

        [MemoryAlias (0x00)]
        private ushort wordAccess;

        public byte Counter
        {
            get { return counter; }
            set
            {
                wordAccess = (ushort) (0x5A00 | (ushort) value);
            }
        }

        public sbyte ControlStatusRegister
        {
            get { return controlStatusRegister; }
            set
            {
                wordAccess = (ushort) (0xA500 | (ushort) value);
            }
        }

        public struct VectorTable
        {
            [MemoryAlias (0x00)]
            public InterruptHandler Overflow;
        }
    }
}
