using System;
using System.Collections.Generic;

namespace MultiParse.Default
{
    public class MPSqrt : MPFunction
    {

        /// <summary>
        /// Constructor
        /// </summary>
        public MPSqrt()
            : base("[sS]qrt", false)
        {
        }

        /// <summary>
        /// Execute Sqrt
        /// </summary>
        /// <param name="output"></param>
        /// <param name="arguments"></param>
        public override void Execute(Stack<object> output, int arguments)
        {
            // check number of arguments
            if (arguments != 1)
                throw new InvalidArgumentCountException(1, "Sqrt()");
            object top = PopOrGet(output);
            Sqrt(output, top);
        }

        /// <summary>
        /// Square root
        /// </summary>
        /// <param name="output"></param>
        /// <param name="arg"></param>
        public void Sqrt(Stack<object> output, object arg)
        {
            // Calculate
            double v;
            if (CastImplicit(arg, out v))
                output.Push(Math.Sqrt(v));
            else
                throw new InvalidArgumentTypeException("Sqrt()", arg);
        }
    }
}
