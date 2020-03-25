namespace Hardware
{
    public static class Cpu
    {
        [StaticDelegate]
        public delegate void ProgramCounterDelegate ();
    
        [StaticDelegate]
        public delegate Stack SchedulerDelegate (Stack stack);
                  
        public static SchedulerDelegate Scheduler;
        
        public struct Stack
        {
          public ushort Pointer;
        }

#pragma warning disable 0626
        public extern static void Sleep ();
        public extern static void InvokeScheduler ();
          
        public extern static Stack InitStack (ProgramCounterDelegate pcd, ushort stackStart);
#pragma warning restore 0626
    }
}
