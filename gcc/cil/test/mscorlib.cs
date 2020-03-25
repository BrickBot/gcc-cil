using System.Runtime.CompilerServices;
namespace System{
  public class Object{
    public Object(){}
    virtual public string ToString(){
        return "";
    }
    public Object Clone(){return this;}
  }
  
  sealed public class String{
    int length;
    char startChar;
    //[Runtime.InteropServices.DllImportAttribute()]
    static String native_Concat(String a,String b){return null;}
    
    int Length{
      get{return length;}
   }

    public override String ToString(){return this;}
    
    public static String Concat(String a, String b){
      return native_Concat(a,b);
    }
    public static String Concat(String a, String b, String c){
      return Concat(Concat(a,b), c);
    }
    public static String Concat(String a, String b, String c, String d){
      return Concat(Concat(a,b,c), d);
    }
    public static String Concat(String[] strings){
      String result = strings[0];
      for(int i=1; i < strings.Length; i++)
        result = native_Concat(result, strings[i]);
      return result;
    }
    public static String Concat(Object o){
      if(o==null)return "";
      return o.ToString();
    }
    public static String Concat(Object o1, Object o2){
      return Concat(Concat(o1),Concat(o2));
    }
    public static String Concat(Object o1, Object o2, Object o3){
      return Concat(Concat(o1, o2), o3);
    }
    
    public static String Concat(Object[] objs) {
      String result = "";
      for(int i=0;i<objs.Length;i++){
        result = Concat(result, objs[i]);
      }
      return result;
    }
    public static String IsInterned(String s){return null;}
  }
  
  abstract public class ValueType{
  }
  
  public class Attribute{}
  public class ObsoleteAttribute:Attribute{}
  public class CLSCompliantAttribute:Attribute{}
  public class AttributeUsageAttribute:Attribute{}
  public class ParamArrayAttribute:Attribute{}
  
  public struct Void{byte empty;}
  public struct Byte{byte m_value;}
  public struct SByte{sbyte m_value;}
  public struct Int16{short m_value;}
  public struct UInt16{ushort m_value;}
  public struct Int32{int m_value;}
  public struct UInt32{uint m_value;}
  public struct Int64{long m_value;}
  public struct UInt64{ulong m_value;}
  public struct Single{float m_value;}
  public struct Double{double m_value;}
  public struct Char{char m_value;}
  public struct Boolean{bool m_value;}

  public struct Decimal{byte notyet;
    public Decimal(int lo, int mid, int hi, bool isNegative,  byte scale){}
    public Decimal(int val){}
  }
  public struct IntPtr{int notyet;}
  public struct TypedReference{byte notyet;}
  public struct ArgIterator{int notyet;}
  public struct RuntimeFieldHandle{int notyet;}
  public struct RuntimeArgumentHandle{int notyet;}
  public struct RuntimeTypeHandle{int notyet;}

  public class Enum:ValueType{}
  
  public abstract class Array{
    public int Rank{get{return 0;}}
    public int Length{get{return 0;}}
    public int GetLength(int d){return 0;}
    public int GetUpperBound(int d){return 0;}
    public int GetLowerBound(int d){return 0;}
    public abstract void CopyTo(Array a, int i);
  }
  
  class Type{
    [MethodImplAttribute (MethodImplOptions.InternalCall)]
    extern public static Type internal_type_from_handle(RuntimeTypeHandle h);
    
    public static Type GetTypeFromHandle(RuntimeTypeHandle h){
	return internal_type_from_handle(h);
    }
  }
  
  class Exception{}
  class InvalidOperationException:Exception{}
  class NotSupportedException:Exception{}
  
  class MarshalByRefObject{}
  
  class Delegate{
    int unused1;
    Object target;
    int unused2;
    int method_ptr;
    public static Delegate Combine(Delegate a, Delegate b){
      return null;
    }
    public static Delegate Remove(Delegate a, Delegate b){return null;}
  }
  class MulticastDelegate:Delegate{}
  delegate void AsyncCallback(IAsyncResult r);

  interface IComparable{}
  interface ICloneable{
    Object Clone();
  }
  interface IConvertible{}
 interface IAsyncResult{}
 interface IDisposable{
   void Dispose();
 }
  namespace Collections{
    interface IEnumerable{
      IEnumerator GetEnumerator();
    }
    interface ICollection{
    }
    interface IEnumerator{
      Object Current{get;}
      void Reset();
      bool MoveNext();
    }
    interface IList{
    }
 }
 
  namespace Threading{
    class Monitor{
      public static void Enter(Object o){}
      public static void Exit(Object o){}
    }
  }

  namespace Runtime{
    namespace InteropServices{
      public class DllImportAttribute:Attribute{
        public DllImportAttribute(){}
      }
      class MarshalAsAttribute:Attribute{}
      class InAttribute:Attribute{}
      class OutAttribute:Attribute{}
      class InOutAttribute:Attribute{}
      class StructLayoutAttribute:Attribute{
        public StructLayoutAttribute(short kind){}
      }
      class FieldOffsetAttribute:Attribute{
        public FieldOffsetAttribute(int p){}
      }
      class GuidAttribute:Attribute{}
      class ComImportAttribute:Attribute{}
      class CoClassAttribute:Attribute{}
    }
    namespace CompilerServices{
      public class IndexerNameAttribute:Attribute{}
      public class MethodImplAttribute:Attribute{
        public MethodImplAttribute (MethodImplOptions options){}
      }
      public class RequiredAttributeAttribute:Attribute{}
      public class DecimalConstantAttribute:Attribute{
        public DecimalConstantAttribute(byte scale, byte sign, uint hi, uint mid, uint low){}
      }
      public class RuntimeHelpers{
         public static int OffsetToStringData{ get{return 0;}}
         public static void InitializeArray(Array a, RuntimeFieldHandle h){}
      }
      public enum MethodImplOptions {
         Unmanaged = 4, ForwardRef = 16, InternalCall = 4096,
         Synchronized = 32, NoInlining = 8, PreserveSig = 128
      }
    } 
    namespace Serialization{
      interface ISerializable{}
    }
 }
    namespace Reflection{
      interface IReflect{}
      interface ICustomAttributeProvider{}
      class MemberInfo{}
      class DefaultMemberAttribute:Attribute{
        public DefaultMemberAttribute(String name){}
      }
      class AssemblyCultureAttribute:Attribute{
      }
    }
    
    namespace Diagnostics{
      class ConditionalAttribute:Attribute{
      }
    }
    
    namespace Security{
      namespace Permissions{
         class SecurityAttribute:Attribute{}
      }
      class UnverifiableCodeAttribute:Attribute{}
      class CodeAccessPermission{}
    }
}
