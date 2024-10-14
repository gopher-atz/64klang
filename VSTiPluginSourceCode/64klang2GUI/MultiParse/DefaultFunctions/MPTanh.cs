using System;
using System.Collections.Generic;

namespace MultiParse.Default
{
    public class MPTanh : MPFunction
    {

        /// <summary>
        /// Constructor
        /// </summary>
        public MPTanh()
            : base("[tT]anh", false)
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
                throw new InvalidArgumentCountException(1, "Tanh()");
            object top = PopOrGet(output);
            Tanh(output, top);
        }

        /// <summary>
        /// Tangent hyperbolic
        /// </summary>
        /// <param name="output"></param>
        /// <param name="arg"></param>
        public void Tanh(Stack<object> output, object arg)
        {
            // Calculate
            double v;
            if (CastImplicit(arg, out v))
                output.Push(Math.Tanh(v));
            else
                throw new InvalidArgumentTypeException("Tanh()", arg);
        }
    }
}
