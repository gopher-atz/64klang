using System;
using System.Collections.Generic;

namespace MultiParse.Default
{
    public class MPExp2 : MPFunction
    {

        /// <summary>
        /// Constructor
        /// </summary>
        public MPExp2()
            : base("[eE]xp2", false)
        {
        }

        /// <summary>
        /// Execute Exp2
        /// </summary>
        /// <param name="output"></param>
        /// <param name="arguments"></param>
        public override void Execute(Stack<object> output, int arguments)
        {
            // check number of arguments
            if (arguments != 1)
                throw new InvalidArgumentCountException(1, "Exp2()");
            object top = PopOrGet(output);
            Exp2(output, top);
        }

        /// <summary>
        /// Exponent Base 2
        /// </summary>
        /// <param name="output"></param>
        /// <param name="arg"></param>
        public void Exp2(Stack<object> output, object arg)
        {
            // Calculate
            double v;
            if (CastImplicit(arg, out v))
                output.Push(Math.Pow(2.0, v));
            else
                throw new InvalidArgumentTypeException("Exp2()", arg);
        }
    }
}
