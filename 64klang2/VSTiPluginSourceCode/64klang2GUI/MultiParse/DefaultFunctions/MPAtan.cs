using System;
using System.Collections.Generic;

namespace MultiParse.Default
{
    public class MPAtan : MPFunction
    {

        /// <summary>
        /// Constructor
        /// </summary>
        public MPAtan()
            : base("[aA]tan", false)
        {
        }

        /// <summary>
        /// Execute Atan
        /// </summary>
        /// <param name="output"></param>
        /// <param name="arguments"></param>
        public override void Execute(Stack<object> output, int arguments)
        {
            // check number of arguments
            if (arguments != 1)
                throw new InvalidArgumentCountException(1, "Atan()");
            object top = PopOrGet(output);
            Atan(output, top);
        }

        /// <summary>
        /// Arc tangent
        /// </summary>
        /// <param name="output"></param>
        /// <param name="arg"></param>
        public void Atan(Stack<object> output, object arg)
        {
            // Calculate
            double v;
            if (CastImplicit(arg, out v))
                output.Push(Math.Atan(v));
            else
                throw new InvalidArgumentTypeException("Atan()", arg);
        }
    }
}
