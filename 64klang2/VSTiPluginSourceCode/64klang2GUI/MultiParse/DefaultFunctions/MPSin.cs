using System;
using System.Collections.Generic;

namespace MultiParse.Default
{
    public class MPSin : MPFunction
    {

        /// <summary>
        /// Constructor
        /// </summary>
        public MPSin()
            : base("[sS]in", false)
        {
        }

        /// <summary>
        /// Execute Asin
        /// </summary>
        /// <param name="output"></param>
        /// <param name="arguments"></param>
        public override void Execute(Stack<object> output, int arguments)
        {
            // check number of arguments
            if (arguments != 1)
                throw new InvalidArgumentCountException(1, "Sin()");
            object top = PopOrGet(output);
            Sin(output, top);
        }

        /// <summary>
        /// Arc sine
        /// </summary>
        /// <param name="output"></param>
        /// <param name="arg"></param>
        public void Sin(Stack<object> output, object arg)
        {
            // Calculate
            double v;
            if (CastImplicit(arg, out v))
                output.Push(Math.Sin(v));
            else
                throw new InvalidArgumentTypeException("Sin()", arg);
        }
    }
}
