using System;
using System.Collections.Generic;

namespace MultiParse.Default
{
    public class MPAsin : MPFunction
    {

        /// <summary>
        /// Constructor
        /// </summary>
        public MPAsin()
            : base("[aA]sin", false)
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
                throw new InvalidArgumentCountException(1, "Asin()");
            object top = PopOrGet(output);
            Asin(output, top);
        }

        /// <summary>
        /// Arc sine
        /// </summary>
        /// <param name="output"></param>
        /// <param name="arg"></param>
        public void Asin(Stack<object> output, object arg)
        {
            // Calculate
            double v;
            if (CastImplicit(arg, out v))
                output.Push(Math.Asin(v));
            else
                throw new InvalidArgumentTypeException("Asin()", arg);
        }
    }
}
