using System;
using System.Collections.Generic;

namespace MultiParse.Default
{
    public class MPTan : MPFunction
    {

        /// <summary>
        /// Constructor
        /// </summary>
        public MPTan()
            : base("[tT]an", false)
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
                throw new InvalidArgumentCountException(1, "Tan()");
            object top = PopOrGet(output);
            Tan(output, top);
        }

        /// <summary>
        /// Tangent
        /// </summary>
        /// <param name="output"></param>
        /// <param name="arg"></param>
        public void Tan(Stack<object> output, object arg)
        {
            // Calculate
            double v;
            if (CastImplicit(arg, out v))
                output.Push(Math.Tan(v));
            else
                throw new InvalidArgumentTypeException("Tan()", arg);
        }
    }
}
