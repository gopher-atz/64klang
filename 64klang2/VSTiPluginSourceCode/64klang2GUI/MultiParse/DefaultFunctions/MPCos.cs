using System;
using System.Collections.Generic;

namespace MultiParse.Default
{
    public class MPCos : MPFunction
    {

        /// <summary>
        /// Constructor
        /// </summary>
        public MPCos()
            : base("[cC]os", false)
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
                throw new InvalidArgumentCountException(1, "Cos()");
            object top = PopOrGet(output);
            Cos(output, top);
        }

        /// <summary>
        /// Cosine
        /// </summary>
        /// <param name="output"></param>
        /// <param name="arg"></param>
        public void Cos(Stack<object> output, object arg)
        {
            // Calculate
            double v;
            if (CastImplicit(arg, out v))
                output.Push(Math.Cos(v));
            else
                throw new InvalidArgumentTypeException("Cos()", arg);
        }
    }
}
