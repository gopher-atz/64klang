using System;
using System.Collections.Generic;

namespace MultiParse.Default
{
	public class MPSqr : MPFunction
	{

		/// <summary>
		/// Constructor
		/// </summary>
		public MPSqr()
			: base("[sS]qr", false)
		{
		}

		/// <summary>
		/// Execute Sqr
		/// </summary>
		/// <param name="output"></param>
		/// <param name="arguments"></param>
		public override void Execute(Stack<object> output, int arguments)
		{
			// check number of arguments
			if (arguments != 1)
				throw new InvalidArgumentCountException(1, "Sqr()");
			object top = PopOrGet(output);
			Sqr(output, top);
		}

		/// <summary>
		/// Square root
		/// </summary>
		/// <param name="output"></param>
		/// <param name="arg"></param>
		public void Sqr(Stack<object> output, object arg)
		{
			// Calculate
			double v;
			if (CastImplicit(arg, out v))
				output.Push(v*v);
			else
				throw new InvalidArgumentTypeException("Sqr()", arg);
		}
	}
}
