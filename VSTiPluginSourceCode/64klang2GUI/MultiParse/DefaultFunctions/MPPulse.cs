using System;
using System.Collections.Generic;

namespace MultiParse.Default
{
	public class MPPulse : MPFunction
	{
		/// <summary>
		/// Constructor
		/// </summary>
		public MPPulse()
			: base("[pP]ulse", false)
		{
		}

		/// <summary>
		/// Execute the max function
		/// </summary>
		/// <param name="arguments"></param>
		public override void Execute(Stack<object> output, int arguments)
		{
			if (arguments != 2)
				throw new InvalidArgumentCountException(2, "MPPulse()");

			// Pop two objects from the stack
			object color = PopOrGet(output);
			object phase = PopOrGet(output);
			Pulse(output, phase, color);
		}

		/// <summary>
		/// Maximum value
		/// </summary>
		/// <param name="output"></param>
		/// <param name="left"></param>
		/// <param name="right"></param>
		public void Pulse(Stack<object> output, object phase, object color)
		{
			double p, c;
			if (CastImplicit(phase, out p) && CastImplicit(color, out c))
			{
				if (p > c)
					output.Push((double)-1.0);
				else
					output.Push((double)1.0);
			}
			else
				throw new InvalidArgumentTypeException("MPPulse()", phase, color);
		}
	}
}
