using System;
using System.Collections.Generic;

namespace MultiParse.Default
{
    public class MPCosh : MPFunction
    {

        /// <summary>
        /// Constructor
        /// </summary>
        public MPCosh()
            : base("[cC]osh", false)
        {
        }

        /// <summary>
        /// Execute Cosh
        /// </summary>
        /// <param name="output"></param>
        /// <param name="arguments"></param>
        public override void Execute(Stack<object> output, int arguments)
        {
            // check number of arguments
            if (arguments != 1)
                throw new InvalidArgumentCountException(1, "Cosh()");
            object top = PopOrGet(output);
            Cosh(output, top);
        }

        /// <summary>
        /// Cosine hyperbolic
        /// </summary>
        /// <param name="output"></param>
        /// <param name="arg"></param>
        public void Cosh(Stack<object> output, object arg)
        {
            // Calculate
            double v;
            if (CastImplicit(arg, out v))
                output.Push(Math.Cosh(v));
            else
                throw new InvalidArgumentTypeException("Cosh()", arg);
        }
    }
}
