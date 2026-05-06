using System;
using System.Collections.Generic;

namespace MultiParse.Default
{
    public class MPAcos : MPFunction
    {

        /// <summary>
        /// Constructor
        /// </summary>
        public MPAcos()
            : base("[aA]cos", false)
        {
        }

        /// <summary>
        /// Execute Acos
        /// </summary>
        /// <param name="output"></param>
        /// <param name="arguments"></param>
        public override void Execute(Stack<object> output, int arguments)
        {
            // check number of arguments
            if (arguments != 1)
                throw new InvalidArgumentCountException(1, "Acos()");
            object top = PopOrGet(output);
            Acos(output, top);
        }

        /// <summary>
        /// Arc cosine
        /// </summary>
        /// <param name="output"></param>
        /// <param name="arg"></param>
        public void Acos(Stack<object> output, object arg)
        {
            // Calculate
            double v;
            if (CastImplicit(arg, out v))
                output.Push(Math.Acos(v));
            else
                throw new InvalidArgumentTypeException("Acos()", arg);
        }
    }
}
