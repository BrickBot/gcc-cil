using System;

namespace EnumTest
{
	public enum Color:byte{Yellow,Red,Green,Blue,Pink,Brown};
	public enum Flags:byte{EINS=0x01,ZWEI=0x02,DREI=0x04,VIER=0x08};
	public class Test
	{
		Color TestColor
		{
			set
			{
                Color col=value;
			}
			get
			{
				return Color.Red;
			}
		}
		public static void WriteColor(Color c)
		{
			Color _c = c;
		}

		public Color GetColor(Color c)
		{
			return c;
		}

		public static void Main()
		{
			Color color;
			color = (Color)1;
			WriteColor(color);
			Test t = new Test();
			t.GetColor(color);

			Flags flags = Flags.ZWEI | Flags.DREI;
			if((flags&Flags.DREI)!=Flags.DREI) System.Environment.Exit(1);
			//if((flags&Flags.VIER)>0) System.Environment.Exit(1);
			System.Environment.Exit(0);
		}
	}
}
