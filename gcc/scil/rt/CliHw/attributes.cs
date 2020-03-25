using System;

namespace Hardware
{
    [AttributeUsage(AttributeTargets.Field, AllowMultiple=false, Inherited=false)]
    public class AliasAttribute : Attribute
    {
        public AliasAttribute() {}
    }

    [AttributeUsage(AttributeTargets.Field, AllowMultiple=false, Inherited=false)]
    public sealed class PortAliasAttribute : AliasAttribute
    {
        public PortAliasAttribute(int port) {}
        public PortAliasAttribute(long port) {}
    }

    [AttributeUsage(AttributeTargets.Field, AllowMultiple=false, Inherited=false)]
    public sealed class MemoryAliasAttribute : AliasAttribute
    {
        public MemoryAliasAttribute(int address) {}
        public MemoryAliasAttribute(long address) {}
    }

    [AttributeUsage(AttributeTargets.Method, AllowMultiple = false, Inherited = false)]
    public sealed class InterruptHandlerAttribute : Attribute
    {
        public InterruptHandlerAttribute() {}
    }

    [AttributeUsage(AttributeTargets.Method, AllowMultiple = false, Inherited = false)]
    public sealed class NoReturnAttribute : Attribute
    {
        public NoReturnAttribute() {}
    }

    [AttributeUsage(AttributeTargets.Method, AllowMultiple = false, Inherited = false)]
    public sealed class SaveRegistersAttribute : Attribute
    {
        public SaveRegistersAttribute() {}
    }
  
    /* This class exists to disallow multiple annotations of MemoryRegion AND MemoryRegionGroup */
    [AttributeUsage(AttributeTargets.Class, AllowMultiple = false, Inherited = false)]
    public class MemoryRegionBaseAttribute : Attribute
    {
        protected internal MemoryRegionBaseAttribute () { }
    }
  
    [AttributeUsage(AttributeTargets.Class, AllowMultiple = false, Inherited = false)]
    public class MemoryRegionAttribute : MemoryRegionBaseAttribute
    {
        protected MemoryRegionAttribute(string name, int start, int length) { }
        protected MemoryRegionAttribute(string name, long start, long length) { }
    }

    [AttributeUsage(AttributeTargets.Class, AllowMultiple = false, Inherited = false)]
    public class MemoryRegionGroupAttribute : MemoryRegionBaseAttribute
    {
        protected MemoryRegionGroupAttribute() { }
    }
  
    /* If applied to a delegate type, this attribute declares that type to be a valuetype
       whose values can refer to static methods only. Hence, a delegate of that type has
       the same size as the method pointer it contains. 
       Furthermore, for the reasons above, you can only create these delegates and call
       them synchronously.
    */
    [AttributeUsage(AttributeTargets.Delegate, AllowMultiple = false, Inherited = false)]
    public class StaticDelegateAttribute : Attribute { }
}