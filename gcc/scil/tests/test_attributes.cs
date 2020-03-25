using System;

namespace TestCil
{
    [AttributeUsage(AttributeTargets.Field, AllowMultiple=false, Inherited=false)]
    public sealed class FixedLocationAttribute : Attribute
    {
        public FixedLocationAttribute(int ptr)
        {
        }
    }

    public class TestThis
    {
        [FixedLocation(0xFF03)]
        static byte x;
    
        public static void Main ()
        {
          x = 12;
        }
    }
}