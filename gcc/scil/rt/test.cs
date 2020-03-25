using Hardware;


class Hello
{
  [StaticDelegate]
  delegate void D ();
 
  struct S
   {
      [MemoryAlias(0x10)]
      public byte x;
   }
 
  struct T
    {
      [MemoryAlias(0x04)]
      public S s;
    }
 
  [MemoryAlias(0xFF00)]
  static byte x;
 
  [MemoryAlias(0xFF00)]
  static S s;
  
  [MemoryAlias(0xFF00)]
  static T t;

  [MemoryAlias(0xFE43)]
  static D d;
  
  static void Main ()
  {
    d = Blabla;
    x = (byte) 120;
    s.x &= (byte) 10;
    t.s.x ^= (byte) 12;
  }

  static void Blabla ()
  {
    x = (byte) 210;
  }
}