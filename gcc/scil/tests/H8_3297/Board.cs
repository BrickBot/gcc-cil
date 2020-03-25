using Hardware;

namespace Renesas
{
    [StaticDelegate]
    public delegate void InterruptHandler ();

    public struct H8_3297
    {
        [MemoryAlias (0x0000)]
        public static H8_3297 Board;

        [MemoryAlias (0x0000)]
        public VectorTable VectorTable;
        /* If programming agains ROM use this line: */
        /* public readonly VectorTable VectorTable; */

        [MemoryAlias (0xFF90)]
        public Timer16Bit Timer16Bit;

        [MemoryAlias (0xFFA8)]
        public WatchdogTimer WatchdogTimer;

        [MemoryAlias (0xFFB0)]
        public PortIpcr Port1;

        [MemoryAlias (0xFFB1)]
        public PortIpcr Port2;

        [MemoryAlias (0xFFB4)]
        public PortIpcr3 Port3;

        [MemoryAlias (0xFFB5)]
        public Port Port4;

        [MemoryAlias (0xFFB8)]
        public Port Port5;

        [MemoryAlias (0xFFB9)]
        public Port Port6;

        [MemoryAlias (0xFFBE)]
        public sbyte Port7;

        [MemoryAlias (0xFFC2)]
        public sbyte WaitStateControlRegister;

        [MemoryAlias (0xFFC3)]
        public sbyte SerialTimerControlRegister;

        [MemoryAlias (0xFFC4)]
        public sbyte SystemControlRegister;

        [MemoryAlias (0xFFC5)]
        public sbyte ModeControlRegister;

        [MemoryAlias (0xFFC6)]
        public sbyte IrqSenseControlRegister;

        [MemoryAlias (0xFFC7)]
        public sbyte IrqEnableRegister;

        [MemoryAlias (0xFFC8)]
        public Timer8Bit Timer8Bit;

        [MemoryAlias (0xFFD8)]
        public SerialCommunication SerialCommunication;

        [MemoryAlias (0xFFE0)]
        public ADConverter ADConverter;
    }

    public struct VectorTable
    {
        [MemoryAlias (0x00)]
        public InterruptHandler AfterReset;

        [MemoryAlias (0x06)]
        public InterruptHandler NonMaskableInterrupt;

        [MemoryAlias (0x08)]
        public InterruptHandler Irq0;

        [MemoryAlias (0x0A)]
        public InterruptHandler Irq1;

        [MemoryAlias (0x0C)]
        public InterruptHandler Irq2;

        [MemoryAlias (0x18)]
        public Timer16Bit.VectorTable Timer16Bit;

        [MemoryAlias (0x26)]
        public Timer8Bit.VectorTable Timer8Bit;

        [MemoryAlias (0x36)]
        public SerialCommunication.VectorTable SerialCommunication;

        [MemoryAlias (0x46)]
        public ADConverter.VectorTable ADConverter;

        [MemoryAlias (0x48)]
        public WatchdogTimer.VectorTable WatchdogTimer;
    }
}
