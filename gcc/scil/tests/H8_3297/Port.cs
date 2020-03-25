using Hardware;

namespace Renesas
{
    public struct PortIpcr
    {
        [MemoryAlias (0x00)]
        public sbyte InputPullUpControlRegister;
        
        [MemoryAlias (0x04)]
        public Port port;    
    }

    public struct PortIpcr3
    {
        [MemoryAlias (0x00)]
        public sbyte InputPullUpControlRegister;
        
        [MemoryAlias (0x06)]
        public Port port;    
    }

    public struct Port
    {
        [MemoryAlias (0x00)]
        public sbyte DataDirectionRegister;

        [MemoryAlias (0x02)]
        public sbyte DataRegister;
    }
}
