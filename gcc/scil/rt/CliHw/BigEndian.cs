namespace Hardware.BigEndian
{
    public struct TwoBytes
    {
        [MemoryAlias(0x00)]
        public byte High;

        [MemoryAlias(0x01)]
        public byte Low;
    }
}
