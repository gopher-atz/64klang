using System;
using System.Collections.Generic;

namespace MultiParse.Default
{
	public class MPTriSaw : MPFunction
	{
		/// <summary>
		/// Constructor
		/// </summary>
		public MPTriSaw()
			: base("[tT]risaw", false)
		{
		}

		/// <summary>
		/// Execute the max function
		/// </summary>
		/// <param name="arguments"></param>
		public override void Execute(Stack<object> output, int arguments)
		{
			if (arguments != 2)
				throw new InvalidArgumentCountException(2, "MPTriSaw()");

			// Pop two objects from the stack
			object color = PopOrGet(output);
			object phase = PopOrGet(output);
			TriSaw(output, phase, color);
		}

		/// <summary>
		/// Maximum value
		/// </summary>
		/// <param name="output"></param>
		/// <param name="left"></param>
		/// <param name="right"></param>
		public void TriSaw(Stack<object> output, object phase, object color)
		{
			double p, c;
			if (CastImplicit(phase, out p) && CastImplicit(color, out c))
			{
				c = c % 1.0;
				if (c < 0.00001) c = 0.00001;
				if (c > 0.99999) c = 0.99999;
				if (p > c)
					output.Push((double)(2.0 * (1.0 - p) / (1.0 - c) - 1.0));
				else
					output.Push((double)(2.0 * p / c - 1.0));
			}
			else
				throw new InvalidArgumentTypeException("TriSaw()", phase, color);			
		}
	}
}
