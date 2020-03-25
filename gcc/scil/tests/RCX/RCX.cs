using Hardware;
using Renesas;

namespace Lego
{
 
    public struct Rcx
    {    
        [MemoryAlias (0xfd90)]
        public static VectorTable VectorTable;
    }
  
    public struct VectorTable
    {
        [MemoryAlias (0x00)]
        public InterruptHandler AfterReset;

        [MemoryAlias (0x02)]
        public InterruptHandler NonMaskableInterrupt;

        [MemoryAlias (0x04)]
        public InterruptHandler Irq0;

        [MemoryAlias (0x06)]
        public InterruptHandler Irq1;

        [MemoryAlias (0x08)]
        public InterruptHandler Irq2;

        [MemoryAlias (0x0A)]
        public Timer16Bit.VectorTable Timer16Bit;

        [MemoryAlias (0x18)]
        public Timer8Bit.VectorTable Timer8Bit;

        [MemoryAlias (0x24)]
        public SerialCommunication.VectorTable SerialCommunication;

        [MemoryAlias (0x2C)]
        public ADConverter.VectorTable ADConverter;

        [MemoryAlias (0x2E)]
        public WatchdogTimer.VectorTable WatchdogTimer;
    }
}
