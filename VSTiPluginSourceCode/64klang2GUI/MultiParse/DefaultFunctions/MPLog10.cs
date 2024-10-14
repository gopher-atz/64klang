using System;
using System.Collections.Generic;

namespace MultiParse.Default
{
    public class MPLog10 : MPFunction
    {

        /// <summary>
        /// Constructor
        /// </summary>
        public MPLog10()
            : base("[lL]og10", false)
        {
        }

        /// <summary>
        /// Execute Log10
        /// </summary>
        /// <param name="output"></param>
        /// <param name="arguments"></param>
        public override void Execute(Stack<object> output, int arguments)
        {
            // check number of arguments
            if (arguments != 1)
                throw new InvalidArgumentCountException(1, "Log10()");
            object top = PopOrGet(output);
            Log10(output, top);
        }

        /// <summary>
        /// Logarithm base 10
        /// </summary>
        /// <param name="output"></param>
        /// <param name="arg"></param>
        public void Log10(Stack<object> output, object arg)
        {
            // Calculate
            double v;
            if (CastImplicit(arg, out v))
                output.Push(Math.Log10(v));
            else
                throw new InvalidArgumentTypeException("Log10()", arg);
        }
    }
}
