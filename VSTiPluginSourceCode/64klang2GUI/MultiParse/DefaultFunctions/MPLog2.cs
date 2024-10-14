using System;
using System.Collections.Generic;

namespace MultiParse.Default
{
    public class MPLog2 : MPFunction
    {

        /// <summary>
        /// Constructor
        /// </summary>
        public MPLog2()
            : base("[lL]og2", false)
        {
        }

        /// <summary>
        /// Execute Log2
        /// </summary>
        /// <param name="output"></param>
        /// <param name="arguments"></param>
        public override void Execute(Stack<object> output, int arguments)
        {
            // check number of arguments
            if (arguments != 1)
                throw new InvalidArgumentCountException(1, "Log2()");
            object top = PopOrGet(output);
            Log2(output, top);
        }

        /// <summary>
        /// Logarithm base 10
        /// </summary>
        /// <param name="output"></param>
        /// <param name="arg"></param>
        public void Log2(Stack<object> output, object arg)
        {
            // Calculate
            double v;
            if (CastImplicit(arg, out v))
                output.Push(Math.Log(v, 2.0));
            else
                throw new InvalidArgumentTypeException("Log2()", arg);
        }
    }
}
